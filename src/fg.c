/*
 *
 *
 *
 */

#include <kernel.h>

#include "nrf.h"
#include "fg.h"

#define CONFIG_SYS_LOG_FG_LEVEL 1

#define SYS_LOG_DOMAIN "fg"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_FG_LEVEL
#include <logging/sys_log.h>

#define FG_MEAS_INTERVAL 120
#define FG_VBG         1200
#define FG_PRESCALER   3
#define FG_NUM_VBAT_SAMPLES 30

static fg_update_cb_t fg_cb;
static struct k_timer meas_timer;
static struct k_work meas_work;

// ToDo: Add temperature dependant compensation
// const uint16_t c_bat_levels_cr2032[BAT_LEVELS_CR2023] = {2800, 2700, 2600, 2500};
const uint8_t temperature_compensation = 5;    // 5 mA * 1 Ohm @ 25 degC

static uint16_t vbat_avg[FG_NUM_VBAT_SAMPLES];
static uint8_t  vbat_idx;

static void meas_work_handler(struct k_work *work);
static void meas_timer_handler(struct k_timer *timer);

static uint16_t adc_acquire(void)
{
    uint16_t adc_raw;
    uint16_t vbat;

    // Configure ADC
    NRF_ADC->CONFIG = (ADC_CONFIG_RES_10bit << ADC_CONFIG_RES_Pos)
                    | (ADC_CONFIG_INPSEL_SupplyOneThirdPrescaling << ADC_CONFIG_INPSEL_Pos)
                    | (ADC_CONFIG_REFSEL_VBG << ADC_CONFIG_REFSEL_Pos)
                    | (ADC_CONFIG_PSEL_Disabled << ADC_CONFIG_PSEL_Pos)
                    | (ADC_CONFIG_EXTREFSEL_None << ADC_CONFIG_EXTREFSEL_Pos);
    NRF_ADC->EVENTS_END = 0;
    NRF_ADC->ENABLE = ADC_ENABLE_ENABLE_Enabled;

    NRF_ADC->EVENTS_END = 0;    // Stop any running conversions.
    NRF_ADC->TASKS_START = 1;

    while (!NRF_ADC->EVENTS_END) {
        // Wait for ADC to finish measurement
    }

    adc_raw = NRF_ADC->RESULT;

    NRF_ADC->EVENTS_END = 0;
    NRF_ADC->TASKS_STOP = 1;

    vbat = adc_raw * FG_PRESCALER * FG_VBG / 1024;
    vbat += temperature_compensation;

    vbat_avg[vbat_idx++] = vbat;
    vbat_idx %= FG_NUM_VBAT_SAMPLES;

    SYS_LOG_DBG("ADC:%d", adc_raw);
    SYS_LOG_DBG("VBAT:%d", vbat);

    return adc_raw;
}


static uint16_t vbat_avg_get(void)
{
    uint32_t vbat_tot = 0;
    uint8_t num_samples = 0;

    for (int i = 0; i < FG_NUM_VBAT_SAMPLES; i++) {
        if (vbat_avg[i] != 0) {
            vbat_tot += vbat_avg[i];
            num_samples++;
        }
    }

    SYS_LOG_DBG("VBAT_AVG:%d %d", vbat_tot / num_samples, num_samples);

    return (vbat_tot / num_samples);
}

static uint8_t convert_vbat_to_capacity(uint32_t vbat)
{
    double c_max = 100;
    double c_min = 0;
    double v_max = 3100;
    double v_min = 2400;
    double k;
    double m;
    double capacity;

    k = (c_max - c_min) / (v_max - v_min);
    m = c_max - k * v_max;
    capacity = k * vbat + m;

    if (capacity > 100) {
        return 100;
    } else if (capacity < 0) {
        return 0;
    }

    return (uint8_t)(capacity + 0.5);
}

static void meas_work_handler(struct k_work *work)
{
    uint16_t vbat;
    uint8_t capacity;

    SYS_LOG_DBG("Periodic FG measurement");

    adc_acquire();
    vbat = vbat_avg_get();
    capacity = convert_vbat_to_capacity(vbat);

    SYS_LOG_DBG("Capacity:%d", capacity);

    if (fg_cb != NULL) {
        fg_cb(capacity);
    }
}

static void meas_timer_handler(struct k_timer *timer)
{
    k_work_submit(&meas_work);
}

void fg_init(fg_update_cb_t cb)
{
    fg_cb = cb;

    adc_acquire();

    k_work_init(&meas_work, meas_work_handler);
    k_timer_init(&meas_timer, meas_timer_handler, NULL);
    k_timer_start(&meas_timer, K_SECONDS(5), K_SECONDS(FG_MEAS_INTERVAL));
}
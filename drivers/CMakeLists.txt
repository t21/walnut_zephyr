zephyr_include_directories(
  i2cwrap
)

add_subdirectory_ifdef(CONFIG_I2C_WRAP  i2c_wrap)
add_subdirectory_ifdef(CONFIG_SI7020    si7020)
add_subdirectory_ifdef(CONFIG_TSL4531   tsl4531)
add_subdirectory_ifdef(CONFIG_BMP280    bmp280)

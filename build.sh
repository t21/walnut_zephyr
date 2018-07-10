#!/bin/bash

# default arguments
CLEAN=1
DEBUG=1
#BUILD_BOOTLOADER=1
#BUILD_FW=1
#BOARD_VARIANT=EVT1

# default directory definitions
BOOT_BUILD_DIR=./boot/build
FW_BUILD_DIR=./build
KEY_DIR=./boot

# Parse command line arguments
while [[ $# -gt 0 ]]
do
key="$1"

case $key in
    clean)
    CLEAN=1
    shift
    ;;
    CLEAN)
    CLEAN=1
    shift
    ;;
    boot)
    BUILD_BOOTLOADER=1
    shift
    ;;
    BOOT)
    BUILD_BOOTLOADER=1
    shift
    ;;
    climate | CLIMATE )
    BUILD_FW=1
    BOARD_VARIANT=CLIMATE
    shift
    ;;
    evt2)
    BUILD_FW=1
    BOARD_VARIANT=EVT2
    shift
    ;;
    EVT2)
    BUILD_FW=1
    BOARD_VARIANT=EVT2
    shift
    ;;
    flash)
    FLASH=1
    shift
    ;;
    FLASH)
    FLASH=1
    shift
    ;;
    help)
    HELP=1
    shift
    ;;
    *)    # unknown option
    HELP=1
    #POSITIONAL+=("$1") # save it in an array for later
    shift
    ;;
esac
done

function usage {
    echo "Usage:"
    echo "   ./build.sh <arg_1> <arg_2> <arg_n>"
    echo ""
    echo "Arguments:"
    echo "   boot  - build bootloader"
    echo "   evt1  - build for evt1 board (cleans build dir first)"
    echo "   evt2  - build for evt2 board (cleans build dir first)"
    echo "   flash - flash the board, the file to flash depends on \"boot\" and \"evtx\" build options"
    echo "   help  - shows this help text"
    echo ""
    echo "Examples:"
    echo "   ./build.sh boot evt1        - build bootloader and app for evt1 board"
    echo "   ./build.sh boot evt2 flash  - build bootloader and app for evt2 board plus flash the board"
    echo "   ./build.sh boot flash       - build and flash bootloader"
    echo "   ./build.sh evt1 flash       - build and flash app for evt1 board"
    echo ""
}

function clean_boot_build_dir {
    echo -e "\n******************************************"
    echo -e "***  Clean bootloader build directory  ***"
    echo -e "******************************************"
    rm -r $BOOT_BUILD_DIR
    mkdir -p $BOOT_BUILD_DIR
    touch $BOOT_BUILD_DIR/.keep
    echo "Done!"
}

function clean_fw_build_dir {
    echo -e "\n**********************************************"
    echo -e "***     Clean firmware build directory     ***"
    echo -e "**********************************************"
    rm -r $FW_BUILD_DIR
    mkdir -p $FW_BUILD_DIR
    touch $FW_BUILD_DIR/.keep
    echo "Done!"
}

function cmake_bootloader {
    echo -e "\n**********************************"
    echo -e "***  Run cmake for bootloader  ***"
    echo -e "**********************************"

    if [ ! -d "$BOOT_BUILD_DIR" ]; then
        mkdir -p $BOOT_BUILD_DIR
    fi

    pushd $BOOT_BUILD_DIR > /dev/null || exit 1

    cmake -GNinja -DBOARD=trackr_smart_tag -DBOARD_VARIANT=BOOT .. || exit 1

    popd > /dev/null
}

function build_bootloader {
    echo -e "\n**************************"
    echo -e "***  Build bootloader  ***"
    echo -e "**************************"

    if [ ! -d "$BOOT_BUILD_DIR" ]; then
        mkdir -p $BOOT_BUILD_DIR
    fi

    pushd $BOOT_BUILD_DIR > /dev/null || exit 1

    ninja || exit 1

    popd > /dev/null
}

function cmake_fw {
    echo -e "\n**************************************"
    echo -e "***     Run cmake for firmware     ***"
    echo -e "**************************************"

    if [ ! -d "$FW_BUILD_DIR" ]; then
        mkdir -p $FW_BUILD_DIR
    fi

    pushd $FW_BUILD_DIR > /dev/null || exit 1

    if [ -z "$BOARD_VARIANT" ]
    then
        cmake -GNinja -DBOARD=walnut .. || exit 1
    else
        cmake -GNinja -DBOARD=walnut -DBOARD_VARIANT=$BOARD_VARIANT .. || exit 1
    fi

    popd > /dev/null
}

function build_fw {
    echo -e "\n******************************"
    echo -e "***     Build firmware     ***"
    echo -e "******************************"

    if [ ! -d "$FW_BUILD_DIR" ]; then
        mkdir -p $FW_BUILD_DIR
    fi

    pushd $FW_BUILD_DIR > /dev/null || exit 1

    ninja || exit 1

    popd > /dev/null
}

function get_version {
    echo -e "\n***************************"
    echo -e "***     Get Version     ***"
    echo -e "***************************"

    NEAREST=$(git describe --tags --dirty)
    echo "Nearest release Tag: \"$NEAREST\""

    MAJOR="0"
    MINOR="0"
    PATCH="0"

    if [ "$NEAREST" == "" ]
    then
        echo "No release tag found!"
    else
        MAJOR=$(echo $NEAREST | cut -d "." -f 1)
        if [[ $MAJOR == "" ]]
        then
            MAJOR="0"
        else
            MINOR=$(echo $NEAREST | cut -d "." -f 2 | cut -d "-" -f 1)
            if [[ $MINOR == "" ]]
            then
                MINOR="0"
            else
                PATCH=$(echo $NEAREST | cut -d "-" -f 2 -s)
                if [[ $PATCH == "" ]]
                then
                    PATCH="0"
                else
                    REV=$(echo $NEAREST | cut -d "-" -f 3)
                    STATE=$(echo $NEAREST | cut -d "-" -f 4)
                fi
            fi
        fi
    fi

    echo "Firmware version: $MAJOR.$MINOR.$PATCH"
}

function sign {
    echo -e "\n***********************"
    echo -e "***  Sign firmware  ***"
    echo -e "***********************"

    ../../../external/mcuboot/scripts/imgtool.py sign --key $KEY_DIR/root-rsa-2048.pem \
    --header-size 0x200 --align 8 --version $MAJOR.$MINOR.$PATCH --included-header \
    $FW_BUILD_DIR/zephyr/zephyr.hex $FW_BUILD_DIR/walnut_signed_$BOARD_VARIANT.hex || exit 1

    ../../../external/mcuboot/scripts/imgtool.py sign --key $KEY_DIR/root-rsa-2048.pem \
    --header-size 0x200 --align 8 --version $MAJOR.$MINOR.$PATCH --included-header \
    $FW_BUILD_DIR/zephyr/zephyr.bin $FW_BUILD_DIR/walnut_signed_${BOARD_VARIANT}_OTA.bin || exit 1

    echo "Done!"
}

function merge {
    echo -e "\n***************************************"
    echo -e "***  Merge bootloader and firmware  ***"
    echo -e "***************************************"

    mergehex -m $BOOT_BUILD_DIR/zephyr/zephyr.hex \
    $FW_BUILD_DIR/walnut_signed_$BOARD_VARIANT.hex \
    -o $FW_BUILD_DIR/walnut_fw_plus_boot_signed_$BOARD_VARIANT.hex || exit 1
}

function flash {
    echo -e "\n**********************"
    echo -e "***  Flash device  ***"
    echo -e "**********************"

    if [ ! -z "$DEBUG" ]
    then
        FILE_TO_FLASH=$FW_BUILD_DIR/zephyr/zephyr.hex
        echo "Flashing firmware: '$FILE_TO_FLASH'"
    elif [ ! -z "$BUILD_BOOTLOADER" ] && [ -z "$BUILD_FW" ]
    then
        FILE_TO_FLASH=$BOOT_BUILD_DIR/zephyr/zephyr.hex
        echo "Flashing bootloader: '$FILE_TO_FLASH'"
    elif [ -z "$BUILD_BOOTLOADER" ] && [ ! -z "$BUILD_FW" ]
    then
        FILE_TO_FLASH=$FW_BUILD_DIR/walnut_signed_$BOARD_VARIANT.hex
        echo "Flashing firmware app: '$FILE_TO_FLASH'"
    elif [ ! -z "$BUILD_BOOTLOADER" ] && [ ! -z "$BUILD_FW" ]
    then
        FILE_TO_FLASH=$FW_BUILD_DIR/trackr_app_plus_boot_signed_$BOARD_VARIANT.hex
        echo "Flashing bootloader plus firmware app: '$FILE_TO_FLASH'"
    else
        FILE_TO_FLASH=$FW_BUILD_DIR/trackr_app_signed_$BOARD_VARIANT.hex
        echo "Flashing pre-built firmware app: '$FILE_TO_FLASH'"
    fi

    if [ -f $FILE_TO_FLASH ]
    then
        nrfjprog --program $FILE_TO_FLASH --sectorerase || exit 1
        nrfjprog -r || exit 1
    else
        echo "File \"$FILE_TO_FLASH\" does not exist."
        exit 0
    fi
}


if [ ! -z "$HELP" ]
then
    usage
    exit 0
fi


if [ ! -z "$BUILD_BOOTLOADER" ]; then
    if [ -v CLEAN ]; then
        clean_boot_build_dir
    fi

    cmake_bootloader
    build_bootloader
fi

if [ ! -z "$BUILD_FW" ]
then
    if [ ! -z "$CLEAN" ]
    then
        clean_fw_build_dir
    fi

    cmake_fw
    build_fw
    get_version

    if [ -z "$DEBUG" ]
    then
        sign
    fi

    if [ ! -z "$BUILD_BOOTLOADER" ]
    then
        merge
    fi
fi


if [ ! -z "$FLASH" ]
then
    flash
fi

exit 0

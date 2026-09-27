#!/usr/bin/env bash

set -e

atf_image()
{
        local ATF_VARIABLES
        local DTB_NAME
        local STM_NAME

        ATF_VARIABLES="$(sed -n \
                's/^BR2_TARGET_ARM_TRUSTED_FIRMWARE_ADDITIONAL_VARIABLES="\(.*\)"$/\1/p' \
                "${BR2_CONFIG}")"

        DTB_NAME="$(sed -n \
                's/.*DTB_FILE_NAME=\([^ ]*\)/\1/p' \
                <<< "${ATF_VARIABLES}")"

        STM_NAME="tf-a-$(cut -f1 -d'.' <<< "${DTB_NAME}").stm32"

        echo "${STM_NAME}"
}

generate_emmc()
{
        local ATFBIN
        local GENIMAGE_CFG

        ATFBIN="$(atf_image)"
        GENIMAGE_CFG="$(dirname "$0")/genimage-emmc.cfg"

        if [ ! -e "${BINARIES_DIR}/${ATFBIN}" ]; then
                echo "Cannot find ATF binary ${BINARIES_DIR}/${ATFBIN}"
                exit 1
        fi

        if [ ! -e "${GENIMAGE_CFG}" ]; then
                echo "Cannot find genimage config ${GENIMAGE_CFG}"
                exit 1
        fi

        for f in \
                tf-a-stm32mp257_dk_usb.stm32 \
                fip-stm32mp257_dk_usb.bin \
                fip-ddr-stm32mp257_dk_usb.bin
        do
                if [ ! -e "${BINARIES_DIR}/${f}" ]; then
                        echo "Cannot find ${BINARIES_DIR}/${f}"
                        exit 1
                fi
        done

        echo "Generating eMMC user-area image..."

        support/scripts/genimage.sh -c "${GENIMAGE_CFG}"

        gzip -fk "${BINARIES_DIR}/emmc.img"

        {
                printf '#opt\tId\tName\tType\tDevice\tOffset\tBinary\n'
                printf -- '-\t0x01\tfsbl_boot\tBinary\tnone\t0x0\ttf-a-stm32mp257_dk_usb.stm32\n'
                printf -- '-\t0x02\tfip_ddr\tFIP\tnone\t0x0\tfip-ddr-stm32mp257_dk_usb.bin\n'
                printf -- '-\t0x03\tfip_boot\tFIP\tnone\t0x0\tfip-stm32mp257_dk_usb.bin\n'
                printf 'P\t0x04\tfsbla1\tBinary\tmmc1\tboot1\t%s\n' "${ATFBIN}"
                printf 'P\t0x05\tfsbla2\tBinary\tmmc1\tboot2\t%s\n' "${ATFBIN}"
                printf 'P\t0x10\temmc\tRawImage\tmmc1\t0x0\temmc.img\n'
        } > "${BINARIES_DIR}/flash-emmc.tsv"

        awk -F '\t' '
        NF != 7 {
                print "Invalid TSV line " NR ": expected 7 columns, got " NF
                exit 1
        }
        ' "${BINARIES_DIR}/flash-emmc.tsv"

        echo
        echo "Generated:"
        echo "  ${BINARIES_DIR}/emmc.img"
        echo "  ${BINARIES_DIR}/emmc.img.gz"
        echo "  ${BINARIES_DIR}/flash-emmc.tsv"


}

generate_emmc "$@"

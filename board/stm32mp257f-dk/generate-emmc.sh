k#!/usr/bin/env bash

set -euo pipefail

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
        local TSV

        ATFBIN="$(atf_image)"
        GENIMAGE_CFG="$(dirname "$0")/genimage-emmc.cfg"
        TSV="${BINARIES_DIR}/flash-emmc.tsv"

        #
        # Vérification des fichiers nécessaires
        #
        if [ ! -e "${BINARIES_DIR}/${ATFBIN}" ]; then
                echo "ERROR: Cannot find ATF binary:"
                echo "  ${BINARIES_DIR}/${ATFBIN}"
                exit 1
        fi

        if [ ! -e "${GENIMAGE_CFG}" ]; then
                echo "ERROR: Cannot find genimage config:"
                echo "  ${GENIMAGE_CFG}"
                exit 1
        fi

        for f in \
                tf-a-stm32mp257_dk_usb.stm32 \
                fip-stm32mp257_dk_usb.bin \
                fip-ddr-stm32mp257_dk_usb.bin
        do
                if [ ! -e "${BINARIES_DIR}/${f}" ]; then
                        echo "ERROR: Cannot find:"
                        echo "  ${BINARIES_DIR}/${f}"
                        exit 1
                fi
        done


        #
        # Génération de l'image eMMC
        #
        echo
        echo "=========================================="
        echo "Generating eMMC user-area image"
        echo "=========================================="
        echo

        support/scripts/genimage.sh -c "${GENIMAGE_CFG}"

        if [ ! -e "${BINARIES_DIR}/emmc.img" ]; then
                echo "ERROR: emmc.img was not generated"
                exit 1
        fi


        #
        # Version compressée facultative
        #
        gzip -fk "${BINARIES_DIR}/emmc.img"


        #
        # Génération du FlashLayout STM32CubeProgrammer
        #
        # IMPORTANT :
        # printf utilise explicitement \t entre CHAQUE champ.
        # Le fichier obtenu est donc un vrai TSV et non un fichier
        # aligné avec des espaces.
        #
        echo
        echo "Generating CubeProgrammer TSV:"
        echo "  ${TSV}"

        rm -f "${TSV}"

        printf '%s\t%s\t%s\t%s\t%s\t%s\t%s\n' \
                '#opt' 'Id'   'Name'      'Type'     'Device' 'Offset' 'Binary' \
                '-'    '0x01' 'fsbl_boot' 'Binary'   'none'   '0x0'    'tf-a-stm32mp257_dk_usb.stm32' \
                '-'    '0x02' 'fip_ddr'   'FIP'      'none'   '0x0'    'fip-ddr-stm32mp257_dk_usb.bin' \
                '-'    '0x03' 'fip_boot'  'FIP'      'none'   '0x0'    'fip-stm32mp257_dk_usb.bin' \
                'P'    '0x04' 'fsbla1'    'Binary'   'mmc1'   'boot1'  "${ATFBIN}" \
                'P'    '0x05' 'fsbla2'    'Binary'   'mmc1'   'boot2'  "${ATFBIN}" \
                'P'    '0x10' 'emmc'      'RawImage' 'mmc1'   '0x0'    'emmc.img' \
                > "${TSV}"


        #
        # Vérification structurelle du TSV
        #
        echo
        echo "Checking TSV structure..."

        awk -F '\t' '
        {
                if (NF != 7) {
                        printf "ERROR: TSV line %d has %d columns instead of 7\n",
                               NR, NF
                        exit 1
                }
        }

        END {
                if (NR != 7) {
                        printf "ERROR: TSV contains %d lines instead of 7\n", NR
                        exit 1
                }
        }
        ' "${TSV}"


        #
        # Vérification supplémentaire :
        # chaque ligne doit contenir exactement 6 TAB
        #
        awk '
        {
                line = $0
                tabs = gsub(/\t/, "\t", line)

                if (tabs != 6) {
                        printf "ERROR: TSV line %d contains %d TAB characters instead of 6\n",
                               NR, tabs
                        exit 1
                }
        }
        ' "${TSV}"


        #
        # Affichage avec caractères spéciaux visibles
        #
        echo
        echo "TSV content (TABs must appear as \\t):"
        echo "------------------------------------------"
        sed -n 'l' "${TSV}"
        echo "------------------------------------------"


        #
        # Résumé
        #
        echo
        echo "=========================================="
        echo "eMMC image generation successful"
        echo "=========================================="
        echo
        echo "Generated:"
        echo "  ${BINARIES_DIR}/emmc.img"
        echo "  ${BINARIES_DIR}/emmc.img.gz"
        echo "  ${BINARIES_DIR}/flash-emmc.tsv"
        echo
        echo "flash-emmc.tsv validation OK"
        echo
}


generate_emmc "$@"

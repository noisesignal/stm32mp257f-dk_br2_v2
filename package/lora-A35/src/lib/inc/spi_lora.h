#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <linux/gpio.h>


#define SPI_DEVICE "/dev/spidev0.0"
#define SPI_SPEED 10000000
#define SPI_MODE SPI_MODE_0
#define SPI_BITS_PER_WORD 8
#define GPIO_CHIP    "/dev/gpiochip0"
#define DIO0_PIN     5 //PA5
/* NSS n'est plus géré ici : piloté automatiquement par le kernel via cs-gpios dans le DT */


#define RF_FIFO        ( 0x00 )
#define RF_OPMODE      ( 0x01 )
#define RF_FREQ_MSB    ( 0x06 )
#define RF_FREQ_MID    ( 0x07 )
#define RF_FREQ_LSB    ( 0x08 )
#define RF_FIFO_ADDR   ( 0x0D )
#define RF_FIFO_TXBASE ( 0x0E )
#define RF_FIFO_RXBASE ( 0x0F )
#define RF_FIFO_RXADDR ( 0x10 )
#define RF_IRQ_FLAGS   ( 0x12 )
#define RF_RX_LEN      ( 0x13 )
#define RF_LORA_STAT   ( 0x18 )
#define RF_TX_LEN      ( 0x22 )
#define RF_ID          ( 0x42 )

#define RF_OP_MOD      ( 0x80 )
#define RF_OP_MOD_OOK  ( 0x00 )
#define RF_OP_MOD_LORA ( 0x80 )
#define RF_OP_FREQ     ( 0x08 )
#define RF_OP_FREQ_HF  ( 0x00 )
#define RF_OP_FREQ_LF  ( 0x08 )
#define RF_OP_MD       ( 0x07 )
#define RF_OP_MD_SLEEP ( 0x00 )
#define RF_OP_MD_STDBY ( 0x01 )
#define RF_OP_MD_FSTX  ( 0x02 )
#define RF_OP_MD_TX    ( 0x03 )
#define RF_OP_MD_FSRX  ( 0x04 )
#define RF_OP_MD_RXC   ( 0x05 )
#define RF_OP_MD_RXS   ( 0x06 )
#define RF_OP_MD_CAD   ( 0x07 )

#define RF_IRQ_RX_TIMEOUT       ( 0x80 )
#define RF_IRQ_RX_DONE          ( 0x40 )
#define RF_IRQ_CRC_ERROR        ( 0x20 )
#define RF_IRQ_HDR_VALID        ( 0x10 )
#define RF_IRQ_TX_DONE          ( 0x08 )
#define RF_IRQ_CAD_DONE         ( 0x04 )
#define RF_IRQ_FHSS_CHANGE_CHAN ( 0x02 )
#define RF_IRQ_CAD_DETECTED     ( 0x01 )

#define RF_MDM_RX_RATE   ( 0xE0 )
#define RF_MDM_CLEAR     ( 0x10 )
#define RF_MDM_HDR_VALID ( 0x08 )
#define RF_MDM_RX        ( 0x04 )
#define RF_MDM_SYNC      ( 0x02 )
#define RF_MDM_SIG       ( 0x01 )
#define RegPaConfig	    (0x09)
#define PA_BOOST	    (0x80)
#define LORARegRssiValue    (0x1B) 
#define RegPaDac	    (0x4D)
#define LORARegPktRssiValue (0x1A)
#define RegModemConfig2     (0x1E)
#define RegModemConfig1     (0x1D)

//int spi_fd;

#ifndef SPI_LORA_H
#define SPI_LORA_H
extern int spi_fd;  // Déclaration de la variable globale, sans définition
#endif

#define MESSAGE "STM32MP157_SPIDEV"
#define NUMBCHAR (sizeof(MESSAGE) - 1)


void spi_init(void);
void dio0_irq_init(void);
int dio0_wait_irq(void);
void dio0_irq_close(void);
uint8_t readReg(uint8_t addr);
void LoraInit(void);
void LoraRx(void);
void LoraTx(void);
void LoraID(void);
void writeReg(uint8_t addr, uint8_t val);
void opmode (uint8_t mode);
static const unsigned char eu863_lora_frequency[8][3] = {
	{0xD9, 0x06, 0x8B}, // Channel 0 868.100 MHz / 61.035 Hz = 14222987 = 0xD9068B
	{0xD9, 0x13, 0x58}, // Channel 1 868.300 MHz / 61.035 Hz = 14226264 = 0xD91358
	{0xD9, 0x20, 0x24}, // Channel 2 868.500 MHz / 61.035 Hz = 14229540 = 0xD92024
	{0xD8, 0xC6, 0x8B}, // Channel 3 867.100 MHz / 61.035 Hz = 14206603 = 0xD8C68B
	{0xD8, 0xD3, 0x58}, // Channel 4 867.300 MHz / 61.035 Hz = 14209880 = 0xD8D358
	{0xD8, 0xE0, 0x24}, // Channel 5 867.500 MHz / 61.035 Hz = 14213156 = 0xD8E024
	{0xD8, 0xEC, 0xF1}, // Channel 6 867.700 MHz / 61.035 Hz = 14216433 = 0xD8ECF1
	{0xD8, 0xF9, 0xBE} // Channel 7 867.900 MHz / 61.035 Hz = 14219710 = 0xD8F9BE
};

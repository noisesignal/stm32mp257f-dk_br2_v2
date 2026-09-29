#include "spi_lora.h"

int spi_fd;

/* NSS n'est plus géré ici : le kernel pilote automatiquement la broche
   CS déclarée via cs-gpios dans le DT, à chaque ioctl(SPI_IOC_MESSAGE). */

// ==========================
//  IRQ DIO0 (ABI GPIO chardev v2)
// ==========================

static int dio0_line_fd = -1;

void dio0_irq_init(void)
{
    int chip_fd;
    struct gpio_v2_line_request req;

    chip_fd = open(GPIO_CHIP, O_RDONLY);
    if (chip_fd < 0) {
        perror("Erreur ouverture GPIO chip");
        exit(EXIT_FAILURE);
    }

    memset(&req, 0, sizeof(req));
    req.num_lines = 1;
    req.offsets[0] = DIO0_PIN;
    req.config.flags = GPIO_V2_LINE_FLAG_INPUT | GPIO_V2_LINE_FLAG_EDGE_RISING;
    strncpy(req.consumer, "RFM95W_DIO0", sizeof(req.consumer) - 1);

    if (ioctl(chip_fd, GPIO_V2_GET_LINE_IOCTL, &req) < 0) {
        perror("Erreur configuration IRQ DIO0");
        close(chip_fd);
        exit(EXIT_FAILURE);
    }

    /* le chip_fd n'est plus nécessaire une fois la requête de ligne obtenue,
       le kernel a créé un fd dédié pour la ligne (et ses events) dans req.fd */
    close(chip_fd);
    dio0_line_fd = req.fd;
}

/* Bloque jusqu'au prochain front montant sur DIO0 (RxDone) */
int dio0_wait_irq(void)
{
    struct gpio_v2_line_event event;
    ssize_t n = read(dio0_line_fd, &event, sizeof(event));

    if (n != sizeof(event)) {
        perror("Erreur lecture event DIO0");
        return -1;
    }
    return 0;  /* event.id == GPIO_V2_LINE_EVENT_RISING_EDGE, event.timestamp_ns le temps kernel */
}

void dio0_irq_close(void)
{
    if (dio0_line_fd >= 0)
        close(dio0_line_fd);
}

void spi_init() {
    uint8_t mode = SPI_MODE_0;
    uint8_t bits = 8;
    uint32_t speed = SPI_SPEED;

    spi_fd = open(SPI_DEVICE, O_RDWR);
    if (spi_fd < 0) {
        perror("Erreur ouverture SPI");
        exit(EXIT_FAILURE);
    }

    if (ioctl(spi_fd, SPI_IOC_WR_MODE, &mode) < 0 ||
        ioctl(spi_fd, SPI_IOC_WR_BITS_PER_WORD, &bits) < 0 ||
        ioctl(spi_fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed) < 0) {
        perror("Erreur configuration SPI");
        close(spi_fd);
        exit(EXIT_FAILURE);
    }
}

// ==========================
//  READ REGISTER
// ==========================

uint8_t readReg(uint8_t addr) {
//    gpio_set_cs(0);
    uint8_t tx_buf[2] = {addr & 0x7F, 0xFF};
    uint8_t rx_buf[2] = {0, 0};
    uint8_t spi_bits_per_word = 8;
    uint32_t spi_speed = SPI_SPEED;

    struct spi_ioc_transfer spi_read_msg = {
            .tx_buf = (unsigned long)tx_buf,
            .rx_buf = (unsigned long)rx_buf,
            .len = 2,
            .speed_hz = spi_speed,
            .bits_per_word = spi_bits_per_word,
            .delay_usecs = 0
   };

    if (ioctl(spi_fd, SPI_IOC_MESSAGE(1), &spi_read_msg) < 1) {
        perror("Erreur transmission SPI");
        close(spi_fd);
        exit(EXIT_FAILURE);
    }
  //  gpio_set_cs(1);

    return rx_buf[1];
}

// ==========================
//  WRITE REGISTER
// ==========================

void writeReg(uint8_t reg, uint8_t value) {

//    gpio_set_cs(0);
    uint8_t tx[2] = { reg | 0x80, value };
    uint8_t rx[2] = {0, 0};

    struct spi_ioc_transfer spi_write_msg = {
        .tx_buf = (unsigned long)tx,
        .rx_buf = (unsigned long)rx,
        .len = 2,
        .speed_hz = SPI_SPEED,
        .bits_per_word = 8,
        .delay_usecs = 0
  };

    if (ioctl(spi_fd, SPI_IOC_MESSAGE(1), &spi_write_msg) < 1) {
        perror("Erreur transmission SPI");
        exit(EXIT_FAILURE);
    }
  //  gpio_set_cs(1);
    usleep(1000);
}

void opmode (uint8_t mode) {
    writeReg(RF_OPMODE, (readReg(RF_OPMODE) & ~RF_OP_MD) | mode);
}

// ==========================
//  MODULE INIT
// ==========================

void LoraInit(void) {
        uint8_t reg;

        opmode(RF_OP_MD_SLEEP);
        reg = readReg (RF_OPMODE);
        while ( ( reg & RF_OP_MD ) != RF_OP_MD_SLEEP ) {
        reg = readReg( RF_OPMODE );
        }

        reg &= ~( RF_OP_FREQ | RF_OP_MOD );
        reg |=  ( RF_OP_FREQ_HF | RF_OP_MOD_LORA );
        writeReg(RF_OPMODE, reg );
        reg = readReg( RF_OPMODE );

        opmode(RF_OP_MD_STDBY);
        reg = readReg( RF_OPMODE );
        while ( ( reg & RF_OP_MD ) != RF_OP_MD_STDBY ) {
        reg = readReg( RF_OPMODE );
        }

        uint8_t channel = 0;
        writeReg(RF_FREQ_MSB, eu863_lora_frequency[channel][0]);
        writeReg(RF_FREQ_MID, eu863_lora_frequency[channel][1]);
        writeReg(RF_FREQ_LSB, eu863_lora_frequency[channel][2]);

        // Clear all interrupt request flags
        writeReg( RF_IRQ_FLAGS, 0xFF );

        // set bandwidth to 125 khz and coding rate to 4/8
        reg = 0x00;
        reg |= 0b01111000;
        writeReg( RegModemConfig1, reg );

        // Spreading factor 12
        reg = 0x00;
        reg |= 0b11000000;
        writeReg( RegModemConfig2, reg );

}

// ==========================
//  LORA TX
// ==========================

void LoraTx() {
	uint8_t reg;
        uint8_t pw = 0;       // output power 2db
        writeReg(RegPaConfig, (uint8_t)(0x80|(pw&0xf)));
        writeReg(RegPaDac, readReg(RegPaDac)|0x4);


        writeReg( RF_FIFO_TXBASE, 0x00 );

        writeReg( RF_TX_LEN, NUMBCHAR );

while( 1 ) {
        writeReg( RF_FIFO_ADDR, 0x00 );

        char mychar[] = MESSAGE;
        int len = 0;
        while (len <= NUMBCHAR) {
        writeReg( RF_FIFO, mychar[len]);
        len++;
        }
      // Enter 'transmit' mode.
      reg = readReg( RF_OPMODE );
      reg &= ~( RF_OP_MD );
      reg |=  ( RF_OP_MD_TX );
      writeReg( RF_OPMODE, reg );
      // Wait for the device to finish and enter 'standby' mode.
      reg = readReg( RF_OPMODE );
      while( ( reg & RF_OP_MD ) != RF_OP_MD_STDBY ) {
        reg = readReg( RF_OPMODE );
        usleep( 1000 );
      }
      // Read the interrupt status, then clear the 'TX done' bit.
      reg = readReg( RF_IRQ_FLAGS );
      printf( "ReqIrqFlag: 0x%02X\r\n", reg );
      writeReg( RF_IRQ_FLAGS, RF_IRQ_TX_DONE );
      // Sleep for 10 seconds before transmitting again.
      printf( "TX done.\r\n" );
      sleep(1);
}

}

// ==========================
//  LORA RX
// ==========================

void LoraRx(void) {

    uint8_t reg;

    // Set the base FIFO address to 0.
    writeReg( RF_FIFO_RXBASE, 0x00 );

    // Enter 'continuous receive' mode.
    reg = readReg( RF_OPMODE );
    reg &= ~( RF_OP_MD );
    reg |=  ( RF_OP_MD_RXC );
    writeReg( RF_OPMODE, reg );

dio0_irq_init();

   while( 1 ) {
	dio0_wait_irq(); 
/*
      reg = readReg( RF_IRQ_FLAGS );
      while( ( reg & ( RF_IRQ_RX_DONE | RF_IRQ_CRC_ERROR ) ) == 0 ) {
        reg = readReg( RF_IRQ_FLAGS );
        printf( "ReqIrqFlag: 0x%02X\r\n", reg );
        // Check receive status every half-second or so.
        sleep( 1 );
        }
*/
      // Clear all interrupt request flags.
      writeReg( RF_IRQ_FLAGS, 0xFF );
      // Set FIFO address to the start of received data.
      reg = readReg( RF_FIFO_RXADDR );
      printf( "RX@: 0x%02X\r\n", reg );
      writeReg( RF_FIFO_ADDR, reg );
      // Check how many bytes of data were received.
      uint8_t rx_len = readReg( RF_RX_LEN );
      printf( "RXL: 0x%02X\r\n", rx_len );
      // Read data out of the FIFO and print it.
      uint8_t i;
      for ( i = 0; i < rx_len; ++i ) {
        reg = readReg( RF_FIFO );
        printf("%c", reg);
        }
        printf("\r\n");
        uint8_t r = readReg(LORARegRssiValue);
        int rssiRaw = (int)r;
//        int RSSI_dBm = calculateRSSI(rssiRaw);
//        printf("RSSI: %03d\r\n", RSSI_dBm);
        printf("RSSI: 0x%02X\r\n", r);
	printf("========================\r\n");
        }
}

// ==========================
//  LORA ID
// ==========================

void LoraID(void) {
while(1) {
    uint8_t val = readReg(RF_ID);

    printf("Valeur lue du registre 0x%02X: 0x%02X\n", 0X42, val);
        sleep(2);
}
    close(spi_fd);
}

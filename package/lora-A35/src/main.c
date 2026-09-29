#include "spi_lora.h"

int main() {

//	gpio_init();
	spi_init();
	LoraInit();
        LoraRx();
//	LoraID();  
      	return 0;
}

// opr1-quickstart1 
// Quick start for Orbital Platform R1 
// TCS 202304. Yume Research & Friends. 
// The above attribution must be included regardless of license. 
// This is not permissively licensed software. 

#include <stm32l476xx.h>

void nop(int nop_loops) {
	for (int i = 0; i < nop_loops; i++) {__NOP();}
}

int current_core_freq_MHz = 0;
void init_clocks() {
	// read chip identification information. only useful when watching memory through debugger 
	volatile int id_w_x = (*(uint32_t*)(UID_BASE) & 0xFFFF0000) >> 16;
	volatile int id_w_y = *(uint32_t*)(UID_BASE) & 0xFFFF;
	volatile int id_w_n = *(uint32_t*)(UID_BASE+0x4) & 0xFF;
	volatile int id_lot_l = *(uint32_t*)(UID_BASE+0x4) & 0xFFFFFF00;
	volatile int id_lot_h = *(uint32_t*)(UID_BASE+0x8);
	volatile char id_lot[7] = {
		(id_lot_l >> 8) & 0xFF,
		(id_lot_l >> 16) & 0xFF,
		(id_lot_l >> 24) & 0xFF,
		(id_lot_h) & 0xFF,
		(id_lot_h >> 8) & 0xFF,
		(id_lot_h >> 16) & 0xFF,
		(id_lot_h >> 24) & 0xFF
	};
	RCC->AHB2ENR = 0x000530FF; // enable clock to all gpio banks, usb otg, adc, crypto 
	RCC->AHB3ENR = 0x00000100; // enable clock to QSPI 
	RCC->APB1ENR1 = 
		1 << 28 // enable power control 
		| 1 << 22 // enable i2c2 
		| 1 << 15 // enable spi3 
		| 1 << 14 // enable spi2 
		| 1 << 10; // enable rtc apb 
	RCC->APB2ENR = 1; // enable syscfg 
	FLASH->ACR = 0x600 | 4 | 1<<8; // flash access latency to four wait states, prefetch enable lol 
	//FLASH->ACR = 0x604; // OVERCLOCK! for testing only 
	RCC->CR |= 1 << 8; // enable HSI 
	RCC->PLLCFGR = 
		0x0 << 25 // PLLR = 2 
		| 1 << 24 // PLLREN = 1 
		| 0x0 << 21 // PLLQ = 2 
		| 1 << 20 // PLLQEN (PLL48M1CLK) output enabled 
		| 0x0 << 17 // PLLP default 
		| 0 << 16 // PLLSAI3 not enabled  
		 | 10 << 8 // PLLN = 10 
	//	| 20 << 8 // OVERCLOCK! for testing only 
		| 0 << 4 // PLLM = 1 
		| 0x2; // HSI16 as input clock to PLLs 
	RCC->PLLSAI1CFGR = 
		0x1 << 25 // PLLSAI1R = 4 
		| 1 << 24 // PLLSAI1REN = PLLADC1CLK enable 
		| 0x1 << 21 // PLLSAI1Q = 4
		| 1 << 20 // PLLSAI1QEN = PLL48M2CLK enable
		| 0x0 << 17 // PLLSAI1P = default 
		| 0 << 16 // PLLSAI1PEN = 0
		| 12 << 8; // PLLSAI1N = 12 
	RCC->CR |= (1 << 24) | (1 << 26); // enable PLL and PLLSAI1 
	while (!(RCC->CR & (1 << 25))); // wait for PLL lock 
	while (!(RCC->CR & (1 << 27))); // wait for PLLSAI1 lock 
	RCC->CFGR = 0x3; // system clock to PLL 
	current_core_freq_MHz = 80;
}

int systick_time = 0; 
int heartbeat_counter = 0;
void SysTick_Handler() {
	systick_time++;
	
	// use d7 to d0 to display system time 
	GPIOD->ODR = (GPIOD->ODR & 0xFFFFFF00) | (systick_time >> 4) & 0xFF; 
	
	// heartbeat led 
	if (!(systick_time % 1000)) {
		heartbeat_counter = 100;
	}
	if (heartbeat_counter) {
		GPIOE->BSRR = 1 << 2; 
		heartbeat_counter--;
	} else {
		GPIOE->BSRR = 1 << (2+16); 
	}
}

void init_nvic() {
	__disable_irq();
	SysTick->LOAD = current_core_freq_MHz/8*1000; // configure for 1 ms period, use AHB/8 
	SysTick->CTRL = 0x3; // use AHB/8 as input clock, enable interrupts and counter 
	NVIC_EnableIRQ(SysTick_IRQn);
	__enable_irq();
}

void init_gpio() {
	PWR->CR2 |= 1 << 9; // enable VDDIO2 supply for GPIOG 
	// wait until each GPIO is clocked and ready 
	while (GPIOA->OTYPER == 0xFFFFFFFF); 
	while (GPIOB->OTYPER == 0xFFFFFFFF); 
	while (GPIOC->OTYPER == 0xFFFFFFFF); 
	while (GPIOD->OTYPER == 0xFFFFFFFF); 
	while (GPIOE->OTYPER == 0xFFFFFFFF); 
	while (GPIOF->OTYPER == 0xFFFFFFFF); 
	while (GPIOG->OTYPER == 0xFFFFFFFF); 
	// OP R1 gpio pinout 
	// 		D7 to D0 	D7 to D0 
	// 		HEARTBEAT 	E2 
	// 		A 			G11 
	// 		B 			G12 
	// 		C 			G9 
	// 		FAULT		G7 
	// 		AG 			G6 
	// 		BTN1 		B11 
	// 		BTN0 		B10 
	GPIOB->MODER = 0x00000000; // digital input for btn 0 and 1 
	GPIOB->PUPDR = 0x1 << (10*2) | 0x1 << (11*2); // pull up for btn0 and 1 
	GPIOD->MODER = 0x00005555; // leds D7 to D0 
	GPIOE->MODER = 0x1 << (2*2); // led 
	GPIOG->MODER = 0x01445000; // leds 
}

void gpio_set(GPIO_TypeDef * port, int pin, int value) {
	if (value) {
		port->BSRR = 1 << pin; 
	} else {
		port->BSRR = 1 << (pin+16); 
	}
}

int gpio_read(GPIO_TypeDef * port, int pin) {
	if (port->IDR & (1 << pin)) {
		return 1;
	} else {
		return 0;
	}
}

void gpio_high(GPIO_TypeDef * port, int pin) {
	port->BSRR = 1 << pin; 
}

void gpio_low(GPIO_TypeDef * port, int pin) {
	port->BSRR = 1 << (pin+16);
}

void op_led_a(int value) {
	gpio_set(GPIOG, 11, value);
}

void op_led_b(int value) {
	gpio_set(GPIOG, 12, value);
}

void op_led_c(int value) {
	gpio_set(GPIOG, 9, value);
}

void op_led_fault(int value) {
	gpio_set(GPIOG, 7, value);
}

void op_led_ag(int value) {
	gpio_set(GPIOG, 6, value);
}

void op_led_hb(int value) {
	gpio_set(GPIOE, 2, value);
}

void op_led_dx(int pin, int value) {
	if (pin & 0xFFFFFF00) return; 
	gpio_set(GPIOD, pin, value);
}

void init_i2c2() {
	//GPIOF->MODER = (1 << 0*2) | (1 << 1*2); // bs mode 
	GPIOF->ODR = 0x0;
	

	 
	GPIOF->OTYPER = (1 << 0) | (1 << 1); // open drain 
	GPIOF->OSPEEDR = (3 << 0*2) | (3 << 1*2); // very high speed 
	//GPIOF->PUPDR = (1 << 0*2) | (1 << 1*2); // also use internal pullups (extern is proper) 
	GPIOF->AFR[0] = (4 << 0*4) | (4 << 1*4); // select af 
	
	I2C2->CR1 = 0; // reset 
	//I2C2->TIMINGR = 0x00702991; // generated by cubemx for i2c fast mode 400 kHz 
	I2C2->TIMINGR = 0x10909CEC; // for standard mode 100 kHz 
	I2C2->CR2 = 
		0 << 11 // ADD10 = 0, 7 bit addressing mode 
		| (0 << 1) & 0x3FF // outgoing slave address. dummy with 0 
		| 0 << 10 // RD_WRN: 1 request read transfer, 0 request write transfer 
		| (0 & 0xFF) << 16 // number of bytes to be transferred. dummy with 0 
		| 0 << 25; // autoend = disabled  
	I2C2->CR1 = 1 | 0xf << 8;; // peripheral enable 
	GPIOF->MODER = (2 << 0*2) | (2 << 1*2); // AF mode
}

void i2c2_sig_start() {
	I2C2->CR2 |= 1 << 13; // generate START condition 
}

void i2c2_sig_stop() {
	I2C2->CR2 |= 1 << 14; // generate STOP condition 
}

void i2c2_tx(int addr, int *write_buf, int write_num, int *read_buf, int read_num) { 
	if (write_num) {
		I2C2->CR2 = 
			(I2C2->CR2 & 0xFF00FC00) // mask away addr and num of bytes bitfields 
			| (addr & 0x3FF) // set outgoing slave address 
			| 0 << 10 // set tx direction to write 
			| (write_num & 0xFF) << 16; // set number of bytes 
		if (read_num) {
			I2C2->CR2 |= 1<<25; // autoend disable 
		} else {
			I2C2->CR2 &= ~(1<<25); // autoend enable 
		}
		i2c2_sig_start();
		while (write_num--) {
			//while (!(I2C2->ISR & (1 << 1))); // while not TXIS 
			while (!(I2C2->ISR & 1)); // while not TXE 
			I2C2->TXDR = *write_buf++;
		}
	}
	if (read_num) {
		//while (!(I2C2->ISR & 1 << 6)); // while not TC 
		while (!(I2C2->ISR & 1)); // while not TXE 
		I2C2->CR2 = 
			(I2C2->CR2 & 0xFF00FC00) // mask away addr and num of bytes bitfields 
			| (addr & 0x3FF) // set outgoing slave address 
			| 1 << 10 // set tx direction to read 
			| (read_num & 0xFF) << 16; // set number of bytes 
		i2c2_sig_start();
		I2C2->CR2 &= ~(1<<25); // autoend enable 
		for (int i = 0; i < read_num; i++) {
			while (I2C2->ISR & (1 << 2)); // while RXNE 
			read_buf[i] = I2C2->RXDR & 0xFF; 
		}
	}
}

int main() {
	init_clocks();
	init_nvic();
	init_gpio();
	init_i2c2();

	
	nop(1000);

	int lol[10] = {0x0F, 0, 0};
	int lmao; 
//	i2c2_tx(0xD4, &lol, 1, &lmao, 1);

	int lola[10] = {0x0b, 0x01}; 
	i2c2_tx(0x0D<<1, &lola, 2, &lmao, 0);
	i2c2_sig_stop();
	int lolb[10] = {0x09, 0x1d};
	i2c2_tx(0x0D<<1, &lolb, 2, &lmao, 0);

	int lolc[10] = {0x07};
	i2c2_tx(0x0D<<1, &lolc, 1, &lmao, 2);

	I2C2->CR2 = 
			(I2C2->CR2 & 0xFF00FC00) // mask away addr and num of bytes bitfields 
			| (0x0D<<1 & 0x3FF) // set outgoing slave address 
			| 1 << 10 // set tx direction to read 
			| (1 & 0xFF) << 16; // set number of bytes 
		i2c2_sig_start();
		I2C2->CR2 &= ~(1<<25); // autoend enable 
		for (int i = 0; i < 1; i++) {
			while (I2C2->ISR & (1 << 2)); // while RXNE 
			int a  = I2C2->RXDR & 0xFF; 
		}

		i2c2_tx(0x0D<<1, &lolc, 1, &lmao, 2);

	//GPIOD->ODR = (GPIOD->ODR & 0xFFFFFF00) | I2C2->RXDR & 0xFF;
	

	while(1) { // blinky 
		op_led_c(!gpio_read(GPIOB, 11));
	}
}






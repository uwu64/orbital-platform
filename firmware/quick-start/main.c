// opr1-quickstart1 
// Quick start for Orbital Platform R1 
// TCS 202304. Yume Research & Friends. 
// The above attribution must be included regardless of license. 
// This is not permissively licensed software. 

#include <stm32l476xx.h>

void nop(int nop_loops) {
	for (int i = 0; i < nop_loops; i++) {__NOP();}
}

int core_MHz = 0;
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
	core_MHz = 80;
}

int systick_time = 0; 
int heartbeat_counter = 0;
void SysTick_Handler() {
	systick_time++;
	
	// use d7 to d0 to display system time 
//	GPIOD->ODR = (GPIOD->ODR & 0xFFFFFF00) | (systick_time >> 4) & 0xFF; 
	
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
	SysTick->LOAD = core_MHz/8*1000; // configure for 1 ms period, use AHB/8 
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

void softi2c_init_pins(GPIO_TypeDef * scl_port, int scl_pin, GPIO_TypeDef * sda_port, int sda_pin) {
	scl_port->OTYPER = (scl_port->OTYPER & ~(1 << scl_pin)) | (1 << scl_pin); // open drain 
	sda_port->OTYPER = (sda_port->OTYPER & ~(1 << sda_pin)) | (1 << sda_pin); 
	scl_port->OSPEEDR = (scl_port->OSPEEDR & ~(0x3 << (scl_pin*2))) | (3 << scl_pin*2); // very high speed 
	sda_port->OSPEEDR = (sda_port->OSPEEDR & ~(0x3 << (sda_pin*2))) | (3 << sda_pin*2); 
	scl_port->BSRR = 1 << scl_pin; // default high 
	sda_port->BSRR = 1 << sda_pin;
	scl_port->MODER = (scl_port->MODER & ~(0x3 << (scl_pin*2))) | (1 << scl_pin*2); // output 
	sda_port->MODER = (sda_port->MODER & ~(0x3 << (sda_pin*2))) | (1 << sda_pin*2); 
}

void softi2c_delay() {
	nop(30);
}

void softi2c_sig_start(GPIO_TypeDef * scl_port, int scl_pin, GPIO_TypeDef * sda_port, int sda_pin) {
	gpio_set(scl_port, scl_pin, 1);
	gpio_set(sda_port, sda_pin, 1);
	softi2c_delay(); softi2c_delay();
	gpio_set(sda_port, sda_pin, 0);
	softi2c_delay(); softi2c_delay();
	gpio_set(scl_port, scl_pin, 0);
	softi2c_delay(); 
}

void softi2c_sig_repeated_start(GPIO_TypeDef * scl_port, int scl_pin, GPIO_TypeDef * sda_port, int sda_pin) {
	gpio_set(scl_port, scl_pin, 0);
	gpio_set(sda_port, sda_pin, 1);
	softi2c_delay();
	gpio_set(scl_port, scl_pin, 1);
	softi2c_delay(); softi2c_delay();
	gpio_set(sda_port, sda_pin, 0);
	softi2c_delay(); softi2c_delay();
	gpio_set(scl_port, scl_pin, 0);
	softi2c_delay(); 
}

void softi2c_sig_stop(GPIO_TypeDef * scl_port, int scl_pin, GPIO_TypeDef * sda_port, int sda_pin) {
	gpio_set(scl_port, scl_pin, 0);
	gpio_set(sda_port, sda_pin, 0);
	softi2c_delay(); softi2c_delay();
	gpio_set(scl_port, scl_pin, 1);
	softi2c_delay(); softi2c_delay();
	gpio_set(sda_port, sda_pin, 1);
	softi2c_delay(); 
}

void softi2c_send8(GPIO_TypeDef * scl_port, int scl_pin, GPIO_TypeDef * sda_port, int sda_pin, int data8) {
	for (int i = 0; i < 8; i++) {
		gpio_set(scl_port, scl_pin, 0);
		softi2c_delay();
		gpio_set(sda_port, sda_pin, (data8 << i) & (1 << 7));
		softi2c_delay(); 
		gpio_set(scl_port, scl_pin, 1);
		softi2c_delay(); softi2c_delay();
	}
}

int softi2c_read8(GPIO_TypeDef * scl_port, int scl_pin, GPIO_TypeDef * sda_port, int sda_pin) {
	int data = 0; 
	gpio_set(sda_port, sda_pin, 1);
	for (int i = 0; i < 8; i++) {
		gpio_set(scl_port, scl_pin, 0);
		softi2c_delay();
		data |= gpio_read(sda_port, sda_pin) << (7-i);
		softi2c_delay(); 
		gpio_set(scl_port, scl_pin, 1);
		softi2c_delay(); softi2c_delay();
	}
	return data;
}

void softi2c_send_nack(GPIO_TypeDef * scl_port, int scl_pin, GPIO_TypeDef * sda_port, int sda_pin) {
	gpio_set(scl_port, scl_pin, 0);
	softi2c_delay();
	gpio_set(sda_port, sda_pin, 1);
	softi2c_delay(); 
	gpio_set(scl_port, scl_pin, 1);
	softi2c_delay(); 
}

int softi2c_read_nack(GPIO_TypeDef * scl_port, int scl_pin, GPIO_TypeDef * sda_port, int sda_pin) {
	gpio_set(scl_port, scl_pin, 0);
	softi2c_delay();
	gpio_set(sda_port, sda_pin, 1);
	softi2c_delay(); 
	gpio_set(scl_port, scl_pin, 1);
	softi2c_delay(); 
	int nack = gpio_read(sda_port, sda_pin);
	softi2c_delay();
	return nack; 
}

int softi2c_reg_write(GPIO_TypeDef * scl_port, int scl_pin, GPIO_TypeDef * sda_port, int sda_pin, int device_addr, int reg_addr, int data) {
	int nack = 0; 
	softi2c_sig_start(scl_port, scl_pin, sda_port, sda_pin);
	softi2c_send8(scl_port, scl_pin, sda_port, sda_pin, device_addr << 1 | 0);
	nack += softi2c_read_nack(scl_port, scl_pin, sda_port, sda_pin);
	softi2c_send8(scl_port, scl_pin, sda_port, sda_pin, reg_addr);
	nack += softi2c_read_nack(scl_port, scl_pin, sda_port, sda_pin);
	softi2c_send8(scl_port, scl_pin, sda_port, sda_pin, data);
	nack += softi2c_read_nack(scl_port, scl_pin, sda_port, sda_pin);
	softi2c_sig_stop(scl_port, scl_pin, sda_port, sda_pin);
	return nack; 
}

int softi2c_reg_read(GPIO_TypeDef * scl_port, int scl_pin, GPIO_TypeDef * sda_port, int sda_pin, int device_addr, int reg_addr) {
	int nack = 0; 
	softi2c_sig_start(scl_port, scl_pin, sda_port, sda_pin);
	softi2c_send8(scl_port, scl_pin, sda_port, sda_pin, device_addr << 1 | 0);
	nack += softi2c_read_nack(scl_port, scl_pin, sda_port, sda_pin);
	softi2c_send8(scl_port, scl_pin, sda_port, sda_pin, reg_addr);
	nack += softi2c_read_nack(scl_port, scl_pin, sda_port, sda_pin);
	softi2c_sig_repeated_start(scl_port, scl_pin, sda_port, sda_pin);
	softi2c_send8(scl_port, scl_pin, sda_port, sda_pin, device_addr << 1 | 1);
	nack += softi2c_read_nack(scl_port, scl_pin, sda_port, sda_pin);
	int data = softi2c_read8(scl_port, scl_pin, sda_port, sda_pin);
	softi2c_send_nack(scl_port, scl_pin, sda_port, sda_pin);
	softi2c_sig_stop(scl_port, scl_pin, sda_port, sda_pin);
	return data;
}

void init_adc() {
	GPIOA->MODER |= 0x3 << (1*2);
	
}

#define OB_I2C2 GPIOF, 1, GPIOF, 0
#define IMU_ADDR 0x6A 
#define MAG_ADDR 0x0D 

int main() {
	init_clocks();
	init_nvic();
	init_gpio();
	softi2c_init_pins(OB_I2C2);
	
	softi2c_reg_write(OB_I2C2, MAG_ADDR, 0x0B, 0x01);  
	softi2c_reg_write(OB_I2C2, MAG_ADDR, 0x09, 0x1D); 
	softi2c_reg_read(OB_I2C2, MAG_ADDR, 0x08); // temp high byte 
	softi2c_reg_read(OB_I2C2, MAG_ADDR, 0x07); // temp low byte 

	// while(1) { 
	// 	op_led_c(!gpio_read(GPIOB, 11));
	// 	nop(10000);
	// 	int temp = (softi2c_reg_read(OB_I2C2, MAG_ADDR, 0x08) << 8) | softi2c_reg_read(OB_I2C2, MAG_ADDR, 0x07);
	// 	GPIOD->ODR = (GPIOD->ODR & 0xFFFFFF00) | (temp>>3) & 0xFF;
	// }

	softi2c_reg_write(OB_I2C2, IMU_ADDR, 0x12, 0x01); // soft reset imu  
	softi2c_reg_write(OB_I2C2, IMU_ADDR, 0x10, 0xA0); // imu acel start 
	softi2c_reg_write(OB_I2C2, IMU_ADDR, 0x11, 0x10); // imu gyro start 
	softi2c_reg_read(OB_I2C2, IMU_ADDR, 0x29); // acel x high 
	softi2c_reg_read(OB_I2C2, IMU_ADDR, 0x28); // acel x low 

	while(1) { // blinky 
		op_led_c(!gpio_read(GPIOB, 11));
		nop(100000);
		GPIOD->ODR = (GPIOD->ODR & 0xFFFFFF00) | (softi2c_reg_read(OB_I2C2, IMU_ADDR, 0x29)) & 0xFF;
	}
}






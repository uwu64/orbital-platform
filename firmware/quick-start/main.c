// opr1-quickstart1 
// Quick start for Orbital Platform R1 
// TCS 202304. Yume Research & Friends. 
// The above attribution must be included regardless of license. 
// This is not permissively licensed software. 

#include <stm32l476xx.h>
#include <math.h> // for sqrt 
typedef enum { false, true } bool;

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

void gpio_high(GPIO_TypeDef * port, int pin) {
	port->BSRR = 1 << pin;
}

void gpio_low(GPIO_TypeDef * port, int pin) {
	port->BSRR = 1 << (pin+16);
}

void gpio_set(GPIO_TypeDef * port, int pin, int value) {
	if (value) {
		gpio_high(port, pin);
	} else {
		gpio_low(port, pin);
	}
}

int gpio_read(GPIO_TypeDef * port, int pin) {
	if (port->IDR & (1 << pin)) {
		return 1;
	} else {
		return 0;
	}
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

int systick_time = 0; 
int heartbeat_counter = 0;
int ag_counter = 0;
void SysTick_Handler() {
	systick_time++;
	
	// use d7 to d0 to display system time 
//	GPIOD->ODR = (GPIOD->ODR & 0xFFFFFF00) | (systick_time >> 4) & 0xFF; 
	
	// heartbeat led 
	if (!(systick_time % 1000)) {
		heartbeat_counter = 100;
	}
	if (!(systick_time % 250)) {
		ag_counter = 10;
	}
	if (heartbeat_counter) {
		op_led_hb(1);
		heartbeat_counter--;
	} else {
		op_led_hb(0);
	}
	if (ag_counter) {
		op_led_ag(1);
		ag_counter--;
	} else {
		op_led_ag(0);
	}
}

void init_nvic() {
	__disable_irq();
	SysTick->LOAD = core_MHz/8*1000; // configure for 1 ms period, use AHB/8 
	SysTick->CTRL = 0x3; // use AHB/8 as input clock, enable interrupts and counter 
	NVIC_EnableIRQ(SysTick_IRQn);
	__enable_irq();
}

void softi2c_line_mode(GPIO_TypeDef * port, int pin, bool deassert_line) {
	if (deassert_line) {
		port->MODER &= ~(0x3 << (pin*2)); 
	} else {
		port->MODER = (port->MODER & ~(0x3 << (pin*2))) | (1 << pin*2); 
	}
}

void init_softi2c(GPIO_TypeDef * scl_port, int scl_pin, GPIO_TypeDef * sda_port, int sda_pin) {
	scl_port->OTYPER = (scl_port->OTYPER & ~(1 << scl_pin)) | (0 << scl_pin); // push pull 
	sda_port->OTYPER = (sda_port->OTYPER & ~(1 << sda_pin)) | (0 << sda_pin); 
	scl_port->OSPEEDR = (scl_port->OSPEEDR & ~(0x3 << (scl_pin*2))) | (3 << scl_pin*2); // very high speed 
	sda_port->OSPEEDR = (sda_port->OSPEEDR & ~(0x3 << (sda_pin*2))) | (3 << sda_pin*2); 
	scl_port->BSRR = 1 << (scl_pin + 16); // drive low when asserted 
	sda_port->BSRR = 1 << (sda_pin + 16);
	softi2c_line_mode(scl_port, scl_pin, 1); 
	softi2c_line_mode(sda_port, sda_pin, 1); 
}

void softi2c_delay() {
	nop(30);
}

void softi2c_sig_start(GPIO_TypeDef * scl_port, int scl_pin, GPIO_TypeDef * sda_port, int sda_pin) {
	softi2c_line_mode(scl_port, scl_pin, 1);
	softi2c_line_mode(sda_port, sda_pin, 1);
	softi2c_delay(); softi2c_delay();
	softi2c_line_mode(sda_port, sda_pin, 0);
	softi2c_delay(); softi2c_delay();
	softi2c_line_mode(scl_port, scl_pin, 0);
	softi2c_delay(); 
}

void softi2c_sig_repeated_start(GPIO_TypeDef * scl_port, int scl_pin, GPIO_TypeDef * sda_port, int sda_pin) {
	softi2c_line_mode(scl_port, scl_pin, 0);
	softi2c_line_mode(sda_port, sda_pin, 1);
	softi2c_delay();
	softi2c_line_mode(scl_port, scl_pin, 1);
	softi2c_delay(); softi2c_delay();
	softi2c_line_mode(sda_port, sda_pin, 0);
	softi2c_delay(); softi2c_delay();
	softi2c_line_mode(scl_port, scl_pin, 0);
	softi2c_delay(); 
}

void softi2c_sig_stop(GPIO_TypeDef * scl_port, int scl_pin, GPIO_TypeDef * sda_port, int sda_pin) {
	softi2c_line_mode(scl_port, scl_pin, 0);
	softi2c_line_mode(sda_port, sda_pin, 0);
	softi2c_delay(); softi2c_delay();
	softi2c_line_mode(scl_port, scl_pin, 1);
	softi2c_delay(); softi2c_delay();
	softi2c_line_mode(sda_port, sda_pin, 1);
	softi2c_delay(); 
}

void softi2c_send8(GPIO_TypeDef * scl_port, int scl_pin, GPIO_TypeDef * sda_port, int sda_pin, int data8) {
	for (int i = 0; i < 8; i++) {
		softi2c_line_mode(scl_port, scl_pin, 0);
		softi2c_delay();
		softi2c_line_mode(sda_port, sda_pin, (data8 << i) & (1 << 7));
		softi2c_delay(); 
		softi2c_line_mode(scl_port, scl_pin, 1);
		softi2c_delay(); softi2c_delay();
	}
}

int softi2c_read8(GPIO_TypeDef * scl_port, int scl_pin, GPIO_TypeDef * sda_port, int sda_pin) {
	int data = 0; 
	softi2c_line_mode(sda_port, sda_pin, 1);
	for (int i = 0; i < 8; i++) {
		softi2c_line_mode(scl_port, scl_pin, 0);
		softi2c_delay();
		data |= gpio_read(sda_port, sda_pin) << (7-i);
		softi2c_delay(); 
		softi2c_line_mode(scl_port, scl_pin, 1);
		softi2c_delay(); softi2c_delay();
	}
	return data;
}

void softi2c_send_nack(GPIO_TypeDef * scl_port, int scl_pin, GPIO_TypeDef * sda_port, int sda_pin) {
	softi2c_line_mode(scl_port, scl_pin, 0);
	softi2c_delay();
	softi2c_line_mode(sda_port, sda_pin, 1);
	softi2c_delay(); 
	softi2c_line_mode(scl_port, scl_pin, 1);
	softi2c_delay(); 
}

int softi2c_read_nack(GPIO_TypeDef * scl_port, int scl_pin, GPIO_TypeDef * sda_port, int sda_pin) {
	softi2c_line_mode(scl_port, scl_pin, 0);
	softi2c_delay();
	softi2c_line_mode(sda_port, sda_pin, 1);
	softi2c_delay(); 
	softi2c_line_mode(scl_port, scl_pin, 1);
	softi2c_delay(); 
	int nack = gpio_read(sda_port, sda_pin);
	softi2c_delay();
	return nack; 
}

int softi2c_write_reg(GPIO_TypeDef * scl_port, int scl_pin, GPIO_TypeDef * sda_port, int sda_pin, int device_addr, int reg_addr, int data) {
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

int softi2c_read_reg(GPIO_TypeDef * scl_port, int scl_pin, GPIO_TypeDef * sda_port, int sda_pin, int device_addr, int reg_addr) {
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

bool softi2c_probe(GPIO_TypeDef * scl_port, int scl_pin, GPIO_TypeDef * sda_port, int sda_pin, int device_addr) {
	int nack = 0; 
	softi2c_sig_start(scl_port, scl_pin, sda_port, sda_pin);
	softi2c_send8(scl_port, scl_pin, sda_port, sda_pin, device_addr << 1 | 1);
	nack += softi2c_read_nack(scl_port, scl_pin, sda_port, sda_pin);
	softi2c_sig_stop(scl_port, scl_pin, sda_port, sda_pin);
	if (nack) {
		return 0;
	} else {
		return 1; 
	}
}

int16_t softi2c_read_reg_hl(GPIO_TypeDef * scl_port, int scl_pin, GPIO_TypeDef * sda_port, int sda_pin, int device_addr, int high_reg_addr, int low_reg_addr) {
	return softi2c_read_reg(scl_port, scl_pin, sda_port, sda_pin, device_addr, high_reg_addr) << 8 | softi2c_read_reg(scl_port, scl_pin, sda_port, sda_pin, device_addr, low_reg_addr);
}

void init_adc() {
	GPIOA->MODER |= 0x3 << (1*2);
	
}

#define OP1_I2C2 GPIOF, 1, GPIOF, 0
#define IMU_ADDR 0x6A 
#define MAG_ADDR 0x0D 

#define IMU_ODR_OFF 		0 
#define IMU_ODR_12_5_Hz 	1 
#define IMU_ODR_26_Hz 		2 
#define IMU_ODR_52_Hz 		3 
#define IMU_ODR_104_Hz 		4 
#define IMU_ODR_208_Hz 		5 
#define IMU_ODR_417_Hz 		6 
#define IMU_ODR_833_Hz 		7 
#define IMU_ODR_1667_Hz 	8 
#define IMU_ODR_3333_Hz 	9 
#define IMU_ODR_6667_Hz 	10 
#define IMU_FS_2_g 			0 
#define IMU_FS_4_g 			2 
#define IMU_FS_8_g 			3 
#define IMU_FS_16_g 		1 
#define IMU_FS_125_dps 		2 
#define IMU_FS_250_dps 		0 
#define IMU_FS_500_dps 		4 
#define IMU_FS_1000_dps 	8 
#define IMU_FS_2000_dps 	12 
#define IMU_FS_4000_dps 	1 

#define MAG_ODR_OFF 		0 
#define MAG_ODR_10_Hz 		1 
#define MAG_ODR_50_Hz 		2 
#define MAG_ODR_100_Hz 		3 
#define MAG_ODR_200_Hz		4 
#define MAG_FS_2_G 			0 // btw: "G" is not a typo, it stands for gauss 
#define MAG_FS_8_G	 		1 
#define MAG_OVERSAMPLE_512 	0 
#define MAG_OVERSAMPLE_256 	1 
#define MAG_OVERSAMPLE_128 	2 
#define MAG_OVERSAMPLE_64  	3 

void op1_imu_acel_ctrl(int acel_rate, int acel_scale, int digital_filter_on) {
	acel_rate &= 0xFF;
	acel_scale &= 0xF;
	digital_filter_on &= 1;
	int data = acel_rate << 4 | acel_scale << 2 | digital_filter_on << 1;
	softi2c_write_reg(OP1_I2C2, IMU_ADDR, 0x10, data); 
}

void op1_imu_gyro_ctrl(int gyro_rate, int gyro_scale) {
	gyro_rate &= 0xFF;
	gyro_scale &= 0xFF; 
	int data = gyro_rate << 4 | gyro_scale;
	softi2c_write_reg(OP1_I2C2, IMU_ADDR, 0x11, data); 
}

void op1_imu_init(int acel_rate, int acel_scale, int gyro_rate, int gyro_scale) {
	softi2c_write_reg(OP1_I2C2, IMU_ADDR, 0x12, 0x01); // soft reset imu  
	nop(100); 
	op1_imu_acel_ctrl(acel_rate, acel_scale, 0);
	op1_imu_gyro_ctrl(gyro_rate, gyro_scale);
} 

int16_t op1_imu_read_acel_x() {
	return softi2c_read_reg_hl(OP1_I2C2, IMU_ADDR, 0x29, 0x28);
}

int16_t op1_imu_read_acel_y() {
	return softi2c_read_reg_hl(OP1_I2C2, IMU_ADDR, 0x2B, 0x2A);
}

int16_t op1_imu_read_acel_z() {
	return softi2c_read_reg_hl(OP1_I2C2, IMU_ADDR, 0x2D, 0x2C);
}

int16_t op1_imu_read_gyro_x() {
	return softi2c_read_reg_hl(OP1_I2C2, IMU_ADDR, 0x23, 0x22);
}

int16_t op1_imu_read_gyro_y() {
	return softi2c_read_reg_hl(OP1_I2C2, IMU_ADDR, 0x25, 0x24);
}

int16_t op1_imu_read_gyro_z() {
	return softi2c_read_reg_hl(OP1_I2C2, IMU_ADDR, 0x27, 0x26);
}

int16_t op1_imu_read_temp() {
	return softi2c_read_reg_hl(OP1_I2C2, IMU_ADDR, 0x21, 0x20);
}

void op1_mag_ctrl(int rate, int scale, int oversample) {
	if (rate) {
		softi2c_write_reg(OP1_I2C2, MAG_ADDR, 0x09, oversample << 6 | scale << 4 | (rate-1) << 2 | 1); 
	} else {
		softi2c_write_reg(OP1_I2C2, MAG_ADDR, 0x09, 0);
	}
}

void op1_mag_init(int rate, int scale, int oversample) {
	softi2c_write_reg(OP1_I2C2, MAG_ADDR, 0x0A, 0x80); // soft reset 
	nop(50); 
	softi2c_write_reg(OP1_I2C2, MAG_ADDR, 0x0A, 0x00); 
	softi2c_write_reg(OP1_I2C2, MAG_ADDR, 0x0B, 1);
	nop(50); 
	op1_mag_ctrl(rate, scale, oversample);
}

int16_t op1_mag_read_x() {
	return softi2c_read_reg_hl(OP1_I2C2, MAG_ADDR, 0x01, 0x00);
}

int16_t op1_mag_read_y() {
	return softi2c_read_reg_hl(OP1_I2C2, MAG_ADDR, 0x03, 0x02);
}

int16_t op1_mag_read_z() {
	return softi2c_read_reg_hl(OP1_I2C2, MAG_ADDR, 0x05, 0x04);
}

int16_t op1_mag_read_temp() {
	return softi2c_read_reg_hl(OP1_I2C2, MAG_ADDR, 0x08, 0x07);
}

unsigned int abs16(int16_t a) {
	if (a < 0) {
		return (a * -1);
	} else {
		return a;
	}
}


// PE13 clock 
// PE12 latch 
// PG4 mosi 
// PF3 PB7 PB6 all go high  
void mo(int val) {
	gpio_set(GPIOG, 4, val);
}

void cl(int val) {
	gpio_set(GPIOE, 13, val);
}

void la(int val) {
	gpio_set(GPIOE, 12, val);
}

void init_lolspi() {
	GPIOB->ODR |= 1 << 7 | 1 << 6;
	GPIOF->ODR |= 1 << 3; 
	la(1);
	cl(0);
	mo(0);
	
	GPIOG->MODER = 0x01 << 4*2;
	GPIOB->MODER = 0x01 << 7*2 | 0x01 << 6*2; 
	GPIOF->MODER = 0x01 << 3*2;
	GPIOE->MODER = 0x01 << 13*2 | 0x01 << 12*2; 
}

void ss() {
	nop(100);
}

void w32(int val) {
	la(0);
	cl(0);
	ss();
	for (int i = 0; i < 32; i++) {
		mo((val>>(31-i)) & 1);
		ss();
		cl(1);
		ss();
		cl(0);
	}
	la(1);
	ss(); ss(); ss(); nop(0xFFFF);
}
int regsa[] = {
  0x10BB140,
  0x575011,  
  0x3DD082,
  0x2B149923,
  0x80493814,
  0x03FF5,
  0x50C78C6,
  0x007,

  0x0008,
  0x231E9,
  0x3296286A,
  0x0003B,
  0x0010C,
  0x0000D,
  0x0000000E,
  0x0000050F
};

int regs[] = {
  0x10bb140,
  0x21b56a1,
  0x533082,
  0x2bfc89a3,
  0x80587a14,
  0x03FF5,
  0x50C78C6,
  0x007,

  0x0008,
  0x231E9,
  0x3296286A,
  0x0003B,
  0x0010C,
  0x0000D,
  0x0000000E,
  0x0000050F
};

int main() {
	init_clocks();
	init_gpio();
	init_nvic();
	init_softi2c(OP1_I2C2);
	
	op1_mag_init(MAG_ODR_200_Hz, MAG_FS_8_G, MAG_OVERSAMPLE_512);
	op1_imu_init(IMU_ODR_3333_Hz, IMU_FS_2_g, IMU_ODR_3333_Hz, IMU_FS_1000_dps);

	int state = 0;
	int btn_debounce = 0;



	while (0) { // radio attempt 
		init_lolspi();
		nop(0xFFF);
		w32(regs[1]); 
    w32(regs[3]);
    w32(regs[0]);
    w32(regs[4]);
    w32(regs[2]); 

    w32(regs[15]);
		nop(0xFFFFFF);nop(0xFFFFFF);nop(0xFFFFFF);nop(0xFFFFFF);nop(0xFFFFFF);nop(0xFFFFFF);nop(0xFFFFFF);

	}



	while(1) { // test program 
		op_led_c(!gpio_read(GPIOB, 11));
		
		if ((!gpio_read(GPIOB, 10))&&(!btn_debounce)) {
			state = (state + 1) % 5;
			btn_debounce = 100;
		} else if (btn_debounce) {
			btn_debounce--;
		}
		
		switch (state) {
			case 0: // imu (tilt)
				GPIOD->ODR = (GPIOD->ODR & 0xFFFFFF00) | (op1_imu_read_acel_x()>>8) & 0xFF;
				op_led_a(1);
				op_led_b(systick_time & 1 << 6);
				break; 
			case 1: // gyro 
				GPIOD->ODR = (GPIOD->ODR & 0xFFFFFF00) | (abs16(op1_imu_read_gyro_z())>>8) & 0xFF;
				op_led_a(0);
				op_led_b(1);
				break; 
			case 2: // noisy gyro.. 
				GPIOD->ODR = (GPIOD->ODR & 0xFFFFFF00) | (abs16(op1_imu_read_gyro_z())) & 0xFF;
				op_led_a(1);
				op_led_b(0);
				break; 
			default: 
				op_led_a(0);
				op_led_b(0);
				break; 
		}
		nop(100000);
	}

	while(1) { // just blinky 
		op_led_c(!gpio_read(GPIOB, 11));
		nop(1000000);
		int16_t ax = op1_imu_read_acel_x();
		int16_t ay = op1_imu_read_acel_y();
		int16_t az = op1_imu_read_acel_z();
		int amag = sqrt(ax^2 + ay^2 + az^2);
		int value;
		value = abs16(op1_imu_read_gyro_z()) >> 8; 
		value = op1_mag_read_x();
		value = op1_mag_read_temp();
		value = value >> 8;
		GPIOD->ODR = (GPIOD->ODR & 0xFFFFFF00) | value & 0xFF;
	}
	
	
	
	
	
	
	
}






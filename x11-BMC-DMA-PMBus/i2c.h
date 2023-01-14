
#ifndef I2C_H
#define I2C_H

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#define I2C_BASE 0x1E78A000 
#define I2C_MMIO_SIZE 0x1000

#define I2C_G0_OFFSET 0x0
#define I2C_G8_OFFSET 0x8

#define I2C_DEV1_OFFSET 0x40
#define I2C_DEV2_OFFSET 0x80
#define I2C_DEV3_OFFSET 0xC0
#define I2C_DEV4_OFFSET 0x100
#define I2C_DEV5_OFFSET 0x140
#define I2C_DEV6_OFFSET 0x180
#define I2C_DEV7_OFFSET 0x1C0
#define I2C_DEV8_OFFSET 0x300
#define I2C_DEV9_OFFSET 0x340
#define I2C_DEV10_OFFSET 0x380
#define I2C_DEV11_OFFSET 0x3C0
#define I2C_DEV12_OFFSET 0x400
#define I2C_DEV13_OFFSET 0x440
#define I2C_DEV14_OFFSET 0x480

// #define BUFFER_POOL_P2_OFFSET 0x200
#define BUFFER_POOL_P0_OFFSET 0x800
#define BUFFER_POOL_P1_OFFSET 0x900
#define BUFFER_POOL_P2_OFFSET 0xA00
#define BUFFER_POOL_P3_OFFSET 0xB00
#define BUFFER_POOL_P4_OFFSET 0xC00
#define BUFFER_POOL_P5_OFFSET 0xD00
#define BUFFER_POOL_P6_OFFSET 0xE00
#define BUFFER_POOL_P7_OFFSET 0xF00

typedef struct I2C_dev
{
	volatile uint32_t D00;
	volatile uint32_t D04;
	volatile uint32_t D08;
	volatile uint32_t D0C;
	volatile uint32_t D10;
	volatile uint32_t D14;
	volatile uint32_t D18;
	volatile uint32_t D1C;
	volatile uint32_t D20;
} I2C_dev_t;

extern volatile uint8_t *i2c_mem;


void sleep_cycle(uint32_t n);
void msleep(uint32_t ms);
void usleep(uint32_t us);
void print_dev(volatile I2C_dev_t *dev);
void config_dev(volatile I2C_dev_t *dev);
void send_byte(volatile I2C_dev_t * dev, uint8_t byte);
void send_bytes(volatile I2C_dev_t * dev, uint8_t byte[], uint32_t len);
void write_read(volatile I2C_dev_t * dev, uint8_t byte[], uint32_t len);


// Max read 32 bits for now, should be enough for our use case
uint32_t i2c_smbus_read_data(volatile I2C_dev_t * dev, 
	const uint8_t addr, const uint8_t command, const uint8_t length);


// Max write 16 bits for now, should be enough for our use case
uint32_t i2c_smbus_write_data(volatile I2C_dev_t * dev, 
	const uint8_t addr, const uint8_t command, const uint8_t *data,
	const uint8_t length);

#endif
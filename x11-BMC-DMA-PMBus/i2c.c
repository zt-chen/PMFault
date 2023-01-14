#include "i2c.h"
void sleep_cycle(uint32_t n)
{
	uint32_t i;
	for (i = 0; i < n; i++)
		__asm__ __volatile__ ("NOP");
}
void msleep(uint32_t ms)
{
	uint32_t i;
	for (i = 0; i < 4000*ms; i++)
		__asm__ __volatile__ ("NOP");
}
void usleep(uint32_t us)
{
	uint32_t i;
	for (i = 0; i < 4*us; i++)
		__asm__ __volatile__ ("NOP");
}
void print_dev(volatile I2C_dev_t *dev) {
	uint32_t *p_tmp = (uint32_t *)dev;
	for (int i = 0; i < 9; i++) {
		printf("D%02x: 0x%08x\n", i*4, *(p_tmp+i));
	}
}

void config_dev(volatile I2C_dev_t *dev) {
	dev->D00 = 0x00000001 | 0x700000 ; // use buffer page 7 0xf00 - 0xfff
	//dev->D04 = 0x77777355; // 100KHz
	// dev->D04 = 0x77755352; // 1000KHz
	dev->D04 = 0x77755353; // 500KHz
	dev->D08 = 0x00000000; // No timeout Control
	dev->D10 = 0xFFFFFFFF; // clearing interrupt status
	dev->D0C = 0x00000000; // Enable interrupts
}

void send_byte_raw(volatile I2C_dev_t * dev, uint8_t byte) {
	// Load byte to register
	dev->D20 = byte & 0xFF;

	// Trigger send (Start -> Tx -> Stop)
	dev->D14 = 0x00000023;

	// dev->D10 = 0xFFFFFFFF;

};

void send_bytes_raw(volatile I2C_dev_t * dev, uint8_t byte[], uint32_t len) {
	// Base pointer 0x00, end address len bytes, receive buffer end is 256 bytes
	dev-> D1C = 0x00 | (((len-1) & 0x7F) << 8) | (0x100 << 16); 

	volatile uint32_t * p_buff = (uint32_t*)(i2c_mem + BUFFER_POOL_P7_OFFSET);
	
	// We can only write 32bit at a time, so byte writing is not an option
	// So we'll need to convert bytes to 32bits
	uint32_t tmp_buf[64] = {0};

	printf("%x\n", tmp_buf[0]);
	for (uint32_t i = 0; i < len; i++)
	{
		uint8_t tmp_i = (uint8_t)i/4;
		uint32_t byte_val = byte[i];
		tmp_buf[tmp_i] = tmp_buf[tmp_i] | (byte_val << ((i%4)*8));
	}
	printf("%x\n", tmp_buf[0]);


	for (uint32_t i = 0; i < (uint32_t)((float)len/4+0.5); i++)
	{
		p_buff[i] = tmp_buf[i];
		printf("byte %d is %x\n", i, p_buff[i]);
		printf("byte %d is %x\n", i, tmp_buf[i]);
	}


	printf("first byte is %x\n", p_buff[0]);

	if (byte[0] & 0x01) {
		// If it is a read command
		// Enable send and receive buffer
		dev->D14 = 0x63 | 0x8 | 0x80;		// Start -> Txbuf -> Receive -> Stop
		sleep_cycle(5000);	// 1ms

		// Calculate receive data bytes
		uint32_t rx_len = ((dev->D1C & 0xFF000000) >> 24) + 1;
		printf("rx_len is %d\n", rx_len);
		// Read data from receive buffer
		printf("rx data is %08x\n", p_buff[0]);
	} else {
		// If it is a write command
		// Enable send buffer
		dev->D14 = 0x43;		// Start -> Txbuf -> Stop
		sleep_cycle(5000);	// 1ms
	}


}
// Last bit of the 1st byte is W/R bit, 0 is write, 1 is read



// len is the number of bytes to send
void write_read_raw(volatile I2C_dev_t * dev, uint8_t byte[], const uint32_t len, uint32_t rx_buf[]) {
	// Base pointer 0x00, end address len bytes, receive buffer end is 256 bytes
	dev-> D1C = 0x00 | (((len-1) & 0x7F) << 8) | (0x100 << 16); 

	volatile uint32_t * p_buff = (uint32_t*)(i2c_mem + BUFFER_POOL_P7_OFFSET);
	
	// We can only write 32bit at a time, so byte writing is not an option
	// So we'll need to convert bytes to 32bits
	uint32_t tmp_buf[64] = {0};

	printf("%x\n", tmp_buf[0]);
	for (uint32_t i = 0; i < len; i++)
	{
		uint8_t tmp_i = (uint8_t)i/4;
		uint32_t byte_val = byte[i];
		tmp_buf[tmp_i] = tmp_buf[tmp_i] | (byte_val << ((i%4)*8));
	}
	printf("%x\n", tmp_buf[0]);


	for (uint32_t i = 0; i < (uint32_t)((float)len/4+0.5); i++)
	{
		p_buff[i] = tmp_buf[i];
	}

	// Send the command with write
	// Enable send buffer
	dev->D14 = 0x43;		// Start -> Txbuf -> Stop


	/////////////READ/////////////////////////////
	sleep_cycle(7000);	// 1ms
	dev->D10 = 0xFFFFFFFF; // clearing interrupt status

	// Read length is determined by the buffer end address
	// Only send 1 byte address here
	// Max receive buffer 255
	dev-> D1C = 0x00 | (((1-1) & 0x7F) << 8) | (0xff << 16);  

	// Send the address with read bit set
	uint32_t read_cmd = tmp_buf[0] & 0xffff;
	tmp_buf[0] = ((read_cmd & 0xff) +1); 
	
	p_buff[0] = tmp_buf[0];


	// Send read command
	// Enable send and receive buffer
	dev->D14 = 0x43 | 0x8 | 0xB0;		// Wait -> Txbuf -> Receive -> wait > Nack > Stop
	sleep_cycle(9000);	// 1ms

	// Calculate receive data bytes
	uint32_t rx_len = (dev->D1C  >> 24);

	for (uint32_t i = 0; i < (uint32_t)(((float)rx_len/4)+0.5); i++)
	{
		rx_buf[i] = p_buff[i];
	}
	#ifdef DEBUG
		printf("rx_len is %d\n", rx_len);
		// Read data from receive buffer
		printf("rx data is %08x\n", p_buff[0]);
	#endif

}

// Max read 32 bits for now, should be enough for our use case
// length is the expected length of the received data in bytes
uint32_t i2c_smbus_read_data(volatile I2C_dev_t * dev, 
	const uint8_t addr, const uint8_t command, const uint8_t length) 
{
	// Use Buffer Pool 7
	volatile uint32_t * p_buff = (uint32_t*)(i2c_mem + BUFFER_POOL_P7_OFFSET);

	//////////// Writting Stage //////////////
	// Base pointer 0x00, write 2 bytes
	dev-> D1C = 0x00 | ((0x1 & 0x7F) << 8); 
	// Send address first, then command
	// The last bit of the first byte is the W/R bit
	// 0 for write, 1 for read
	p_buff[0] = (addr << 1) | (command << 8);
	#ifdef DEBUG
		printf("[%s] write bytes %04x\n", __func__, p_buff[0]);
	#endif

	// Issue buffered send command
	dev->D14 = 0x43;		// Start -> Txbuf -> Wait

	//////////// Reading Stage //////////////
	// We only need to send the address with R bit set 
	// Then wait for the response

	// Wait and Cleanup 
	sleep_cycle(7000);	// 1ms
	dev->D10 = 0xFFFFFFFF; // clearing interrupt status

	// Base pointer 0x00, write 1 byte, read <length> bytes
	dev-> D1C = 0x00 | ((0 & 0x7F) << 8) | ((length - 1) << 16);  

	// Send the address with read bit
	p_buff[0] = (addr << 1) | 0x1;

	// Issue buffered write and read command
	dev->D14 = 0x43 | 0x8 | 0xB0;		// Start -> Txbuf -> Receive -> wait > NACK > Stop

	sleep_cycle(25000);	// 1ms
	uint32_t res = p_buff[0];

	// Calculate receive data bytes
	uint32_t rx_len = (dev->D1C  >> 24);
	if (rx_len != length) {
		printf("Received data len mismatch, actual received %d, expected %d\n", rx_len, length);
	}

	#ifdef DEBUG
		printf("[%s] rx_len is %d\n", __func_, rx_len);
		// Read data from receive buffer
		printf("[%s] rx data is %08x\n", __func__, p_buff[0]);
	#endif

	dev->D10 = 0xFFFFFFFF; // clearing interrupt status
	sleep_cycle(7000);	// 1ms
	return res;

}

// Max write 16 bits for now, should be enough for our use case
uint32_t i2c_smbus_write_data(volatile I2C_dev_t * dev, 
	const uint8_t addr, const uint8_t command, const uint8_t *data,
	const uint8_t length) 
{
	// Use Buffer Pool 7
	volatile uint32_t * p_buff = (uint32_t*)(i2c_mem + BUFFER_POOL_P7_OFFSET);

	//////////// Writting Stage //////////////
	// Base pointer 0x00, write length + cmd + addr bytes
	dev-> D1C = 0x00 | (((length+2-1) & 0x7F) << 8); 
	// Send address first, then command
	// The last bit of the first byte is the W/R bit
	// 0 for write, 1 for read
	if (length == 1){
		p_buff[0] = (addr << 1) | (command << 8) | data[0] << 16;
	}else if (length == 2){
		p_buff[0] = (addr << 1) | (command << 8) | data[0] << 16 | data[1] << 24;
	} else {
		perror("i2c_smbus_write_data length is too long\n");
	}
	#ifdef DEBUG
		printf("[%s] write bytes %04x\n", __func__, p_buff[0]);
	#endif

	// Issue buffered send command
	dev->D14 = 0x63;		// Start -> Txbuf -> Stop

	// Wait and Cleanup 
	sleep_cycle(9000);	// 1ms
	dev->D10 = 0xFFFFFFFF; // clearing interrupt status
	sleep_cycle(7000);	// 1ms

	// sleep_cycle(12000);	// 1ms
	// dev->D10 = 0xFFFFFFFF; // clearing interrupt status
	// sleep_cycle(7000);	// 1ms
	return 0;
}

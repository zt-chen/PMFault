#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <time.h>

#include "i2c.h"
#include "pmbus.h"
volatile uint8_t *i2c_mem;
volatile I2C_dev_t *i2c_3;

#define VERBOSE

uint32_t readMfrVrConfig(){
	uint32_t res = 0x0;
	res = i2c_smbus_read_data(i2c_3, DEVICE_ADDR, CMD_MFR_VR_CONFIG, 2);
#ifdef VERBOSE
	printf("[%s] RAW DATA: 0x%X\n", __func__, res );

	if ((res & MASK_VID_STEP_SEL) == VID_STEP_5MV){
		printf("[%s] VID STEP: 5mV\n", __func__);
	} else {
		printf("[%s] VID STEP: 10mV\n", __func__);
	}

	if ((res & MASK_OVER_CLOCK1_EN)){
		printf("[%s] OVERCLOCK MODE1 Enabled\n", __func__ );
	}
	if ((res & MASK_OVER_CLOCK2_EN)){
		printf("[%s] OVERCLOCK MODE2 Enabled\n", __func__ );
	}
#endif
	return res;
}

uint32_t setMfrVrConfig(uint32_t data){

	i2c_smbus_write_data(i2c_3, DEVICE_ADDR, CMD_MFR_VR_CONFIG, (uint8_t*)&data, 2);
	// readMfrVrConfig();
	return 0;
}

uint32_t readVoutCMD(){
	uint32_t res = 0x0;
	res = i2c_smbus_read_data(i2c_3, DEVICE_ADDR, CMD_VOUT_CMD, 2);

#ifdef VERBOSE
	printf("[%s] RAW DATA: 0x%X\n", __func__, res );
#endif
	return res;

}

void setVoutCMD(uint16_t data){
	i2c_smbus_write_data(i2c_3, DEVICE_ADDR, CMD_VOUT_CMD, (uint8_t*)&data, 2);
}
uint32_t readVoutOffset(){
	uint32_t res = i2c_smbus_read_data(i2c_3, DEVICE_ADDR, CMD_VOUT_OFFSET, 1);
	
#ifdef VERBOSE
	printf("[%s] RAW DATA: 0x%X, Offset %d VIDs\n", __func__, res, res);
#endif
	return res;

}

void setVoutOffset(uint32_t data){
	i2c_smbus_write_data(i2c_3, DEVICE_ADDR, CMD_VOUT_OFFSET, (uint8_t*)&data, 1);

}

uint32_t readVoutMax(){
	uint32_t res = i2c_smbus_read_data(i2c_3, DEVICE_ADDR, CMD_VOUT_MAX, 2);
#ifdef VERBOSE
	printf("[%s] RAW DATA: 0x%X\n", __func__, res);
#endif
	return res;
}

void setVoutMax(uint16_t data){
	i2c_smbus_write_data(i2c_3, DEVICE_ADDR, CMD_VOUT_MAX, (uint8_t*)&data, 2);
}

// MP2965 Page 102
uint32_t readMfrLoopPiSet(){

	uint32_t res = i2c_smbus_read_data(i2c_3, DEVICE_ADDR, CMD_MFR_LOOP_PI_SET, 2);
#ifdef VERBOSE
	uint32_t mode = res & MASK_VOUT_PMBUS_LSB;
	printf("[%s] RAW DATA: 0x%X\n", __func__, res);

	switch (mode) {
		case VOUT_FORMAT_VID:
			printf("[%s] %s\n", __func__, "Mode VID format");
			break;
		case VOUT_FORMAT_1MV:
			printf("[%s] %s\n", __func__, "Mode 1mV/LSB");
			break;
		default:
			printf("[%s] %s\n", __func__, "Mode Unknown");
	}
#endif
	return res;
}

uint32_t readOperation(){
	uint32_t res = i2c_smbus_read_data(i2c_3, DEVICE_ADDR, CMD_OPERATION, 1);
	if (res == -1) {
		// ERROR HANDLING: i2c transaction failed
		printf("Failed to read from the i2c bus.\n");
		return -1;
	} else {
		// Convert VID to actual voltage
		printf("[%s] OPERATION: %X\n", __func__, res );
		return res;
	}
}
void setOperation(uint32_t data){
	// Using SMBus commands
	i2c_smbus_write_data(i2c_3, DEVICE_ADDR, CMD_OPERATION, (uint8_t*)&data, 1);
}

uint32_t readVout(){
	uint32_t res = i2c_smbus_read_data(i2c_3, DEVICE_ADDR, CMD_VOUT, 2);
	#ifdef VERBOSE
		printf("[%s] RAW DATA: 0x%X\n", __func__, res);
	#endif
	return res & 0xff;
}


void undervoltSuper(uint8_t data){

	setVoutCMD(data);
	// Enable overclock mode 1 - Track Mode
//	uint32_t config2 = readMfrVrConfig();
//	config2 &= ~MASK_OVER_CLOCK1_EN;
//	config2 &= ~MASK_OVER_CLOCK2_EN ;


	// Enable overclock mode 2 - Fix Mode
	uint32_t config = readMfrVrConfig();
	config &= ~MASK_OVER_CLOCK1_EN;
	config |= MASK_OVER_CLOCK2_EN ;

	setMfrVrConfig(config);




}




int main(int argc, char *argv[]) {
	i2c_mem = NULL;

	// Should not be a problem if we are using 0x1E78A000
	// System page size should be 0x1000
    const size_t pagesize = sysconf(_SC_PAGE_SIZE);
	printf("pagesize: %d\n", pagesize);
	if (pagesize != 0x1000) 
	{
		printf("Error: page size is not 4k, the code need to be adjusted\n");
		return -1;
	}
	
    // Truncate offset to a multiple of the page size, or mmap will fail.
    // off_t page_base = (I2C_BASE / pagesize) * pagesize;
    // off_t page_offset = offset - page_base;

    int fd = open("/dev/mem", O_RDWR | O_SYNC);
    i2c_mem = mmap(NULL, I2C_MMIO_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, I2C_BASE);
    if (i2c_mem == MAP_FAILED) {
        perror("Can't map memory");
        return -1;
    } else {
		printf("Memory mapped at %p\n", i2c_mem);
	}

	volatile uint32_t *p_g00 = (uint32_t *)(i2c_mem + I2C_G0_OFFSET);
	volatile uint32_t *p_g08 = (uint32_t *)(i2c_mem + I2C_G8_OFFSET);
	// *p_g08 = 0xffffffff;
	*p_g08 = 0x00000000;

	printf("G0: 0x%08x, G8: 0x%08x\n", *p_g00, *p_g08);

	i2c_3 = (I2C_dev_t *)(i2c_mem + I2C_DEV3_OFFSET);
 	print_dev(i2c_3);
	config_dev(i2c_3);

	// I2C Init completed
	//////////////////////////////////////////////////////////

	printf("-------- system info --------\n");
//	dumpregs();
	readVoutMax();
	msleep(1);
	uint32_t voutMode = readMfrLoopPiSet();
	msleep(1);
	readMfrVrConfig();


	// This will read the voltage from the sensor
	uint32_t vout = readVout();
	float voltage = 0;

	if ((voutMode & MASK_VOUT_PMBUS_LSB) == VOUT_FORMAT_VID){
		voltage =(vout*0.005 +0.245);
		printf("Sensor Voltage: %f\n", voltage );
	}else{
		printf("Error: Unimplemented\n");
	}

	uint32_t offset = readVoutOffset();
	readOperation();
	msleep(1);


	uint32_t res = 0x0;
	res = i2c_smbus_read_data(i2c_3, DEVICE_ADDR, 0x7a, 1);
	printf("[%s] STATUS_VOUT: %X\n", __func__, res);
	msleep(1);

	res = i2c_smbus_read_data(i2c_3, DEVICE_ADDR, 0x7b, 1);
	printf("[%s] STATUS_IOUT: %X\n", __func__, res);
	msleep(1);

	res = i2c_smbus_read_data(i2c_3, DEVICE_ADDR, 0xee, 2);
	printf("[%s] MFR_OCP_TOTAL_SET: %X\n", __func__, res);
	msleep(1);

	printf("---------- Start !!!OVERVOLTING!!! ---------- \n");



	// Set VOUT_MAX
	config_dev(i2c_3);		// Re-configure the device, required
	res = readVoutMax();
	printf("[%s] VOUT_MAX: %X\n", __func__, res);
	msleep(1);

	res = 0x1ff;
	setVoutMax(res);
	msleep(1);

	// Disable OCP
	config_dev(i2c_3);		// Re-configure the device, required
	// uint16_t d = 0x05a3;
	uint16_t d = 0x07ff;
	i2c_smbus_write_data(i2c_3, DEVICE_ADDR, 0xee, (uint8_t*)&d, 2);
	msleep(20);

	// Enable voltage control
	config_dev(i2c_3);		// Re-configure the device, required
	uint32_t op_temp = readOperation();
	msleep(1);
	op_temp |= BIT1;
	setOperation(op_temp);
	msleep(1);
	
	config_dev(i2c_3);		// Re-configure the device, required
	// setVoutCMD(0x97);		// 2.0V
	// setVoutCMD(0xab);		// 2.2V
	// setVoutCMD(0xb5);		// 2.3V
	setVoutCMD(0xff);	// 3V
	msleep(1);

	// Enable overclock mode 2 - Fix Mode
	config_dev(i2c_3);		// Re-configure the device, required
	uint32_t config = readMfrVrConfig();
	msleep(1);
	uint32_t config_orgi = config;
	config = config | MASK_OVER_CLOCK2_EN;	// Fix Mode
	config &= ~BIT8; //  Use 10mV voltage table with gives VoutMax = 3V

	config_dev(i2c_3);		// Re-configure the device, required
	setMfrVrConfig(config);



	// Clean up
	close(fd);
    return 0;
}
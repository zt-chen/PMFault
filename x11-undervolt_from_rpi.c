//#include <glib.h>
//#include <glib/gprintf.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <getopt.h>
#include <i2c/smbus.h>

#define VERBOSE

#include "pmbus.h"

#define msleep(tms) ({usleep(tms * 1000);})

// Connect I2C-1 of Raspberry Pi to the I2C bus on the motherboard
#define I2C_BUS_DEV "/dev/i2c-1"



#define READBYTE(cmd) { \
	__u32 res=0x0; \
	res = i2c_smbus_read_byte_data(i2c_dev_fd, cmd); \
	printf("[%s] RAW DATA: %X\n", __func__, res ); \
	if (res == -1) { \
		printf("Failed to read from the i2c bus.\n"); \
		return -1; \
	} else { \
		return res; \
	} \
}

#define READWORD(cmd) { \
	__u32 res=0x0; \
	res = i2c_smbus_read_word_data(i2c_dev_fd, cmd); \
	printf("[%s] RAW DATA: %X\n", __func__, res ); \
	if (res == -1) { \
		printf("Failed to read from the i2c bus.\n"); \
		return -1; \
	} else { \
		return res; \
	} \
}

int i2c_dev_fd;

__u32 calVID(float voltage){
    if(voltage <= 0.25){
		return 0x1;
    }else if(voltage >=1.52 ){
		return 0xff;
    }
   	__u8 vid = (__u8)((voltage-0.245)/0.005);
    return vid;
  }


float readVolt(){
	__u32 res=0x0;

	// Using SMBus commands
	res = i2c_smbus_read_byte_data(i2c_dev_fd, CMD_VID_R);
	if (res == -1) {
		// ERROR HANDLING: i2c transaction failed
		printf("Failed to read from the i2c bus.\n");
		return -1;
	} else {
		// Convert VID to actual voltage
		return res*0.005 + 0.245;
	}

}

int setVoltFast(__u8 vid){
	return i2c_smbus_write_byte_data(i2c_dev_fd, CMD_VID_W , vid);
}
int setVolt(__u8 vid){
	__u32 res=0x0;
	// Using SMBus commands
	res = i2c_smbus_write_byte_data(i2c_dev_fd, CMD_VID_W , vid);
	if (res == -1) {
		// ERROR HANDLING: i2c transaction failed
		printf("Failed to read from the i2c bus.\n");
		return -1;
	} else {
		return res;
	}
}


int setMode(__u8 mode){
	__u32 res=0x0;

	// Using SMBus commands
	res = i2c_smbus_write_byte_data(i2c_dev_fd, CMD_MFR_VR_CONFIG, MASK_OVER_CLOCK2_EN  );
	if (res == -1) {
		// ERROR HANDLING: i2c transaction failed
		printf("Failed to read from the i2c bus.\n");
		return -1;
	} else {
		return res;
	}
}


__u32 readRevision(){
	READBYTE(CMD_PMBUS_REVISION);
}

__u32 readMode(){
	__u32 res=0x0;

	// Using SMBus commands
	res = i2c_smbus_read_byte_data(i2c_dev_fd, CMD_MOD);
	if (res == -1) {
		// ERROR HANDLING: i2c transaction failed
		printf("Failed to read from the i2c bus.\n");
		return -1;
	} else {
		return res;
	}
}

__u32 readVout(){
	READWORD(0x8B);
}

// PMBus Spec Part II 8.2
__u32 readVoutMode(){
	__u32 res = 0x0;

	res = i2c_smbus_read_byte_data(i2c_dev_fd, CMD_VOUT_MODE);
#ifdef VERBOSE
	__u32 mode = res & MASK_VOUT_MODE;
	__u32 param = res & MASK_VOUT_MODE_PARAM;

	switch (mode) {
		case VOUT_MODE_LINEAR:
			printf("[%s] %s, param %X\n", __func__, "Mode Linear", param);
			break;
		case VOUT_MODE_VID:
			printf("[%s] %s, param %X\n", __func__, "Mode VID", param);
			break;
		case VOUT_MODE_DIRECT:
			printf("[%s] %s, param %X\n", __func__, "Mode Direct", param);
			break;
		default:
			printf("[%s] %s ,RAW DATA: %X\n", __func__, "Mode Unknown", res);
	}
#endif
	return res;
}

__u32 readVoutCMD(){
	__u32 res = 0x0;
	res = i2c_smbus_read_word_data(i2c_dev_fd, CMD_VOUT_CMD);

#ifdef VERBOSE
	printf("[%s] RAW DATA: %X\n", __func__, res );
#endif
	return res;

}

__u32 setVoutCMD(__u16 data){
	__u32 res = i2c_smbus_write_word_data(i2c_dev_fd, CMD_VOUT_CMD, data);

#ifdef VERBOSE
	printf("[%s] RAW DATA: %X\n", __func__, res );
#endif
	return res;
}

__u32 readMfrVrConfig(){
	__u32 res = 0x0;
	res = i2c_smbus_read_word_data(i2c_dev_fd, CMD_MFR_VR_CONFIG);
#ifdef VERBOSE
	printf("[%s] RAW DATA: %X\n", __func__, res );

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

__u32 setMfrVrConfig(__u32 data){

	i2c_smbus_write_word_data(i2c_dev_fd, CMD_MFR_VR_CONFIG, data);
	readMfrVrConfig();
	return 0;
}

// MP2965 Page 102
__u32 readMfrLoopPiSet(){
	__u32 res = 0x0;

	res = i2c_smbus_read_word_data(i2c_dev_fd, CMD_MFR_LOOP_PI_SET);
#ifdef VERBOSE
	__u32 mode = res & MASK_VOUT_PMBUS_LSB;
	printf("[%s] RAW DATA: %X\n", __func__, res);

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

__s32 readVoutOffset(){
	__s32 res = 0x0;
	res = i2c_smbus_read_word_data(i2c_dev_fd, CMD_VOUT_OFFSET);
#ifdef VERBOSE
	printf("[%s] RAW DATA: %X, %d\n", __func__, res, res);
#endif
	return res;

}

__u32 setVoutOffset(__u32 data){
	__u32 res = 0x0;
	res = i2c_smbus_write_word_data(i2c_dev_fd, CMD_VOUT_OFFSET, data);
#ifdef VERBOSE
	printf("[%s] RAW DATA: %X\n", __func__, res);
#endif
	return res;
}

__s32 readVoutMax(){
	__s32 res = 0x0;
	res = i2c_smbus_read_word_data(i2c_dev_fd, 0x24);
#ifdef VERBOSE
	printf("[%s] RAW DATA: %X, %d\n", __func__, res, res);
#endif
	return res;

}

__s32 readMfrVrConfig2(){
	__s32 res = 0x0;
	res = i2c_smbus_read_word_data(i2c_dev_fd, 0x09);
#ifdef VERBOSE
	printf("[%s] RAW DATA: %X\n", __func__, res);
	if (res & BIT9){
		printf("[%s] PMBus override mode enabled\n", __func__ );
	}
#endif
	return res;
}

void dumpregs(){
	printf("---------Read byte--------\n");
	__s32 res = 0x0;
	__s32 res_word = 0x0;
	for (__u32 i=0; i<0xff; i++){
		res = i2c_smbus_read_byte_data(i2c_dev_fd, i);
		res_word = i2c_smbus_read_word_data(i2c_dev_fd, i);
		printf("[%s] Addr: %X, Value byte: %X,\t Value word %X\n", __func__, i, res, res_word);
	}

}

__s32 readOperation(){
	__u32 res=0x0;

	// Using SMBus commands
	res = i2c_smbus_read_byte_data(i2c_dev_fd, CMD_OPERATION);
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
void setOperation(__u32 data){
	// Using SMBus commands
	i2c_smbus_write_byte_data(i2c_dev_fd, CMD_OPERATION, data);
}

void undervoltSuper(__u8 data){

	setVoutCMD(data);
	// Enable overclock mode 2 - Fix Mode
//	__u32 config2 = readMfrVrConfig();
//	config2 &= ~MASK_OVER_CLOCK1_EN;
//	config2 &= ~MASK_OVER_CLOCK2_EN ;
	// Enable overclock mode 2 - Fix Mode
	__u32 config = readMfrVrConfig();
	config &= ~MASK_OVER_CLOCK1_EN;
	config |= MASK_OVER_CLOCK2_EN ;

	setMfrVrConfig(config);

	setVoutCMD(0x80);

	// Disable overclock mode 2 -Fix Mode
//	config = readMfrVrConfig();
	config &= ~MASK_OVER_CLOCK2_EN;
	setMfrVrConfig(config);

}
int main(int argc, char *argv[]){

	char *i2c_dev = NULL;
	int device_name_present = 0;

	int opt;
	while ((opt = getopt(argc, argv, "d:h")) != -1) {
		switch (opt) {
			case 'd':
				printf("Using Device: %s\n", optarg);
				i2c_dev = optarg;
				device_name_present = 1;
				break;
			case 'h':
				printf("Usage: %s -d <i2c device>\n", argv[0]);
				exit(0);
		}
	}

	if (device_name_present == 0){
		perror("No device name specified\n");
		exit(1);
	}


	// Open the bus
	if ((i2c_dev_fd = open(i2c_dev, O_RDWR)) < 0) {
		perror("Failed to open the i2c bus");
		exit(1);
	}

	// Initiating communication with the target device on the bus
	int addr = DEVICE_ADDR;		// The i2c address of the device
	if (ioctl(i2c_dev_fd, I2C_SLAVE, addr) < 0) {
		printf("Failed to acquire bus access and/or talk to slave.\n");
		exit(1);
	}

	readVoutMax();
	readMfrVrConfig2();
	__u32 voutMode = readMfrLoopPiSet();
	readMfrVrConfig();

	__u32 vout = readVout();

	readVoutOffset();

	if ((voutMode & MASK_VOUT_PMBUS_LSB) == VOUT_FORMAT_VID){
		float voltage =(vout*0.005 +0.245);
		printf("Voltage: %f\n", voltage );
	}else{
		printf("Unimplemented\n");
	}

	__u32 voutCMD = readVoutCMD();
	float voutCmdVoltage = (voutCMD*0.005 + 0.245);
	printf("VoutCMD voltage: %f\n", voutCmdVoltage);

	setVoutCMD(0x69);
	readVoutCMD();
	__u32 op_temp = readOperation();
	op_temp |= BIT1;
	setOperation(op_temp);

	for (int volt=0x80; volt>0x65; volt--){
		undervoltSuper(volt);
	}
}

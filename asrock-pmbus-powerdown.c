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
#include <stdbool.h>
#include <getopt.h>
#include <unistd.h>
#include <pmbus.h>

#include <i2c/smbus.h>

#define msleep(tms) ({usleep(tms * 1000);})



#define READBYTE(cmd) { \
	__u32 res=0x0; \
	res = i2c_smbus_read_byte_data(i2c_dev_fd, cmd); \
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
	if (res == -1) { \
		printf("Failed to read from the i2c bus.\n"); \
		return -1; \
	} else { \
		return res; \
	} \
}

int i2c_dev_fd;
bool debug = false;

// Wrapper functions for i2c_smbus_read to handle byte/word reads and errors.
// Return 0 if the device support PMBus commands
int PMBusRead(int i2c_dev_fd, __u8 device_addr, int cmd_to_test, int bytes_to_read, int *read_res){
	msleep(200);
	int res = 0x00;
	// Initiating communication with the target device on the bus
	if (ioctl(i2c_dev_fd, I2C_SLAVE, device_addr) < 0) {
		printf("Failed to acquire bus access and/or talk to slave.\n");
		printf("Device addr: %02X\n", device_addr);
		exit(1);
	}

	if (bytes_to_read == 1) {
		res = i2c_smbus_read_byte_data(i2c_dev_fd, cmd_to_test);
	} else if (bytes_to_read == 2) {
		res = i2c_smbus_read_word_data(i2c_dev_fd, cmd_to_test);
	} else {
		printf("Invalid byte number.\n");
		return -1;
	}
	if (res < 0) {
		if (debug) {
			printf("Device %02X: read failed. error: %d\n", device_addr, res);
		}
		return -2;
	} else {
		*read_res = res;
		return 0;
	}
}

int PMBusSet(int i2c_dev_fd, __u8 device_addr, int cmd, int bytes_to_write, __u16 val){
	msleep(200);

	// Initiating communication with the target device on the bus
	if (ioctl(i2c_dev_fd, I2C_SLAVE, device_addr) < 0) {
		printf("Failed to acquire bus access and/or talk to slave.\n");
		printf("Device addr: %02X\n", device_addr);
		exit(1);
	}

	__u32 res=0x0;
	// Using SMBus commands
	if (bytes_to_write == 1) {
		res = i2c_smbus_write_byte_data(i2c_dev_fd, cmd , (__u8) val);
	} else if (bytes_to_write == 2) {
		res = i2c_smbus_write_word_data(i2c_dev_fd, cmd , val);
	} else {
		printf("Invalid byte number.\n");
		return -1;
	}

	if (res == -1) {
		// ERROR HANDLING: i2c transaction failed
		printf("Failed to read from the i2c bus.\n");
		return -1;
	} else {
		return res;
	}
}


int PMBusTestVout(int i2c_dev_fd, __u8 device_addr, int *read_res){
	int vout_raw = 0;
	int res = PMBusRead(i2c_dev_fd, device_addr, 0x8b, 2, &vout_raw);
	if (res == 0) {
		__u16 vout = vout_raw;
		if (debug) {
			printf("Device %02X: Vout:  %04X\n", device_addr, vout);
		}
		// If response is not 0xfff or 0x0 it means the device is probably replying with a valid VID
		// And it support PMBus READ_VOUT command
	
		if (vout == 0xffff || vout == 0x0) {
			return -1;
		} else {
			*read_res = vout_raw;
			return 0;
		}
	} else {
		return res;
	}
}

// PMBUS_READ_IOUT
int PMBusTestIOUT(int i2c_dev_fd, __u8 device_addr, int *read_res){
	int rev_raw = 0;
	int res = PMBusRead(i2c_dev_fd, device_addr, 0x8c, 2, &rev_raw);
	if (res == 0){
		__u16 rev = rev_raw;
		if (debug)
		{
			printf("Device %02X: READ_IOUT result %4X\n", device_addr, rev);
		}

		if (rev != 0x0000 && rev != 0xffff) {
			*read_res = rev_raw;
			return 0;
		} else {
			return -1;
		}
	} else {
		return res;
	}
}

int PMBusReadPage(int i2c_dev_fd, __u8 device_addr, int *read_res){
	int rev_raw = 0;
	int res = PMBusRead(i2c_dev_fd, device_addr, 0x00, 1, &rev_raw);
	if (res == 0){
		__u8 rev = rev_raw;
		if (debug)
		{
			printf("Device %02X: READ_PAGE result %02X\n", device_addr, rev);
		}
		*read_res = (int)rev_raw;
		return 0;
	} else {
		return res;
	}
}

int PMBusSetPage(int i2c_dev_fd, __u8 device_addr, int *set_val){
	int val = *set_val;
	int res = PMBusSet(i2c_dev_fd, device_addr, 0x00, 1, (__u16) val);
	if (res == 0){
		__u8 rev = val;
		if (debug)
		{
			printf("Device %02X: SET_PAGE val to %2X\n", device_addr, rev);
		}
		return 0;
	} else {
		return res;
	}
}


// PMBUS_MFR_ADDR_PMBUS 
int MPTestADDR_PMBUS(int i2c_dev_fd, __u8 device_addr, int *read_res){
	int rev_raw = 0;
	int res = PMBusRead(i2c_dev_fd, device_addr, 0xe1, 1, &rev_raw);
	if (res == 0){
		__u8 addr = rev_raw;
		if (debug)
		{
			printf("Device %02X: MFR_ADDR_PMBUS result %02X\n", device_addr, addr);
		}

		if (addr == device_addr) {
			*read_res = rev_raw;
			return 0;
		} else {
			return -1;
		}
	} else {
		return res;
	}
}

int main(int argc, char* argv[]){
    int opt;
	char* i2c_dev_name = NULL;
	while ((opt = getopt(argc, argv, "td:")) != -1) {
        switch (opt) {
        case 't': debug = true; break;
        case 'd': 
			i2c_dev_name = optarg;
			break;
        default:
			fprintf(stderr, "Usage: %s [-t] [-d i2c_dev_name]\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }
	if (i2c_dev_name == NULL) {
		printf("Please specify the i2c device name with [-d i2c_dev_name].\n");
		exit(1);
	}
	// Open the bus
	if ((i2c_dev_fd = open(i2c_dev_name, O_RDWR)) < 0) {
		perror("Failed to open the i2c bus");
		exit(1);
	}

	// Read ID_DEVICE in broadcast mode
	__u8 read_res_block[16];
	// Initiating communication with the target device on the bus
	if (ioctl(i2c_dev_fd, I2C_SLAVE, 0x00) < 0) {
		printf("Failed to acquire bus access and/or talk to slave.\n");
		printf("Device addr: %02X\n", 0x00);
		exit(1);
	}	
	int byte_count = i2c_smbus_read_block_data(i2c_dev_fd, 0xad, read_res_block);
	if ( byte_count > 0) { // 
		printf("Device 0x%02X(Broadcast) \t ID_DEVICE success, byte count: %02X, data: ", 0x00, byte_count);

		for (int i = 0; i < byte_count; i++) {
			printf("%02X ", read_res_block[i]);
		}
		printf("\n");
	}


	// Device is at 0x60, page 0x1 is active
	__u8 dev_addr = 0x60;
	int read_res = 0;
	// Verify the device is at 0x60 with the ID_DEVICE command
	byte_count = i2c_smbus_read_block_data(i2c_dev_fd, 0xad, read_res_block);
	if ( byte_count > 0) { // 
		printf("Device 0x%02X \t ID_DEVICE success, byte count: %02X, data: ", dev_addr, byte_count);

		for (int i = 0; i < byte_count; i++) {
			printf("%02X ", read_res_block[i]);
		}
		printf("\n");
		
		// Read original page config
		if (PMBusReadPage(i2c_dev_fd, dev_addr, &read_res) == 0) {
			printf("Device 0x%02X \t READ_PAGE success: %02X\n", dev_addr, read_res);
		}
		// Set page to 0x01
		if (PMBusSet(i2c_dev_fd, dev_addr, CMD_PAGE, 1, 0x01) == 0) {
			printf("Device 0x%02X \t WRITE_PAGE success\n", dev_addr);
		}

		if (PMBusRead(i2c_dev_fd, dev_addr, 0x2, 1, &read_res) == 0) {
			printf("Device 0x%02X \t ON_OFF_CONFIG success: %02X\n", dev_addr, read_res);
		}

		// Read Vout
		if (PMBusTestVout(i2c_dev_fd, dev_addr, &read_res) == 0) {
			printf("Device 0x%02X \t READ_VOUT success: %04X\n", dev_addr, read_res);
		}

		// READ_OPERATION
		int res = PMBusRead(i2c_dev_fd, dev_addr, 0x01, 1, &read_res);
		if (res == 0) {
			printf("Device 0x%02X \t READ_OPERATION success: %02X\n", dev_addr, read_res);
		}

		// SET OPERATION to 0x00: Immediately off
		res = PMBusSet(i2c_dev_fd, dev_addr, 0x01, 1, 0x00);
		if (res == 0) {
			printf("Device 0x%02X \t Set OPERATION success\n", dev_addr);
		}

		// ON_OF_CONFIG from settings of OPERATION ONLY
		if (PMBusSet(i2c_dev_fd, dev_addr, 0x2, 1, 0x1a) == 0) {
			printf("Device 0x%02X \t Set ON_OFF_CONFIG success\n", dev_addr);
		}

		// APPLY_SETTINGS
		// res = PMBusSet(i2c_dev_fd, dev_addr, 0xe7, 2, 0x01);
		// if (res == 0) {
		// 	printf("Device 0x%02X \t APPLY_SETTINGS success\n", dev_addr);
		// }
		
		
	}

	close(i2c_dev_fd);

}

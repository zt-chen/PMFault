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

#include <i2c/smbus.h>

#define msleep(tms) ({usleep(tms * 1000);})
#define SLEEP_GAP 200


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
	msleep(SLEEP_GAP);
	int res = 0x00;
	// Initiating communication with the target device on the bus
	if (ioctl(i2c_dev_fd, I2C_SLAVE, device_addr) < 0) {
		printf("Failed to acquire bus access and/or talk to slave.\n");
		printf("Device addr: %02X\n", device_addr);
		exit(1);
	}

	// Use command 0x8b	to read VOUT
	//res = i2c_smbus_read_word_data(i2c_dev_fd, 0x8b);
	// res = i2c_smbus_read_byte_data(i2c_dev_fd, 0xd6);
	if (bytes_to_read == 1) {
		res = i2c_smbus_read_byte_data(i2c_dev_fd, cmd_to_test);
	} else if (bytes_to_read == 2) {
		res = i2c_smbus_read_word_data(i2c_dev_fd, cmd_to_test);
	} else {
		// res = i2c_smbus_read_block_data(i2c_dev_fd, cmd_to_test, read_res);
		printf("Invalid byte number.\n");
		*read_res = 0x00;
		return -1;
	}
	if (res < 0) {
		if (debug) {
			printf("Device %02X: read failed. error: %d\n", device_addr, res);
		}
		*read_res = 0x00;
		return -2;
	} else {
		*read_res = res;
		return 0;
	}
}

int PMBusSet(int i2c_dev_fd, __u8 device_addr, int cmd, int bytes_to_write, __u16 val){
	msleep(SLEEP_GAP);

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

int VRMfrDetect(int i2c_dev_fd, __u8 dev_addr){
	int res = -1; 		// -1 means not found
	__u8 read_res_block[16];
	int read_res = 0;
	// Check if the device responds to ISL_DEVICE_ID (ISL68137)
	int byte_count = i2c_smbus_read_block_data(i2c_dev_fd, 0xad, read_res_block);
	if ( byte_count > 0) { // 
		printf("This device is likely to be an ISL VRM\n");
		printf("Device 0x%02X \t ISL_DEVICE_ID success, byte count: %02X, data: ", dev_addr, byte_count);	
		for (int i = 0; i < byte_count; i++) {
			printf("%02X ", read_res_block[i]);
		}
		printf("\n");	
		res = 1;
	}

	// Check if the device responds to SVID_VENDOR_PRODUCT_ID (0xBF)
	if (PMBusRead(i2c_dev_fd, dev_addr, 0xbf, 2, &read_res) == 0){ // 
		if (read_res != 0xff && read_res != 0x00 && read_res != 0xffff && read_res != 0xff00) {
			printf("Device 0x%02X \t SVID_VENDOR_PRODUCT_ID success, data: %04X\n", dev_addr, read_res);
			if (res > 0) {
				printf("I'm confused, this device signature also match another vendor. \n");
			}
			printf("This device is likely to be a MPS VRM\n");
			res = 2;
		}
	}
	return res;
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

	// Test read block
	__u8 read_res_block[16];


	bool found_device = false;
	for (__u8 dev_addr = 0x10; dev_addr <= 0x7f; dev_addr++) {

		int read_res = 0x00;

		if (PMBusRead(i2c_dev_fd, dev_addr, 0x8d, 2, &read_res) == 0){ // 
			if (read_res == 0xff || read_res == 0x00 || read_res == 0xffff || read_res == 0xff00) {
				continue;
			}

			printf("Device 0x%02X \t READ_TEMPERATURE success: %04X\n", dev_addr, read_res);
			found_device = true;
			printf("!!!!!!!!!!! Detected! Device addr: %02X !!!!!!!!!!!\n", dev_addr);

			// Detect VRM vendor
			VRMfrDetect(i2c_dev_fd, dev_addr);
		
			// READ PMBUS_VERSION
			if (PMBusRead(i2c_dev_fd, dev_addr, 0x98, 1, &read_res) == 0){  // PRODUCT_REV
				printf("Device 0x%02X \t PMBUS_VERSION success: %02X\n", dev_addr, read_res);
			}
			// Set page
			int page_saved = 0;
			if (PMBusRead(i2c_dev_fd, dev_addr, 0x00, 1, &page_saved) == 0) {
				printf("Device 0x%02X : %02X \t READ_PAGE success\n", dev_addr, page_saved);
			}

			// Page loop
			for (__u8 page = 0x00; page <= 0x01; page++){
				printf("\nPage: %02X\n", page);

				read_res = 0x00;
				msleep(100);
				// Set page
				if (PMBusSet(i2c_dev_fd, dev_addr, 0x00, 1, page) == 0) {
					printf("Device 0x%02X : %02X \t WRITE_PAGE success\n", dev_addr, page);
				}

				// Test Vout
				if (PMBusTestVout(i2c_dev_fd, dev_addr, &read_res) == 0) {
					if (read_res != 0x0000 && read_res != 0xffff) {
						printf("Device 0x%02X : %02X \t READ_VOUT success: %04X\n", dev_addr, page, read_res);
					}
				}

				// Read MFR_ID
				int byc = i2c_smbus_read_block_data(i2c_dev_fd, 0x99, read_res_block);
				if (byc > 0) { // 
					printf("Device 0x%02X : %02X \t MFR_ID success, byte count: %02X, data: ", dev_addr, page, byc);	
					for (int i = 0; i < byc; i++) {
						printf("%02X ", read_res_block[i]);
					}
					printf("\n");
				}
			}
			// Reset page after scan
			if (PMBusSet(i2c_dev_fd, dev_addr, 0x00, 1, page_saved) == 0) {
				printf("Device 0x%02X : %02X \t WRITE_PAGE success\n", dev_addr, page_saved);
			}
		}
	}
	
	close(i2c_dev_fd);
	if (!found_device) {
		printf("No device found.\n");
	}
}

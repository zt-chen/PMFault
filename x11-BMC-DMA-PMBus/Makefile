
# Change this to path to gnu toolchain
# export PATH:=../arm-linux-gnueabi/bin

CFLAGS=$(INCLUDES) -O0 -Wall -std=c11
CC=arm-linux-gnueabi-gcc 
AR=arm-linux-gnueabi-ar make
BASE64=/usr/bin/base64

all: undervolt brick

brick: libi2cdma
	$(CC) $(CFLAGS) -o brick brick.c libi2cdma.a
	$(BASE64) brick > brick.base64

undervolt: libi2cdma
	$(CC) $(CFLAGS) -o undervolt undervolt.c libi2cdma.a
	$(BASE64) undervolt > undervolt.base64

libi2cdma: i2c.h i2c.c
	$(CC) $(CFLAGS) -o libi2cdma.a -c i2c.c

load:
	sudo modprobe i2c_i801

clean:
	rm -f *.s *.a *.base64 brick undervolt
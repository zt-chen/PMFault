INCLUDES= -I./i2c-tools/include -L./i2c-tools/lib -I./
CFLAGS=$(INCLUDES) -O0 -Wall -std=c11

all: rpi-undervolt asrock-pmbus-powerdown

rpi-undervolt: x11-undervolt_from_rpi.c
	$(CC) $(CFLAGS) x11-undervolt_from_rpi.c -o x11-undervolt_from_rpi -l:libi2c.a

asrock-pmbus-powerdown: asrock-pmbus-powerdown.c
	$(CC) $(CFLAGS) asrock-pmbus-powerdown.c -o asrock-pmbus-powerdown -li2c -static -std=gnu11

clean:
	rm -f $(OBJ) *.o *.a asrock-pmbus-powerdown x11-undervolt_from_rpi
	rm -f *.s

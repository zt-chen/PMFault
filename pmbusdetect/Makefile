INCLUDES= -I./i2c-tools/include -L./i2c-tools/lib -I./
CFLAGS=$(INCLUDES) -O0 -Wall -std=c11

all: pmbusdetect

pmbusdetect: pmbusdetect.c
	$(CC) $(CFLAGS) pmbusdetect.c -o pmbusdetect -li2c -static -std=gnu11

clean:
	rm -f ./pmbusdetect
CC=gcc
DEBUG=-g -D_DEBUG
DEFINE=-DMODULE -D__KERNEL__ -DLINUX
WARNINGS=-Wall -Wmissing-prototypes -Wmissing-declarations
#ISO=-ansi -pedantic
CC_OPTIONS=-O1 $(WARNINGS) $(ISO) $(DEBUG) $(DEFINE)

# Where to look for header files
INC=-I. -I/usr/include -I/usr/src/kernels/`uname -r`/include

DRIVER=mapdriver.o
MODULE=mapdriver.ko
EXE=mapdriver-test
CLIENT=mapclient
OBJ=main.o $(DRIVER)

obj-m += $(DRIVER)

all: $(EXE) $(CLIENT) $(SERVER)
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

prog:
	make register
	mknod /dev/asciimap c 236 0
	chmod 766 /dev/asciimap

clean:
	rm -f $(EXE) $(OBJ) $(CLIENT) $(SERVER)
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

compile: $(EXE) $(OBJ)

register: $(DRIVER)
	insmod ./$(MODULE)
	modinfo $(MODULE)
	lsmod | grep mapdriver
	@echo ""
	@echo "ASCII Character Device Driver has been built and registered."
	@echo ""

$(DRIVER): types.h mapdriver.h mapdriver.c
	$(CC) $(CC_OPTIONS) $(INC) -c mapdriver.c

test1:
	./$(EXE) test.txt 1 120 20

test2:
	./$(EXE) test.txt 1 120 20 test.txt 10 120 20

clean-all:
	rmmod mapdriver

$(CLIENT): mapclient.c common.c common.h
	$(CC) $(CC_OPTIONS) $(INC) -o $(CLIENT) mapclient.c common.c

$(EXE): mapdriver.c common.c common.h
	$(CC) $(CC_OPTIONS) $(INC) -o $(EXE) mapdriver.c common.c

$(SERVER): mapserver.c common.c common.h
	$(CC) $(CC_OPTIONS) $(INC) -o $(SERVER) mapserver.c common.c

run-server: $(SERVER)
	./$(SERVER) 3000

.PHONY: all prog clean compile register test1 test2 clean-all run-server


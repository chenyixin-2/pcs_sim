#topee imx6ull
#CC=/usr/local/arm/gcc-linaro-4.9.4-2017.01-x86_64_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-
#mrzy
#CC=/usr/local/arm/fsl-imx-x11/4.1.15-2.1.0/sysroots/x86_64-pokysdk-linux/usr/bin/arm-poky-linux-gnueabi/arm-poky-linux-gnueabi-
#mc am335x
#CC=/usr/local/arm/gcc-linaro-arm-linux-gnueabihf-4.7-2013.03-20130313_linux/bin/arm-linux-gnueabihf-
#ubuntu
#CC=gcc
# em-500
#CC=arm-linux-gnueabihf-
# FCU1104
CC=arm-poky-linux-gnueabi-gcc

LDFLAGS=-lpthread -lm -lrt -ldl -ggdb3
# CFLAGS=-std=c++11 -fpermissive -flto -Wfatal-errors -Wno-unused-but-set-variable -Wno-unused-parameter -Wno-unused-variable -Wno-comment -Werror=format -Werror=incompatible-pointer-types -Werror=implicit-int
CFLAGS=-flto -Wfatal-errors -Wno-unused-but-set-variable -Wno-unused-parameter -Wno-unused-variable -Wno-comment -Werror=format -Werror=incompatible-pointer-types -Werror=implicit-int
			
INCLUDES = $(addprefix -I,$(shell find appl/ -type d -print)) $(addprefix -I,$(shell find platform/ -type d -print)) $(addprefix -I,$(shell find component/ -type d -print))
SRC = $(shell find platform/ -name "*.c") $(shell find appl/ -name "*.c") $(shell find component/ -name "*.c")

all:
	$(CC) --sysroot=/opt/fsl-imx-x11/4.1.15-2.1.0/sysroots/cortexa7hf-neon-poky-linux-gnueabi/ -mfloat-abi=hard $(CFLAGS) $(SRC) $(INCLUDES) -o bin/pcs_sim $(LDFLAGS) 	
debug:
	gcc $(SRCFILES) $(INCLUDES) -lpthread -lm -ldl -lrt -w -g -o ./bin/pcs_sim
clean:
	rm pcs_sim

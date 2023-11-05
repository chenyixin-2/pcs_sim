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
CC=arm-poky-linux-gnueabi-
INCPATH += -I platform \
		   -I platform/abs \
		   -I platform/util \
		   -I platform/util/mqtt \
		   -I platform/util/tbmqtt \
		   -I platform/util/log \
		   -I platform/util/ringbuffer \
		   -I platform/component/zlog \
		   -I platform/component/freemodbus/modbus/ascii \
		   -I platform/component/freemodbus/modbus/include \
		   -I platform/component/freemodbus/modbus/rtu \
		   -I platform/component/freemodbus/modbus/tcp \
		   -I platform/component/freemodbus/port \
		   -I platform/component/libmodbus \
		   -I platform/component/libmodbus/src \
		   -I platform/component/sqlite \
		   -I platform/component/cJSON \
		   -I platform/component/paho.mqtt.c-1.3.9/src \
		   -I platform/component/snowid \
		   -I platform/device/chan \
		   -I platform/device/meter \
		   -I platform/device/misc \
		   -I include/ \
		   -I appl

INCLUDES = $(INCPATH)
SRCFILES = 	appl/main.c \
			appl/appl.c \
			appl/sta.c \
			appl/cloud.c \
			appl/tb.c \
			platform/abs/plt.c \
			platform/util/misc.c \
			platform/util/log/log.c \
			platform/util/tool.c \
			platform/util/shm.c \
			platform/util/mqtt/mqtt.c \
			platform/util/mqtt/mqtt_ringbuffer.c \
			platform/util/tbmqtt/tbmqtt.c \
			platform/util/tbmqtt/tbmqtt_ringbuffer.c \
			platform/util/tbmqtt/tbmqtt_cache.c \
			platform/util/ringbuffer/test_ringbuffer.c \
			platform/device/chan/chan_ringbuffer.c \
			platform/device/chan/chan.c \
			platform/device/chan/mbs.c \
			platform/device/misc/mac.c \
			platform/component/freemodbus/modbus/ascii/mbascii.c \
			platform/component/freemodbus/modbus/functions/mbfunccoils.c \
			platform/component/freemodbus/modbus/functions/mbfuncdiag.c \
			platform/component/freemodbus/modbus/functions/mbfuncdisc.c \
			platform/component/freemodbus/modbus/functions/mbfuncholding.c \
			platform/component/freemodbus/modbus/functions/mbfuncinput.c \
			platform/component/freemodbus/modbus/functions/mbfuncother.c \
			platform/component/freemodbus/modbus/functions/mbutils.c \
			platform/component/freemodbus/modbus/rtu/mbcrc.c \
			platform/component/freemodbus/modbus/rtu/mbrtu.c \
			platform/component/freemodbus/modbus/tcp/mbtcp.c \
			platform/component/freemodbus/modbus/mb.c \
			platform/component/freemodbus/port/portevent.c \
			platform/component/freemodbus/port/portother.c \
			platform/component/freemodbus/port/portserial.c \
			platform/component/freemodbus/port/porttcp.c \
			platform/component/freemodbus/port/porttimer.c \
			platform/component/libmodbus/src/modbus.c \
			platform/component/libmodbus/src/modbus-data.c \
			platform/component/libmodbus/src/modbus-rtu.c \
			platform/component/libmodbus/src/modbus-tcp.c \
			platform/component/sqlite/sqlite3.c \
			platform/component/cJSON/cJSON.c \
			platform/component/zlog/buf.c \
			platform/component/zlog/category.c \
			platform/component/zlog/category_table.c \
			platform/component/zlog/conf.c \
			platform/component/zlog/event.c \
			platform/component/zlog/format.c \
			platform/component/zlog/level.c \
			platform/component/zlog/level_list.c \
			platform/component/zlog/mdc.c \
			platform/component/zlog/record.c \
			platform/component/zlog/record_table.c \
			platform/component/zlog/rotater.c \
			platform/component/zlog/rule.c \
			platform/component/zlog/spec.c \
			platform/component/zlog/thread.c \
			platform/component/zlog/zc_arraylist.c \
			platform/component/zlog/zc_hashtable.c \
			platform/component/zlog/zc_profile.c \
			platform/component/zlog/zc_util.c \
			platform/component/zlog/zlog.c \
			platform/component/paho.mqtt.c-1.3.9/src/Base64.c \
			platform/component/paho.mqtt.c-1.3.9/src/Clients.c \
			platform/component/paho.mqtt.c-1.3.9/src/Heap.c \
			platform/component/paho.mqtt.c-1.3.9/src/LinkedList.c \
			platform/component/paho.mqtt.c-1.3.9/src/Log.c \
			platform/component/paho.mqtt.c-1.3.9/src/Messages.c \
			platform/component/paho.mqtt.c-1.3.9/src/MQTTClient.c \
			platform/component/paho.mqtt.c-1.3.9/src/MQTTPacket.c \
			platform/component/paho.mqtt.c-1.3.9/src/MQTTPacketOut.c \
			platform/component/paho.mqtt.c-1.3.9/src/MQTTPersistence.c \
			platform/component/paho.mqtt.c-1.3.9/src/MQTTPersistenceDefault.c \
			platform/component/paho.mqtt.c-1.3.9/src/MQTTProperties.c \
			platform/component/paho.mqtt.c-1.3.9/src/MQTTProtocolClient.c \
			platform/component/paho.mqtt.c-1.3.9/src/MQTTProtocolOut.c \
			platform/component/paho.mqtt.c-1.3.9/src/MQTTReasonCodes.c \
			platform/component/paho.mqtt.c-1.3.9/src/MQTTTime.c \
			platform/component/paho.mqtt.c-1.3.9/src/OsWrapper.c \
			platform/component/paho.mqtt.c-1.3.9/src/SHA1.c \
			platform/component/paho.mqtt.c-1.3.9/src/Socket.c \
			platform/component/paho.mqtt.c-1.3.9/src/SocketBuffer.c \
			platform/component/paho.mqtt.c-1.3.9/src/SSLSocket.c \
			platform/component/paho.mqtt.c-1.3.9/src/StackTrace.c \
			platform/component/paho.mqtt.c-1.3.9/src/Thread.c \
			platform/component/paho.mqtt.c-1.3.9/src/Tree.c \
			platform/component/paho.mqtt.c-1.3.9/src/utf-8.c \
			platform/component/paho.mqtt.c-1.3.9/src/WebSocket.c \
			platform/component/snowid/snowid.c \
			platform/component/snowid/snowid_util.c \
			platform/component/snowid/snowid_checkpoint.c
			
all:
	$(CC)gcc --sysroot=/opt/fsl-imx-x11/4.1.15-2.1.0/sysroots/cortexa7hf-neon-poky-linux-gnueabi/ -mfloat-abi=hard $(SRCFILES) $(INCLUDES) -lpthread -lm -ldl -lrt -w -g -o ./bin/pcs_sim
debug:
	gcc $(SRCFILES) $(INCLUDES) -lpthread -lm -ldl -lrt -w -g -o ./bin/pcs_sim
clean:
	rm pcs_sim

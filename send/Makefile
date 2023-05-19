CC=g++
COPTION=-std=c++14 -O2
COPENCV=-L/usr/local/include/opencv2/ -lopencv_videoio -lopencv_core -lopencv_imgcodecs
CMQTT=-lpaho-mqttpp3 -lpaho-mqtt3as
.PHONY: all

all : send send_vi

send : ./send.cpp
	$(CC) $(COPTION) $^ -o $@ $(COPENCV) $(CMQTT)

send_vi : ./send_vi.cpp
	$(CC) $(COPTION) $^ -o $@ $(COPENCV)

.PHONY: clean
clean :
	rm send send_vi

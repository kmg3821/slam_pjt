CC=g++
COPTION=-std=c++14 -O2
COPENCV=-L/usr/local/include/opencv2/ -lopencv_videoio -lopencv_core -lopencv_imgcodecs

.PHONY: all

all : imu send

imu : ./imu.cpp
	$(CC) $(COPTION) $^ -o $@

send : ./send.cpp
	$(CC) $(COPTION) $^ -o $@ $(COPENCV)

test : ./test.cpp
	$(CC) $(COPTION) $^ -o $@ $(COPENCV)

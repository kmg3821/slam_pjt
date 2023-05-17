CC=g++
COPTION=-std=c++14 -O2
COPENCV=-L/usr/local/include/opencv2/ -lopencv_videoio -lopencv_core -lopencv_imgcodecs

.PHONY: all

all : send send_vi

send : ./send.cpp
	$(CC) $(COPTION) $^ -o $@ $(COPENCV)

send_vi : ./send_vi.cpp
	$(CC) $(COPTION) $^ -o $@ $(COPENCV)

.PHONY: clean
clean :
	rm send send_vi

CC=g++
COPTION=-std=c++14 -O2
COPENCV=-L/usr/local/include/opencv2 -lopencv_videoio -lopencv_core -lopencv_imgcodecs -lopencv_highgui

.PHONY: all

all: main slam

main: main.cpp
	$(CC) $(COPTION) $^ -o $@ $(COPENCV)

slam: slam.cc
	$(CC) $(COPTION) $^ -o $@ $(COPENCV)

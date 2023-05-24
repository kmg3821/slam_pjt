// compile option
// g++ -std=c++14 -O2 ./send.cpp -o ./send -L/usr/local/include/opencv2/ -lopencv_videoio -lopencv_core -lopencv_imgcodecs -lpaho-mqttpp3 -lpaho-mqtt3as

#include <iostream>
#include <fstream>
#include <algorithm>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

#include <opencv2/opencv.hpp>
#include <opencv2/core.hpp>
#include <opencv2/videoio/videoio_c.h>
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>
#include <chrono>

#include <mqtt/client.h>

using namespace std;
using namespace cv;

int main()
{
    ifstream configFile("../../../config/config.txt");
    string line;
    string client_address;
    string client_port;
    while(getline(configFile, line)) {
    	if (line.substr(0, 15) == "MQTT_BROKER_IP=") {
		client_address = line.substr(15);
	}
	if (line.substr(0, 17) == "MQTT_BROKER_PORT=") {
		client_port = line.substr(17);
	}
    }
    if(client_address == ""){
    	cerr << "IP_ADDRESS NOT SET. Please write config.txt as format in config-stub.txt" << endl;
	return EXIT_FAILURE;
    }
    cout << "Connected MQTT Broker on " << client_address << ":" << client_port << endl;
    mqtt::async_client cli(client_address + ":" + client_port, "pub_send");
    cli.connect()->wait();
    mqtt::topic topic(cli, "img", 0);

    // send the images
    VideoCapture cap("/dev/video0");
    if (!cap.isOpened())
    {
        cerr << "Failed to open video file." << endl;
        return EXIT_FAILURE;
    }

    system("v4l2-ctl -c brightness=75"); // 0 ~ 100, default = 50
    system("v4l2-ctl -c contrast=100");   // -100 ~ 100, default = 0
    system("v4l2-ctl -c saturation=100"); // -100 ~ 100, default = 0
    system("v4l2-ctl -c sharpness=100"); // 예리함 정도
    system("v4l2-ctl -c rotate=180");    // 회전

    system("v4l2-ctl -c auto_exposure=0");           // 0:auto, 1:manual
    system("v4l2-ctl -c exposure_time_absolute=30"); // 1 ~ 10000, default = 1000
    system("v4l2-ctl -c auto_exposure_bias=7");      // 노출 기준값

    system("v4l2-ctl -c image_stabilization=1");    // 흔들림 방지
    system("v4l2-ctl -c iso_sensitivity=4");        // 0 ~ 4, default = 0
    system("v4l2-ctl -c iso_sensitivity_auto=1");   // 0:manual, 1:auto
    system("v4l2-ctl -c exposure_metering_mode=0"); // 0:average, 1:center, 2:spot, 3:matrix

    system("v4l2-ctl -p 20"); // fps

    // const auto t0 = chrono::high_resolution_clock::now();
    //  for(int i = 0; i < 20; ++i)
    bool flag = 0;
    vector<uchar> buffer;
    uchar header[12];
    for (;;)
    {
    	Mat frame;
        cap.read(frame);
        const auto tframe = chrono::high_resolution_clock::now().time_since_epoch().count();
        uint32_t bufsz;
	uchar* cbufsz = (uchar*)&bufsz;
        uint64_t stamp;
	uchar* cstamp = (uchar*)&stamp;

        if (frame.empty())
        {
            cerr << "ERROR! blank frame grabbed\n";
            break;
        }
        imencode(".jpg", frame, buffer);
        bufsz = buffer.size();
        stamp = tframe;

	for(int i = 0;i < 4;i++){
	    header[i] = cbufsz[3-i];
	}
	for(int i = 0;i < 8;i++){
	    header[4 + i] = cstamp[7-i];
	}

	buffer.insert(buffer.begin(), header, header+12);
	
	try{
	    mqtt::token_ptr tok = topic.publish(string(buffer.begin(), buffer.end()));
	    tok->wait();
	}
	catch(const mqtt::exception& exc){
		cerr << "Failed to send image data." << endl;
		cerr << exc << endl;
		return EXIT_FAILURE;
	}
    }

    cli.disconnect();
    cap.release();

    return EXIT_SUCCESS;
}

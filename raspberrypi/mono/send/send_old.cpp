// compile option
// g++ -std=c++14 -O2 ./send_old.cpp -o ./send_old -L/usr/local/include/opencv2/ -lopencv_videoio -lopencv_core -lopencv_imgcodecs

#include <iostream>
#include <algorithm>
#include <cstring>
#include <cstdlib>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <opencv2/opencv.hpp>

#include <opencv2/core.hpp>
#include <opencv2/videoio/videoio_c.h>
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>
#include <chrono>

#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

// #define IP_ADDRESS "192.168.219.161"
// #define IP_ADDRESS "192.168.26.33"
#define IP_ADDRESS "192.168.110.103"
#define PORT 8080

using namespace std;
using namespace cv;

int main()
{
    // create a socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1)
    {
        cerr << "Failed to create socket." << endl;
        return EXIT_FAILURE;
    }

    // specify the address and port of the server to connect to
    struct sockaddr_in server_address;
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);
    inet_pton(AF_INET, IP_ADDRESS, &server_address.sin_addr);

    // connect to the server
    if (connect(sock, (struct sockaddr *)&server_address, sizeof(server_address)) == -1)
    {
        cerr << "Failed to connect to server." << endl;
        return EXIT_FAILURE;
    }

    // send the images
    VideoCapture cap("/dev/video0");
    if (!cap.isOpened())
    {
        cerr << "Failed to open video file." << endl;
        return EXIT_FAILURE;
    }

    system("v4l2-ctl -c brightness=85"); // 0 ~ 100, default = 50
    system("v4l2-ctl -c contrast=80");   // -100 ~ 100, default = 0
    system("v4l2-ctl -c saturation=80"); // -100 ~ 100, default = 0
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
    for (;;)
    {
        Mat frame;
        cap.read(frame);
        const auto tframe = chrono::high_resolution_clock::now().time_since_epoch().count();

        struct HEADER
        {
            int bufsz;
            uint64_t stamp;
        } tmp;

        // const auto tframe = chrono::duration_cast<chrono::nanoseconds>(t - t0).count();
        // cout << tframe << " ms\n";
        if (frame.empty())
        {
            cerr << "ERROR! blank frame grabbed\n";
            break;
        }

        vector<uchar> buffer;
        buffer.reserve(200000);
        imencode(".jpg", frame, buffer);
        tmp.bufsz = buffer.size();
        tmp.stamp = tframe;
        buffer.insert(buffer.begin(), (uchar *)(&tmp), (uchar *)(&tmp) + sizeof(tmp));
        const int bytes_sent = send(sock, buffer.data(), buffer.size(), 0);
        if (bytes_sent == -1)
        {
            cerr << "Failed to send image data." << endl;
            return EXIT_FAILURE;
        }
    }

    // close the socket and video capture
    close(sock);
    cap.release();

    return EXIT_SUCCESS;
}

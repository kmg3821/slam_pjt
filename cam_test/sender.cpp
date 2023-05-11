// compile option
// g++ -std=c++14 -O2 ./sender.cpp -o ./sender -L/usr/local/include/opencv2/ -lopencv_videoio -lopencv_core -lopencv_imgcodecs

#include <iostream>
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
    server_address.sin_port = htons(8080);
    inet_pton(AF_INET, "192.168.219.161", &server_address.sin_addr);

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

    system("v4l2-ctl -c iso_sensitivity_auto=0"); // manual mode
    system("v4l2-ctl -c image_stabilization=1");  // 흔들림 방지
    system("v4l2-ctl -c scene_mode=0");           // 촬영모드, 0: None, 8: Night, 11: Sports
    system("v4l2-ctl -c sharpness=100");          // 예리함 정도
    system("v4l2-ctl -c rotate=180");             // 회전
    system("v4l2-ctl -c color_effects=0"); // gray color

    cap.set(CV_CAP_PROP_FPS, 25); // default = 20

    cap.set(CV_CAP_PROP_BRIGHTNESS, 60); // 0 ~ 100, default = 50
    cap.set(CAP_PROP_CONTRAST, 10);      // -100 ~ 100, default = 0
    cap.set(CV_CAP_PROP_SATURATION, 0);  // -100 ~ 100, default = 0

    cap.set(CAP_PROP_AUTO_EXPOSURE, 1); // 0: Auto, 1: Manual
    cap.set(CAP_PROP_EXPOSURE, 30);     // 1 ~ 10000, default = 1000
    cap.set(CV_CAP_PROP_ISO_SPEED, 4);  // 0 ~ 4, default = 0

    auto prev = chrono::steady_clock::now();
    // for(int i = 0; i < 20; ++i)
    for (;;)
    {
        Mat frame;
        cap.read(frame);
        if (frame.empty())
        {
            cerr << "ERROR! blank frame grabbed\n";
            break;
        }

        vector<uchar> buffer;
        imencode(".jpg", frame, buffer);
        int buffer_size = buffer.size();
        buffer.insert(buffer.begin(), (uchar *)(&buffer_size), (uchar *)(&buffer_size) + sizeof(int));
        int bytes_sent = send(sock, buffer.data(), buffer.size(), 0);
        if (bytes_sent == -1)
        {
            cerr << "Failed to send image data." << endl;
            return EXIT_FAILURE;
        }

        auto now = chrono::steady_clock::now();

        auto dt = chrono::duration_cast<chrono::milliseconds>(now - prev).count();
        cout << "Elapsed time in milliseconds: " << dt << " ms" << endl;
        prev = now;
    }

    // close the socket and video capture
    close(sock);
    cap.release();

    return EXIT_SUCCESS;
}

// compile option
// g++ -std=c++14 -O2 ./test.cpp -o ./test -L/usr/local/include/opencv2/ -lopencv_videoio -lopencv_core -lopencv_imgcodecs

#include <opencv2/core.hpp>
#include <opencv2/videoio/videoio_c.h>
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>
#include <iostream>
#include <chrono>
#include <unistd.h>

using namespace cv;
using namespace std;

int main()
{
    VideoCapture cap;
    cap.open("/dev/video0");

    system("v4l2-ctl -c iso_sensitivity_auto=0"); // manual mode
    system("v4l2-ctl -c image_stabilization=1");  // 흔들림 방지
    system("v4l2-ctl -c scene_mode=0");           // 촬영모드, 0: None, 8: Night, 11: Sports
    system("v4l2-ctl -c sharpness=100");          // 예리함 정도
    // system("v4l2-ctl -c color_effects=1"); // gray color

    cap.set(CV_CAP_PROP_FPS, 25); // default = 20

    cap.set(CV_CAP_PROP_BRIGHTNESS, 60); // 0 ~ 100, default = 50
    cap.set(CAP_PROP_CONTRAST, 10);       // -100 ~ 100, default = 0
    cap.set(CV_CAP_PROP_SATURATION, 0);  // -100 ~ 100, default = 0

    cap.set(CAP_PROP_AUTO_EXPOSURE, 1); // 0: Auto, 1: Manual
    cap.set(CAP_PROP_EXPOSURE, 50);     // 1 ~ 10000, default = 1000
    cap.set(CV_CAP_PROP_ISO_SPEED, 4);  // 0 ~ 4, default = 0

    if (!cap.isOpened())
    {
        cerr << "ERROR! Unable to open camera\n";
        return -1;
    }

    for (int i = 0; i < 50; ++i)
    {
        auto st = chrono::steady_clock::now();
        Mat frame;
        cap.read(frame);
        if (frame.empty())
        {
            cerr << "ERROR! blank frame grabbed\n";
            break;
        }

        char path[16];
        sprintf(path, "./image%d.jpg", i);
        imwrite(path, frame);
        auto et = chrono::steady_clock::now();

        auto dt = chrono::duration_cast<chrono::milliseconds>(et - st).count();
        cout << "Elapsed time in milliseconds: " << dt << " ms" << endl;
    }

    cap.release();

    return 0;
}
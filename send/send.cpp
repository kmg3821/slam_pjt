// compile option
// g++ -std=c++14 -O2 ./send.cpp -o ./send -L/usr/local/include/opencv2/ -lopencv_videoio -lopencv_core -lopencv_imgcodecs

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

#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

//#define IP_ADDRESS "192.168.219.161"
#define IP_ADDRESS "192.168.26.33"
#define PORT 8080

#define ACCEL_XOUT_H 0x3B
#define PWR_MGMT 0x6B
#define CONFIG 0x1A
#define SMPRT_DIV 0x19
#define FIFO_EN 0x23
#define USER_CTRL 0x6A
#define FIFO_R_W 0x74
#define FIFO_COUNT_H 0x72

using namespace std;
using namespace cv;


#define DEG2RAD 3.141592 / 180.f

using namespace std;

// // Open the I2C bus.
// int file_i2c = open("/dev/i2c-1", O_RDWR);

// inline void get_inertia(uint64_t &t, float a[3], float w[3])
// {
//   // Write a byte to the slave device.
//   unsigned char data = ACCEL_XOUT_H;
//   if (write(file_i2c, &data, 1) != 1)
//   {
//     cerr << "Failed to write to the i2c bus." << endl;
//     exit(1);
//   }

//   // Read a byte from the slave device.
//   unsigned char read_data[14] = {0};
//   if (read(file_i2c, read_data, 14) != 14)
//   {
//     cerr << "Failed to read from the i2c bus." << endl;
//     exit(1);
//   }
//   t = chrono::high_resolution_clock::now().time_since_epoch().count();

//   short ax = (read_data[0] << 8) | read_data[1];
//   short ay = (read_data[2] << 8) | read_data[3];
//   short az = (read_data[4] << 8) | read_data[5];

//   short wx = (read_data[8] << 8) | read_data[9];
//   short wy = (read_data[10] << 8) | read_data[11];
//   short wz = (read_data[12] << 8) | read_data[13];

//   w[0] = (float)wx * DEG2RAD / 131.f;
//   w[1] = (float)wy * DEG2RAD / 131.f;
//   w[2] = (float)wz * DEG2RAD / 131.f;

//   a[0] = (float)ax * 9.81 / 16384.f;
//   a[1] = (float)ay * 9.81 / 16384.f;
//   a[2] = (float)az * 9.81 / 16384.f;
// }

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

    /////////////////////////////////////////////////////////////////////
    // Open the I2C bus.
    int file_i2c = open("/dev/i2c-1", O_RDWR);
    if (file_i2c < 0)
    {
        cerr << "Failed to open the i2c bus" << endl;
        return 1;
    }

    // Set the I2C address of the slave device.
    int addr = 0x68;
    if (ioctl(file_i2c, I2C_SLAVE, addr) < 0)
    {
        cerr << "Failed to acquire bus access and/or talk to slave." << endl;
        return 1;
    }

    // Set to use the internal oscillator
    {
        uint8_t buf[2] = {PWR_MGMT, 0x00}; // {address, value}
        if (write(file_i2c, buf, 2) != 2)
        {
            cerr << "Failed to write to the i2c bus." << endl;
            return 1;
        }
    }

    // Set to use the lowpass filter
    {
        uint8_t buf[2] = {CONFIG, 0x01}; // {address, value}
        if (write(file_i2c, buf, 2) != 2)
        {
            cerr << "Failed to write to the i2c bus." << endl;
            return 1;
        }
    }

    // Set sampling rate to 200Hz
    {
        uint8_t buf[2] = {SMPRT_DIV, 0x04}; // {address, value}
        if (write(file_i2c, buf, 2) != 2)
        {
            cerr << "Failed to write to the i2c bus." << endl;
            return 1;
        }
    }

    // Set to enable FIFO
    {
        uint8_t buf[2] = {USER_CTRL, 0x44}; // {address, value}
        if (write(file_i2c, buf, 2) != 2)
        {
            cerr << "Failed to write to the i2c bus." << endl;
            return 1;
        }
    }

    // Set to use FIFO buffer
    {
        uint8_t buf[2] = {FIFO_EN, 0x78}; // {address, value}
        if (write(file_i2c, buf, 2) != 2)
        {
            cerr << "Failed to write to the i2c bus." << endl;
            return 1;
        }
    }
    /////////////////////////////////////////////////////////////////////

    const auto t0 = chrono::high_resolution_clock::now();
    // for(int i = 0; i < 20; ++i)
    for (;;)
    {
        Mat frame;
        cap.read(frame);
        const auto t = chrono::high_resolution_clock::now();

        struct HEADER
        {
            int imusz;
            int bufsz;
            uint64_t stamp;
            unsigned char imu[12 * 1000];
        } tmp;

        ///////////////
        // Write a byte to the slave device.
        {
            unsigned char data = FIFO_COUNT_H;
            if (write(file_i2c, &data, 1) != 1)
            {
                cerr << "Failed to write to the i2c bus." << endl;
                return 1;
            }

            // Read a byte from the slave device.
            unsigned char buf[2] = {0};
            if (read(file_i2c, buf, 2) != 2)
            {
                cerr << "Failed to read from the i2c bus." << endl;
                return 1;
            }
            tmp.imusz = (buf[0] << 8) | buf[1];
        }
        // Write a byte to the slave device.
        {
            unsigned char data = FIFO_R_W;
            if (write(file_i2c, &data, 1) != 1)
            {
                cerr << "Failed to write to the i2c bus." << endl;
                return 1;
            }

            // Read a byte from the slave device.
            if (read(file_i2c, tmp.imu, tmp.imusz) != tmp.imusz)
            {
                cerr << "Failed to read from the i2c bus." << endl;
                return 1;
            }
        }
        //////////////

        const auto tframe = chrono::duration_cast<chrono::milliseconds>(t - t0).count();
        //cout << tframe << " ms\n";
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
        buffer.insert(buffer.begin(), (uchar *)(&tmp), (uchar *)(&tmp) + sizeof(tmp.imusz) + sizeof(tmp.bufsz) + sizeof(tmp.stamp) + tmp.imusz);
        const int bytes_sent = send(sock, buffer.data(), buffer.size(), 0);
        if (bytes_sent == -1)
        {
            cerr << "Failed to send image data." << endl;
            return EXIT_FAILURE;
        }
        cout << tmp.imusz << '\n';
    }

    // close the socket and video capture
    close(sock);
    cap.release();

    return EXIT_SUCCESS;
}

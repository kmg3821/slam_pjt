#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <opencv2/opencv.hpp>
#include <string.h>
#include <sys/ioctl.h>
#include <chrono>
#include <System.h>
#include "ImuTypes.h"
#include <opencv2/core/core.hpp>

#define PORT 8080
#define VOCA_PATH "/home/kmg/ORB_SLAM3/Vocabulary/ORBvoc.txt"
#define CAM_INTRINSIC "/home/kmg/slam_pjt/slam/cam_imu_intrinsic.yaml"
#define DEGTORAD 3.141592 / 180.f

using namespace std;
using namespace cv;

double ttrack_tot = 0;
int main(int argc, char *argv[])
{
    ORB_SLAM3::System SLAM(VOCA_PATH, CAM_INTRINSIC, ORB_SLAM3::System::IMU_MONOCULAR, true);

    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    // create socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("socket failed");
        return EXIT_FAILURE;
    }

    // set socket options
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
    {
        perror("setsockopt failed");
        return EXIT_FAILURE;
    }

    // set socket receive buffer
    int buf_size = 200000 * 1000;
    if (setsockopt(server_fd, SOL_SOCKET, SO_RCVBUF, &buf_size, sizeof(buf_size)))
    {
        perror("setsockopt failed");
        return EXIT_FAILURE;
    }

    // set server address
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // bind socket to address
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("bind failed");
        return EXIT_FAILURE;
    }

    // listen for incoming connections
    if (listen(server_fd, 3) < 0)
    {
        perror("listen failed");
        return EXIT_FAILURE;
    }

    // accept incoming connection
    if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
    {
        perror("accept failed");
        return EXIT_FAILURE;
    }

    while (true)
    {
        struct HEADER
        {
            int imu_size;
            int img_size;
            uint64_t stamp;
        } tmp;

        int bytes_available = 0;
        while (bytes_available < sizeof(tmp))
            ioctl(new_socket, FIONREAD, &bytes_available);

        if (read(new_socket, &tmp, sizeof(tmp)) <= 0)
        {
            perror("read failed");
            break;
        }
        // cout << tmp.img_size << '\n';

        bytes_available = 0;
        unsigned char read_data[12 * 1000];
        while (bytes_available < tmp.imu_size)
            ioctl(new_socket, FIONREAD, &bytes_available);
        if (read(new_socket, &read_data, tmp.imu_size) <= 0)
        {
            perror("read failed");
            break;
        }

        bytes_available = 0;
        while (bytes_available < tmp.img_size)
            ioctl(new_socket, FIONREAD, &bytes_available);

        vector<uchar> data(tmp.img_size);
        if (read(new_socket, data.data(), tmp.img_size) <= 0)
        {
            perror("read failed");
            break;
        }

        // decode image data
        Mat image = imdecode(data, IMREAD_COLOR);
        if (image.empty())
        {
            cerr << "Failed to decode image." << endl;
            break;
        }

        // Load imu measurements from previous frame
        vector<ORB_SLAM3::IMU::Point> vImuMeas;
        int cnt = tmp.imu_size / 12;
        for (int i = 0; i < cnt; ++i)
        {
            short ax = (read_data[12 * i + 0] << 8) | read_data[12 * i + 1];
            short ay = (read_data[12 * i + 2] << 8) | read_data[12 * i + 3];
            short az = (read_data[12 * i + 4] << 8) | read_data[12 * i + 5];
            short wx = (read_data[12 * i + 6] << 8) | read_data[12 * i + 7];
            short wy = (read_data[12 * i + 8] << 8) | read_data[12 * i + 9];
            short wz = (read_data[12 * i + 10] << 8) | read_data[12 * i + 11];

            double w[3], a[3];
            w[0] = (double)wx * DEGTORAD / 131.f;
            w[1] = (double)wy * DEGTORAD / 131.f;
            w[2] = (double)wz * DEGTORAD / 131.f;
            a[0] = (double)ax * 9.81 / 16384.f;
            a[1] = (double)ay * 9.81 / 16384.f;
            a[2] = (double)az * 9.81 / 16384.f;
            double imu_stamp = (tmp.stamp - (cnt - i - 1) * 5000000) * 1e-9;

            //cout << imu_stamp << "(" << a[0] << "," << a[1] << "," << a[2] << ")"
            //     << "(" << w[0] << "," << w[1] << "," << w[2] << ")\n"; 

            vImuMeas.push_back(ORB_SLAM3::IMU::Point(a[0],a[1],a[2],w[0],w[1],w[2],imu_stamp));           
        }
        cout << vImuMeas.size() << '\n';

        // const auto t1 = chrono::steady_clock::now();
        SLAM.TrackMonocular(image, (double)tmp.stamp*1e-9, vImuMeas);
        // const auto t2 = chrono::steady_clock::now();
        // auto dt = chrono::duration_cast<chrono::milliseconds>(t2 - t1).count();
        // cout << "Elapsed time in milliseconds: " << dt << "ms\n";
    }
    SLAM.Shutdown();

    close(new_socket);
    close(server_fd);
}
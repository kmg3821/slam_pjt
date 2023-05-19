
#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <opencv2/opencv.hpp>
#include <string.h>
#include <sys/ioctl.h>
#include <chrono>
#include <System.h>
#include<omp.h>

#include <fstream>
#include <thread>

#define PORT 8080
#define VOCA_PATH "/home/kmg/ORB_SLAM3/Vocabulary/ORBvoc.txt"
#define CAM_INTRINSIC "/home/kmg/slam_pjt/slam/cam_intrinsic.yaml"

using namespace std;
using namespace cv;

bool flag[2] = {1, 1};
void foo(ORB_SLAM3::System &SLAM)
{
    const int cell_size = 1000;
    cv::Mat canvas(cell_size, cell_size, CV_8UC3, cv::Scalar(0, 0, 0)); // Creating a blank canvas
    float res = 0.005; // 0.005 m/cell

    while (flag[0])
    {
        //if(SLAM.MapChanged())
        canvas.setTo(0);


        const auto kfs = SLAM.mpAtlas->GetAllKeyFrames();
        int kfs_len = kfs.size();

        #pragma omp parallel for schedule(dynamic,1) collapse(2)
        for (int i = 0; i < kfs_len; ++i)
        {
            const auto p0 = kfs[i]->GetPose().translation();
            const int x0 = (p0(0) / res) + 250;
            const int y0 = (p0(1) / res) + 250;
            for (const auto mp : kfs[i]->GetMapPoints())
            {
                const auto p = mp->GetWorldPos();
            
                const int x = (p(0) / res) + cell_size / 2;
                const int y = (p(1) / res) + cell_size / 2;

                const Point startPoints(x0, y0);
                const Point endPoints(x, y);
                line(canvas, startPoints, endPoints, Scalar(255, 0, 0), 1);
            }
        }
        imshow("Canvas", canvas);
        waitKey(1);

        usleep(200 * 1000);
    }
}

int main()
{
    ORB_SLAM3::System SLAM(VOCA_PATH, CAM_INTRINSIC, ORB_SLAM3::System::MONOCULAR, true);

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

    thread thPoints(foo, ref(SLAM));

    for (int i = 0;;)
    {
        struct HEADER
        {
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

        // const auto t1 = chrono::steady_clock::now();
        SLAM.TrackMonocular(image, (double)tmp.stamp * 1e-9); // TODO change to monocular_inertial
        flag[1] = 0;
        // SLAM.TrackMonocular(image, tmp.stamp); // TODO change to monocular_inertial
        // const auto t2 = chrono::steady_clock::now();
        // auto dt = chrono::duration_cast<chrono::milliseconds>(t2 - t1).count();
        // cout << "Elapsed time in milliseconds: " << dt << "ms\n";
    }
    flag[0] = 0;
    flag[1] = 0;
    thPoints.join();
    close(new_socket);
    close(server_fd);

    SLAM.Shutdown();

    return EXIT_SUCCESS;
}

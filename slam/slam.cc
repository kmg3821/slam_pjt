
#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <opencv2/opencv.hpp>
#include <string.h>
#include <sys/ioctl.h>
#include <chrono>
#include <System.h>
#include <fstream>
#include <thread>

#include <OcTree.h>
// #include <ViewerGui.h>
// #include <QtGui>
// #include <QApplication>

#define PORT 8080
#define VOCA_PATH "/home/kmg/ORB_SLAM3/Vocabulary/ORBvoc.txt"
#define CAM_INTRINSIC "/home/kmg/slam_pjt/slam/cam_intrinsic.yaml"

using namespace std;
using namespace cv;
using namespace octomap;

// void bar()
// {
//     int argc = 1;
//     char* argv[] = {"simple_tree.bt"};
//     QApplication app(argc, argv);
//     ViewerGui gui("simple_tree.bt", NULL, 0);
//     gui.show();
//     app.exec();
// }

bool flag;
void foo(const ORB_SLAM3::System &SLAM)
{
    OcTree ot(0.01);
    const point3d x0(0, 0, 0);

    while (flag)
    {
        const auto mp = SLAM.mpAtlas->GetAllMapPoints();
        const int len = mp.size();
        Pointcloud pc;
        for (int i = 0; i < len; ++i)
        {
            const auto tmp = mp[i]->GetWorldPos();
            pc.push_back(tmp[0], tmp[1], tmp[2]);
        }
        ot.insertPointCloud(pc, x0);
        ot.writeBinary("simple_tree.bt");

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

    flag = 1;
    thread thPoints(foo, ref(SLAM));
    //thread thGui(bar);
    system("~/octomap/bin/octovis ~/slam_pjt/slam/simple_tree.bt");

    for (int i = 0;; ++i)
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
        // SLAM.TrackMonocular(image, tmp.stamp); // TODO change to monocular_inertial
        // const auto t2 = chrono::steady_clock::now();
        // auto dt = chrono::duration_cast<chrono::milliseconds>(t2 - t1).count();
        // cout << "Elapsed time in milliseconds: " << dt << "ms\n";
    }
    flag = 0;
    thPoints.join();
    close(new_socket);
    close(server_fd);
    //thGui.join();

    SLAM.Shutdown();

    return EXIT_SUCCESS;
}

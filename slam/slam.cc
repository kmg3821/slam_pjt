
#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <opencv2/opencv.hpp>
#include <string.h>
#include <sys/ioctl.h>
#include <chrono>
#include <System.h>

#include <omp.h>
#include <atomic>
#include <thread>

#define PORT 8080
#define VOCA_PATH "/home/kmg/ORB_SLAM3/Vocabulary/ORBvoc.txt"
#define CAM_INTRINSIC "/home/kmg/slam_pjt/slam/cam_intrinsic.yaml"

using namespace std;
using namespace cv;

const int cell_size = 1000;
atomic<unsigned long long> atomic_cnts[2][cell_size][cell_size]; // 0:visited, 1:occupied

bool check_boundary(int r, int c)
{
    if (r < 0 || c < 0 || r >= cell_size || c >= cell_size)
        return false;
    return true;
}

void bresenham(int x1, int y1, int x2, int y2)
{
    atomic_cnts[1][y2][x2].fetch_add(1);
    if (x1 == x2)
    {
        if (y1 > y2)
            swap(y1, y2);

        while (y1 <= y2)
        {
            atomic_cnts[0][y1][x1].fetch_add(1);
            y1++;
        }
    }
    else
    {
        if (x1 > x2)
        {
            swap(x1, x2);
            swap(y1, y2);
        }
        if (y1 == y2)
        {
            while (x1 <= x2)
            {
                atomic_cnts[0][y1][x1].fetch_add(1);
                x1++;
            }
        }
        else
        {
            if (y1 > y2)
            {
                int y0 = y1;
                y2 = y1 + (y1 - y2);

                int dx = x2 - x1;
                int dy = y2 - y1;
                int p = 2 * dy - dx;
                while (x1 <= x2)
                {
                    atomic_cnts[0][y0 - (y1 - y0)][x1].fetch_add(1);
                    x1++;
                    if (p < 0)
                    {
                        p = p + 2 * dy;
                    }
                    else
                    {
                        p = p + 2 * dy - 2 * dx;
                        y1++;
                    }
                }
            }
            else
            {
                int dx = x2 - x1;
                int dy = y2 - y1;
                int p = 2 * dy - dx;
                while (x1 <= x2)
                {
                    atomic_cnts[0][y1][x1].fetch_add(1);
                    x1++;
                    if (p < 0)
                    {
                        p = p + 2 * dy;
                    }
                    else
                    {
                        p = p + 2 * dy - 2 * dx;
                        y1++;
                    }
                }
            }
        }
    }
}

void drawOccupancyMap(Mat &canvas)
{

#pragma omp parallel for schedule(dynamic, 1) collapse(2)
    for (int i = 0; i < cell_size; ++i)
    {
        for (int j = 0; j < cell_size; ++j)
        {
            if (atomic_cnts[0][i][j] == 0)
                continue;
            const int prob = 100 - (atomic_cnts[1][i][j] * 100) / atomic_cnts[0][i][j]; // 비어있는 확률
            if (prob < 20)
            {
                circle(canvas, Point(j, i), 0, Scalar(0, 0, 0), 5);
            }
            else if (prob > 80)
            {
                circle(canvas, Point(j, i), 0, Scalar(255, 255, 255));
            }
        }
    }
}

bool flag[2] = {1, 1};
Sophus::Vector3f now;
void foo(ORB_SLAM3::System &SLAM)
{
    Mat canvas(cell_size, cell_size, CV_8UC3, cv::Scalar(120, 120, 120)); // Creating a blank canvas
    const float res = 0.01;                                               // 0.005 m/cell

    while (flag[0])
    {
        canvas.setTo(cv::Scalar(120, 120, 120));

        if (SLAM.isLost() || SLAM.MapChanged())
        {
#pragma omp parallel for schedule(dynamic, 1) collapse(2)
            for (int i = 0; i < cell_size; ++i)
            {
                for (int j = 0; j < cell_size; ++j)
                {
                    atomic_cnts[0][i][j].store(0);
                    atomic_cnts[1][i][j].store(0);
                }
            }
        }

        const auto kfs = SLAM.mpAtlas->GetAllKeyFrames();
        SLAM.GetTrackedMapPoints()
        int kfs_len = kfs.size();

#pragma omp parallel for schedule(dynamic, 1) collapse(2)
        for (int i = 0; i < kfs_len; ++i)
        {
            const auto p0 = kfs[i]->GetOw();
            const int x0 = (int)(p0(0) / res) + cell_size / 2;
            const int y0 = (int)(-p0(2) / res) + cell_size / 2;
            for (const auto mp : kfs[i]->GetMapPoints())
            {
                const auto p = mp->GetWorldPos();
                const int x = (int)(p(0) / res) + cell_size / 2;
                const int y = (int)(-p(2) / res) + cell_size / 2;

                // const Point startPoints(x0, y0);
                // const Point endPoints(x, y);
                // line(canvas, startPoints, endPoints, Scalar(255, 255, 255), 1);
                if (!check_boundary(x0, y0) || !check_boundary(x, y))
                    continue;
                bresenham(x0, y0, x, y);
            }
        }

        drawOccupancyMap(canvas);

        const int x = (int)(now(0) / res) + cell_size / 2;
        const int y = (int)(-now(2) / res) + cell_size / 2;
        circle(canvas, Point(x, y), 0, cv::Scalar(0, 0, 255), 15);
        cout << "(" << y << "," << x << ")\n";

        // for (int i = 0; i < kfs_len; ++i)
        // {
        //     const auto p0 = kfs[i]->GetPose().translation();
        //     cout << p0.transpose() << endl;
        // }

        imshow("Canvas", canvas);
        waitKey(1);

        usleep(100 * 1000);
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
        now = SLAM.TrackMonocular(image, (double)tmp.stamp * 1e-9).inverse().translation(); // TODO change to monocular_inertial
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

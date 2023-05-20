
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

const int cell_size = 800;
atomic<unsigned long long> atomic_cnts[2][cell_size][cell_size]; // 0:visited, 1:occupied

bool check_boundary(int r, int c)
{
    if (r < 0 || c < 0 || r >= cell_size || c >= cell_size)
        return false;
    return true;
}
void bresenham(int r1, int c1, int r2, int c2)
{
    if (check_boundary(r2, c2))
        atomic_cnts[1][r2][c2].fetch_add(1);
    if (c1 == c2)
    {
        if (r1 > r2)
            swap(r1, r2);

        while (r1 <= r2)
        {
            if (!check_boundary(r1, c1))
                break;
            atomic_cnts[0][r1][c1].fetch_add(1);
            r1++;
        }
    }
    else
    {
        if (c1 > c2)
        {
            swap(c1, c2);
            swap(r1, r2);
        }
        if (r1 == r2)
        {
            while (c1 <= c2)
            {
                if (!check_boundary(r1, c1))
                    break;
                atomic_cnts[0][r1][c1].fetch_add(1);
                c1++;
            }
        }
        else
        {
            if (r1 > r2)
            {
                r2 = r1 + (r1 - r2);

                int dr = r2 - r1;
                int dc = c2 - c1;

                if (dr <= dc)
                {
                    const int r0 = r1;
                    int p = 2 * dr - dc;
                    while (c1 <= c2)
                    {
                        if (!check_boundary(r0 - (r1 - r0), c1))
                            break;
                        atomic_cnts[0][r0 - (r1 - r0)][c1].fetch_add(1);
                        c1++;
                        if (p < 0)
                            p = p + 2 * dr;
                        else
                        {
                            p = p + 2 * dr - 2 * dc;
                            r1++;
                        }
                    }
                }
                else
                {
                    swap(dr, dc);
                    swap(c1, r1);
                    swap(c2, r2);
                    int p = 2 * dr - dc;
                    const int c0 = c1;
                    while (c1 <= c2)
                    {
                        if (!check_boundary(c0 - (c1 - c0), r1))
                            break;
                        atomic_cnts[0][c0 - (c1 - c0)][r1].fetch_add(1);
                        c1++;
                        if (p < 0)
                            p = p + 2 * dr;
                        else
                        {
                            p = p + 2 * dr - 2 * dc;
                            r1++;
                        }
                    }
                }
            }
            else
            {
                int dr = r2 - r1;
                int dc = c2 - c1;

                if (dc >= dr)
                {
                    int p = 2 * dr - dc;
                    while (c1 <= c2)
                    {
                        if (!check_boundary(r1, c1))
                            break;
                        atomic_cnts[0][r1][c1].fetch_add(1);
                        c1++;
                        if (p < 0)
                            p = p + 2 * dr;
                        else
                        {
                            p = p + 2 * dr - 2 * dc;
                            r1++;
                        }
                    }
                }
                else
                {
                    swap(dr, dc);
                    swap(c1, r1);
                    swap(c2, r2);
                    int p = 2 * dr - dc;
                    while (c1 <= c2)
                    {
                        if (!check_boundary(c1, r1))
                            break;
                        atomic_cnts[0][c1][r1].fetch_add(1);
                        c1++;
                        if (p < 0)
                            p = p + 2 * dr;
                        else
                        {
                            p = p + 2 * dr - 2 * dc;
                            r1++;
                        }
                    }
                }
            }
        }
    }
}

void drawOccupancyMap(Mat &canvas)
{

#pragma omp parallel for schedule(dynamic, 1) collapse(4)
    for (int i = 0; i < cell_size; ++i)
    {
        for (int j = 0; j < cell_size; ++j)
        {
            int visit_cnt = 0;
            int occupy_cnt = 0;
            for (int dr = -1; dr <= 1; ++dr)
            {
                for (int dc = -1; dc <= 1; ++dc)
                {
                    if (!check_boundary(i + dr, j + dc))
                        continue;
                    visit_cnt += atomic_cnts[0][i + dr][j + dc];
                    occupy_cnt += atomic_cnts[1][i + dr][j + dc];
                }
            }
            if(visit_cnt < 5) continue;

            const int percent = (occupy_cnt * 100) / visit_cnt;
            if (percent >= 15)
            {
                circle(canvas, Point(j, i), 0, Scalar(0, 0, 0), 3);
            }
            else
            {
                circle(canvas, Point(j, i), 0, Scalar(255, 255, 255));
            }
        }
    }
}

bool flag = 1;
Sophus::Vector3f now;
void foo(ORB_SLAM3::System &SLAM)
{
    Mat canvas(cell_size, cell_size, CV_8UC3, cv::Scalar(120, 120, 120)); // Creating a blank canvas
    const float res = 0.01;                                               // 0.01 m/cell

    while (flag)
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

        canvas.setTo(cv::Scalar(120, 120, 120));

        const auto mps = SLAM.mpAtlas->GetAllMapPoints();
        const int mps_len = mps.size();

#pragma omp parallel for schedule(dynamic, 1)
        for (int i = 0; i < mps_len; ++i)
        {
            const auto p = mps[i]->GetWorldPos();
            const int c = cell_size / 2 + (int)(p(0) / res); // x
            const int r = cell_size / 2 - (int)(p(2) / res); // y

            const auto p0 = mps[i]->GetReferenceKeyFrame()->GetPose().inverse().translation();
            const int c0 = cell_size / 2 + (int)(p0(0) / res); // x
            const int r0 = cell_size / 2 - (int)(p0(2) / res); // y

            bresenham(r0, c0, r, c);
        }

        drawOccupancyMap(canvas);

        const int c = cell_size / 2 + (int)(now(0) / res); // x
        const int r = cell_size / 2 - (int)(now(2) / res); // y
        if (check_boundary(r, c))
            circle(canvas, Point(c, r), 0, cv::Scalar(0, 0, 255), 10);

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
    const int opt = 1;
    const int addrlen = sizeof(address);

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
    const int buf_size = 200000 * 1000;
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

    for (;;)
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
        // const auto t2 = chrono::steady_clock::now();
        // auto dt = chrono::duration_cast<chrono::milliseconds>(t2 - t1).count();
        // cout << "Elapsed time in milliseconds: " << dt << "ms\n";
    }
    flag = 0;
    thPoints.join();
    close(new_socket);
    close(server_fd);

    SLAM.Shutdown();

    return EXIT_SUCCESS;
}

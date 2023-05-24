#include <iostream>
#include <fstream>
#include <unistd.h>
#include <string>
#include <chrono>
#include <System.h>

#include <opencv2/opencv.hpp>
#include <omp.h>
#include <atomic>
#include <thread>
#include <mqtt/async_client.h>
#include <nlohmann/json.hpp>

#define VOCA_PATH "/home/kmg/ORB_SLAM3/Vocabulary/ORBvoc.txt"
#define CAM_INTRINSIC "./cam_intrinsic.yaml"

using namespace std;
using namespace cv;
using json = nlohmann::json;

const int cell_size = 800;
atomic<unsigned long long> atomic_cnts[2][cell_size][cell_size]; // 0:visited, 1:occupied
bool flag = 2;
Sophus::SE3f now;
bool check_boundary(int r, int c);
void bresenham(int r1, int c1, int r2, int c2);
void drawOccupancyMap(Mat &canvas);
void occupancy_grid(ORB_SLAM3::System &SLAM);

int main()
{
    ORB_SLAM3::System SLAM(VOCA_PATH, CAM_INTRINSIC, ORB_SLAM3::System::MONOCULAR, true);

    ifstream configFile("../../config/config.txt");
    string line;
    string server_ip;
    string server_port;
    while(getline(configFile, line)){
    	if (line.substr(0, 15) == "MQTT_BROKER_IP="){
		server_ip = line.substr(15);
	}
	if (line.substr(0, 17) == "MQTT_BROKER_PORT="){
		server_port = line.substr(17);
	}
    }
    const string server_address = "tcp://" + server_ip + ":" + server_port;
    const string client_id = "kmg_sub";
    const string topic = "img";
    const int qos = 0;

    mqtt::async_client cli(server_address, client_id);
    mqtt::connect_options conn_opts;
    // conn_opts.set_keep_alive_interval(20);
    conn_opts.set_clean_session(true);

    cli.connect(conn_opts)->wait();
    cli.subscribe(topic, qos)->wait();
    cli.start_consuming();

    thread thPoints(occupancy_grid, ref(SLAM));
    cout << server_address << endl;

    for (;;)
    {
        uint32_t bufsz;
        uchar* cbufsz = (uchar*)&bufsz;
        uint64_t stamp;
        uchar* cstamp = (uchar*)&stamp;

        const mqtt::const_message_ptr msg_ptr = cli.consume_message();
        const auto data = msg_ptr->to_string();
        for(int i = 0; i < 4; i++)
        {
            cbufsz[i] = data[3 - i];
        }
        for(int i = 0; i < 8; i++)
        {
            cstamp[i] = data[sizeof(bufsz) + sizeof(stamp) - 1 - i];
        }
        const vector<uchar> buffer(data.begin() + sizeof(bufsz) + sizeof(stamp), data.end());

        // decode image data
        Mat image = imdecode(buffer, IMREAD_COLOR);
        if (image.empty())
        {
            cerr << "Failed to decode image." << endl;
            break;
        }

        // const auto t1 = chrono::steady_clock::now();
        now = SLAM.TrackMonocular(image, (double)stamp * 1e-9).inverse();
        // const auto t2 = chrono::steady_clock::now();
        // auto dt = chrono::duration_cast<chrono::milliseconds>(t2 - t1).count();
        // cout << "Elapsed time in milliseconds: " << dt << "ms\n";
    }
    flag = 0;
    thPoints.join();
    cli.disconnect();
    SLAM.Shutdown();

    return EXIT_SUCCESS;
}

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
            if (visit_cnt < 5)
                continue;

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

void occupancy_grid(ORB_SLAM3::System &SLAM)
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

        const Sophus::Vector3f trans = now.translation();
        const Sophus::Vector3f dir = now.rotationMatrix().col(2);
        const int c = cell_size / 2 + (int)(trans(0) / res); // x
        const int r = cell_size / 2 - (int)(trans(2) / res); // y
        const int ratio[2] = {(int)(dir(0) / res), -(int)(dir(2) / res)}; // y
        const int dc = (20*ratio[0]) / (abs(ratio[0]) + abs(ratio[1]));
        const int dr = (20*ratio[1]) / (abs(ratio[0]) + abs(ratio[1]));
        if (check_boundary(r, c))
        {
            circle(canvas, Point(c, r), 0, Scalar(0, 0, 255), 10);
            arrowedLine(canvas, Point(c,r), Point(c + dc, r + dr), Scalar(0,0,255),2, LINE_8, 0, 0.5);
        }

        imshow("Canvas", canvas);
        waitKey(1);

        usleep(100 * 1000);
    }
}

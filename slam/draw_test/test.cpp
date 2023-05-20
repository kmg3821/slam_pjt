// g++ -std=c++14 -O2 ./test.cpp -o ./test -L/usr/local/include/opencv2/ -lopencv_imgproc -lopencv_core -lopencv_imgcodecs -lopencv_highgui

#include <unistd.h>
#include <opencv2/opencv.hpp>
#include <string.h>
#include <chrono>

#include <omp.h>
#include <atomic>
#include <thread>

using namespace std;
using namespace cv;

const int cell_size = 1000;
atomic<unsigned long long> atomic_cnts[2][cell_size][cell_size]; // 0:visited, 1:occupied
void bresenham(int r1, int c1, int r2, int c2)
{
    atomic_cnts[1][r2][c2].fetch_add(1);
    if (c1 == c2)
    {
        if (r1 > r2)
            swap(r1, r2);

        while (r1 <= r2)
        {
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
                    int r0 = r1;
                    int p = 2 * dr - dc;
                    while (c1 <= c2)
                    {
                        atomic_cnts[0][r0 - (r1 - r0)][c1].fetch_add(1);
                        c1++;
                        if (p < 0)
                        {
                            p = p + 2 * dr;
                        }
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
                    int c0 = c1;
                    while (c1 <= c2)
                    {
                        atomic_cnts[0][c0 - (c1 - c0)][r1].fetch_add(1);
                        c1++;
                        if (p < 0)
                        {
                            p = p + 2 * dr;
                        }
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
                        atomic_cnts[0][r1][c1].fetch_add(1);
                        c1++;
                        if (p < 0)
                        {
                            p = p + 2 * dr;
                        }
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
                        atomic_cnts[0][c1][r1].fetch_add(1);
                        c1++;
                        if (p < 0)
                        {
                            p = p + 2 * dr;
                        }
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

int main()
{
    Mat canvas(cell_size, cell_size, CV_8UC3, cv::Scalar(120, 120, 120)); // Creating a blank canvas
    const float res = 0.01; // m/cell

    const int c0 = cell_size / 2 + (int)(2.0f / res); // x
    const int r0 = cell_size / 2 - (int)(0.1f / res); // y

    const int c = cell_size / 2 + (int)(1.0f / res); // x
    const int r = cell_size / 2 - (int)(-4.0f / res); // y

    bresenham(r0, c0, r, c);

    drawOccupancyMap(canvas);

    imshow("Canvas", canvas);
    waitKey(2000);
}

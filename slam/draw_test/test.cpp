// g++ -std=c++14 -O2 ./test.cpp -o ./test -L/usr/local/include/opencv2/ -lopencv_imgproc -lopencv_core -lopencv_imgcodecs -lopencv_highgui

#include <opencv2/opencv.hpp>

int main()
{
    cv::Mat canvas(500, 500, CV_8UC3, cv::Scalar(0, 0, 0)); // Creating a blank canvas
    std::vector<cv::Point> startPoints = {cv::Point(50, 50), cv::Point(100, 200), cv::Point(200, 300)};
    std::vector<cv::Point> endPoints = {cv::Point(400, 50), cv::Point(200, 400), cv::Point(400, 400)};

    for (size_t i = 0; i < startPoints.size(); i++)
    {
        cv::line(canvas, startPoints[i], endPoints[i], cv::Scalar(255, 0, 0), 2);

        cv::imshow("Canvas", canvas);
        cv::waitKey(500);
    }
}

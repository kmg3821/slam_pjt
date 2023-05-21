// compile option
// g++ -std=c++14 -O2 ./receive.cpp -o ./receive -L/usr/local/include/opencv2/ -lopencv_videoio -lopencv_core -lopencv_imgcodecs -lopencv_highgui

#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <opencv2/opencv.hpp>
#include <string>
#include <sys/ioctl.h>
#include <chrono>

#include <mqtt/async_client.h>
#include <nlohmann/json.hpp>

using namespace std;
using namespace cv;
using json = nlohmann::json;

int main()
{
    const string server_address = "tcp://localhost:8080";
    const string client_id = "kmg_subsriber";
    const string topic = "img";
    const int qos = 1;

    mqtt::async_client cli(server_address, client_id);
    mqtt::connect_options conn_opts;
    conn_opts.set_keep_alive_interval(20);
    conn_opts.set_clean_session(true);

    cli.connect(conn_opts)->wait();
    cli.subscribe(topic, qos)->wait();

    // receive images from client
    int idx = 0;
    int cnt = 0;
    auto prev = chrono::steady_clock::now();
    while (true)
    {
        const mqtt::const_message_ptr msg_ptr = cli.consume_message();
        json msg = json::parse(msg_ptr->to_string());

        // decode image data
        Mat image = imdecode(json::to_bjdata(msg["image"]), IMREAD_COLOR);
        if (image.empty())
        {
            cerr << "Failed to decode image." << endl;
            break;
        }

        imshow("test", image);
        waitKey(1);
    }

    return EXIT_SUCCESS;
}

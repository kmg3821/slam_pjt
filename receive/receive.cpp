// compile option
// g++ -std=c++14 -O2 ./receive.cpp -o ./receive -L/usr/local/include/opencv2/ -lopencv_videoio -lopencv_core -lopencv_imgcodecs -lopencv_highgui

#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <opencv2/opencv.hpp>
#include <string.h>
#include <sys/ioctl.h>
#include <chrono>

#define PORT 8080

using namespace std;
using namespace cv;

int main()
{
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[1024] = {0};

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
    int buf_size = 60000 * 1000;
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

    // receive images from client
    int idx = 0;
    auto prev = chrono::steady_clock::now();
    int cnt = 0;
    while (true)
    {
        // receive image size
        int img_size = 0;
        if (read(new_socket, &img_size, sizeof(int)) <= 0)
        {
            perror("read failed");
            break;
        }
        cout << img_size << endl;

        int bytes_available = 0;
        while (bytes_available < img_size)
            ioctl(new_socket, FIONREAD, &bytes_available);

        vector<uchar> data(img_size);
        if (read(new_socket, data.data(), img_size) <= 0)
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

        char tmp[30];
        if(cnt == 75)
        {
            sprintf(tmp, "./image%d.jpg", idx++);
            imwrite(tmp, image);
            cnt = 0;
        }
        imshow("test", image);
        waitKey(1);
        cnt++;

        auto now = chrono::steady_clock::now();
        auto dt = chrono::duration_cast<chrono::milliseconds>(now - prev).count();
        cout << "Elapsed time in milliseconds: " << dt << "ms" << endl;
        prev = now;
    }

    // close sockets
    close(new_socket);
    close(server_fd);

    return EXIT_SUCCESS;
}

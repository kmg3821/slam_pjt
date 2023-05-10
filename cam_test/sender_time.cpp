// compile option
// g++ -std=c++14 -O2 ./sender_time.cpp -o sender_time

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/videodev2.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <time.h>

#define BUFFER_SIZE 640 * 480 * 3 // Size of the image buffer
#define PORT 8080                 // Port number to send data to
#define IP_ADDRESS "192.168.219.159"    // IP address of the server

// Custom struct that combines the timestamp and image data
struct ImagePacket
{
    uint64_t timestamp;
    uint8_t data[BUFFER_SIZE];
};

int main()
{
    int fd, sock, ret, counter = 0;
    struct v4l2_capability caps;
    struct v4l2_format fmt;
    struct v4l2_requestbuffers req;
    struct v4l2_buffer buf;
    fd_set fds;
    struct timeval tv;
    void *buffer;
    struct sockaddr_in server_addr;
    ImagePacket packet;

    // Open the video device
    fd = open("/dev/video0", O_RDWR);
    if (fd < 0)
    {
        perror("Failed to open video device");
        exit(EXIT_FAILURE);
    }

    // Check if the video device supports video capture
    if (ioctl(fd, VIDIOC_QUERYCAP, &caps) < 0)
    {
        perror("Failed to query video device capabilities");
        exit(EXIT_FAILURE);
    }
    if (!(caps.capabilities & V4L2_CAP_VIDEO_CAPTURE))
    {
        printf("Video device does not support video capture\n");
        exit(EXIT_FAILURE);
    }

    // Set the format of the video stream
    memset(&fmt, 0, sizeof(fmt));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = 640;
    fmt.fmt.pix.height = 480;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_BGR24;
    fmt.fmt.pix.field = V4L2_FIELD_ANY;
    if (ioctl(fd, VIDIOC_S_FMT, &fmt) < 0)
    {
        perror("Failed to set video format");
        exit(EXIT_FAILURE);
    }

    // Request video buffers from the device
    memset(&req, 0, sizeof(req));
    req.count = 1;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;
    if (ioctl(fd, VIDIOC_REQBUFS, &req) < 0)
    {
        perror("Failed to request video buffers");
        exit(EXIT_FAILURE);
    }

    // Map the buffer from the device to memory
    memset(&buf, 0, sizeof(buf));
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.index = 0;
    if (ioctl(fd, VIDIOC_QUERYBUF, &buf) < 0)
    {
        perror("Failed to query buffer from device");
        exit(EXIT_FAILURE);
    }
    buffer = mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buf.m.offset);
    if (buffer == MAP_FAILED)
    {
        perror("Failed to map buffer from device to memory");
        exit(EXIT_FAILURE);
    }

    // Connect to the server
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        perror("Failed to create socket");
        exit(EXIT_FAILURE);
    }
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    if (inet_pton(AF_INET, IP_ADDRESS, &server_addr.sin_addr) <= 0)
    {
        perror("Failed to convert IP address");
        exit(EXIT_FAILURE);
    }
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Failed to connect to server");
        exit(EXIT_FAILURE);
    }

    // Start capturing video frames
    for (;;)
    {
        FD_ZERO(&fds);
        FD_SET(fd, &fds);
        tv.tv_sec = 0;
        tv.tv_usec = 40000;
        ret = select(fd + 1, &fds, NULL, NULL, &tv);
        if (ret < 0)
        {
            perror("Failed to select video device");
            exit(EXIT_FAILURE);
        }
        if (ret == 0)
        {
            printf("Timeout waiting for video frame\n");
            continue;
        }

        // Capture a video frame
        memset(&buf, 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        if (ioctl(fd, VIDIOC_DQBUF, &buf) < 0) // 여기서 에러남
        {
            perror("Failed to dequeue buffer from device");
            exit(EXIT_FAILURE);
        }

        // Get the timestamp
        uint64_t timestamp = (uint64_t)time(NULL);

        // Copy the image data and timestamp into the packet
        memcpy(packet.data, buffer, BUFFER_SIZE);
        packet.timestamp = timestamp;

        // Send the packet over TCP/IP
        ret = send(sock, &packet, sizeof(packet), 0);
        if (ret < 0)
        {
            perror("Failed to send packet over network");
            exit(EXIT_FAILURE);
        }

        // Print the timestamp for debugging purposes
        printf("Sent frame %d with timestamp %lu\n", counter, timestamp);

        // Release the video buffer back to the device
        if (ioctl(fd, VIDIOC_QBUF, &buf) < 0)
        {
            perror("Failed to queue buffer back to device");
            exit(EXIT_FAILURE);
        }

        // Increment the frame counter
        counter++;
    }

    // Cleanup
    munmap(buffer, buf.length);
    close(sock);
    close(fd);

    return 0;
}
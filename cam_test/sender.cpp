#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <linux/videodev2.h>

#define WIDTH 640
#define HEIGHT 480
#define BUFFER_SIZE (WIDTH * HEIGHT * 3)

int main(int argc, char **argv)
{
    int fd;
    struct v4l2_capability cap;
    struct v4l2_format format;
    struct v4l2_requestbuffers req;
    struct v4l2_buffer buf;
    unsigned char *buffer;
    fd_set fds;
    struct timeval tv;
    int sock, ret;
    struct sockaddr_in servaddr;
    int counter = 0;

    // Open the video device
    fd = open("/dev/video0", O_RDWR);
    if (fd < 0)
    {
        perror("Failed to open device");
        exit(EXIT_FAILURE);
    }

    // Get the device capabilities
    if (ioctl(fd, VIDIOC_QUERYCAP, &cap) < 0)
    {
        perror("Failed to get device capabilities");
        exit(EXIT_FAILURE);
    }

    // Set the device format
    memset(&format, 0, sizeof(format));
    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    format.fmt.pix.width = WIDTH;
    format.fmt.pix.height = HEIGHT;
    format.fmt.pix.pixelformat = V4L2_PIX_FMT_BGR24;
    if (ioctl(fd, VIDIOC_S_FMT, &format) < 0)
    {
        perror("Failed to set device format");
        exit(EXIT_FAILURE);
    }

    // Request buffers from the device
    memset(&req, 0, sizeof(req));
    req.count = 1;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;
    if (ioctl(fd, VIDIOC_REQBUFS, &req) < 0)
    {
        perror("Failed to request buffers from device");
        exit(EXIT_FAILURE);
    }

    // Map the buffer from the device
    memset(&buf, 0, sizeof(buf));
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.index = 0;
    if (ioctl(fd, VIDIOC_QUERYBUF, &buf) < 0)
    {
        perror("Failed to query buffer from device");
        exit(EXIT_FAILURE);
    }
    buffer = (unsigned char *)mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buf.m.offset);
    if (buffer == MAP_FAILED)
    {
        perror("Failed to map buffer from device");
        exit(EXIT_FAILURE);
    }

    // Connect to the server
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        perror("Failed to create socket");
        exit(EXIT_FAILURE);
    }
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(8080);
    inet_pton(AF_INET, "127.0.0.1", &servaddr.sin_addr);
    if (connect(sock, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    {
        perror("Failed to connect to server");
        exit(EXIT_FAILURE);
    }

    // Main loop
    while (1)
    {
        // Select on the video device file descriptor
        FD_ZERO(&fds);
        FD_SET(fd, &fds);
        tv.tv_sec = 0;
        tv.tv_usec = 40000; // 25 fps
        ret = select(fd + 1, &fds, NULL, NULL, &tv);
        if (ret < 0)
        {
            perror("Failed to select on device file descriptor");
            exit(EXIT_FAILURE);
        }
        if (ret == 0)
        {
            // Timeout occurred, try again
            continue;
        }

        // Dequeue a buffer from the device
        memset(&buf, 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        if (ioctl(fd, VIDIOC_DQBUF, &buf) < 0)
        {
            perror("Failed to dequeue buffer from device");
            exit(EXIT_FAILURE);
        }

        // Send the buffer over TCP/IP
        ret = send(sock, buffer, BUFFER_SIZE, 0);
        if (ret < 0)
        {
            perror("Failed to send buffer over TCP/IP");
            exit(EXIT_FAILURE);
        }

        // Queue the buffer back to the device
        if (ioctl(fd, VIDIOC_QBUF, &buf) < 0)
        {
            perror("Failed to queue buffer back to device");
            exit(EXIT_FAILURE);
        }

        // Print a message every 100 frames
        counter++;
        if (counter % 100 == 0)
        {
            printf("Sent frame %d\n", counter);
        }
    }

    // Close the socket
    close(sock);

    // Unmap the buffer from the device
    munmap(buffer, buf.length);

    // Close the video device
    close(fd);

    return 0;
}
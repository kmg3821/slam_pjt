// compile option
// g++ -std=c++14 -O2 ./imu.cpp -o ./imu

#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <chrono>
#include <string.h>

#define ACCEL_XOUT_H 0x3B
#define PWR_MGMT 0x6B
#define CONFIG 0x1A
#define SMPRT_DIV 0x19
#define FIFO_EN 0x23
#define USER_CTRL 0x6A
#define FIFO_R_W 0x74

using namespace std;

int main()
{
  // Open the I2C bus.
  int file_i2c = open("/dev/i2c-1", O_RDWR);
  if (file_i2c < 0)
  {
    cerr << "Failed to open the i2c bus" << endl;
    return 1;
  }

  // Set the I2C address of the slave device.
  int addr = 0x68;
  if (ioctl(file_i2c, I2C_SLAVE, addr) < 0)
  {
    cerr << "Failed to acquire bus access and/or talk to slave." << endl;
    return 1;
  }

  // Set to use the internal oscillator
  {
    uint8_t buf[2] = {PWR_MGMT, 0x00}; // {address, value}
    if (write(file_i2c, buf, 2) != 2)
    {
      cerr << "Failed to write to the i2c bus." << endl;
      return 1;
    }
  }

  // Set to use the lowpass filter
  {
    uint8_t buf[2] = {CONFIG, 0x01}; // {address, value}
    if (write(file_i2c, buf, 2) != 2)
    {
      cerr << "Failed to write to the i2c bus." << endl;
      return 1;
    }
  }

  // Set sampling rate to 200Hz
  {
    uint8_t buf[2] = {SMPRT_DIV, 0x04}; // {address, value}
    if (write(file_i2c, buf, 2) != 2)
    {
      cerr << "Failed to write to the i2c bus." << endl;
      return 1;
    }
  }

  // Set to enable FIFO
  {
    uint8_t buf[2] = {USER_CTRL, 0x44}; // {address, value}
    if (write(file_i2c, buf, 2) != 2)
    {
      cerr << "Failed to write to the i2c bus." << endl;
      return 1;
    }
  }

  // Set to use FIFO buffer
  {
    uint8_t buf[2] = {FIFO_EN, 0x78}; // {address, value}
    if (write(file_i2c, buf, 2) != 2)
    {
      cerr << "Failed to write to the i2c bus." << endl;
      return 1;
    }
  }

  FILE *fs = fopen("imu_data.txt", "w");
  auto t0 = chrono::high_resolution_clock::now();
  for (int i = 0; i < 1000; ++i)
  {
    // Write a byte to the slave device.
    unsigned char data = ACCEL_XOUT_H;
    if (write(file_i2c, &data, 1) != 1)
    {
      cerr << "Failed to write to the i2c bus." << endl;
      return 1;
    }

    // Read a byte from the slave device.
    unsigned char read_data[16] = {0};
    if (read(file_i2c, read_data, 14) != 14)
    {
      cerr << "Failed to read from the i2c bus." << endl;
      return 1;
    }

    auto t = chrono::high_resolution_clock::now();
    int dt = chrono::duration_cast<chrono::milliseconds>(t - t0).count();

    short ax = (read_data[0] << 8) | read_data[1];
    short ay = (read_data[2] << 8) | read_data[3];
    short az = (read_data[4] << 8) | read_data[5];

    short wx = (read_data[8] << 8) | read_data[9];
    short wy = (read_data[10] << 8) | read_data[11];
    short wz = (read_data[12] << 8) | read_data[13];

    // char str[512] = {0};
    // sprintf(str, "%d,%d,%d,%d,%d,%d,%d\n", dt, ax, ay, az, wx, wy, wz);
    // cout << str << endl;
    //fprintf(fs, "%d,%d,%d,%d,%d,%d,%d\n", dt, ax, ay, az, wx, wy, wz);

    // Print the data that was read.
    cout << dt << '(' << ax << ',' << ay << ',' << az << ')'
        << '(' << wx << ',' << wy << ',' << wz << ")\n";

    usleep(10 * 1000);
  }

  // Close the I2C bus.
  close(file_i2c);
  fclose(fs);

  return 0;
}
// compile option
// g++ -std=c++14 -O2 ./imu.cpp -o ./imu

#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <chrono>

#define ACCEL_XOUT_H 0x3B
#define PWR_MGMT 0x6B

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
  uint8_t buf[2] = {PWR_MGMT, 0x00}; // {address, value}
  if (write(file_i2c, buf, 2) != 2)
  {
    cerr << "Failed to write to the i2c bus." << endl;
    return 1;
  }

  for (int i = 0; ; ++i)
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

    auto stamp = chrono::high_resolution_clock::now().time_since_epoch().count();

    volatile short ax = (read_data[0] << 8) | read_data[1];
    volatile short ay = (read_data[2] << 8) | read_data[3];
    volatile short az = (read_data[4] << 8) | read_data[5];

    volatile short wx = (read_data[8] << 8) | read_data[9];
    volatile short wy = (read_data[10] << 8) | read_data[11];
    volatile short wz = (read_data[12] << 8) | read_data[13];


    // Print the data that was read.
    cout << stamp << '(' << ax << ',' << ay << ',' << az << ')'
      << '(' << wx << ',' << wy << ',' << wz << ")\n";

    usleep(10 * 1000);
  }

  // Close the I2C bus.
  close(file_i2c);

  return 0;
}
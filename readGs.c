#include "hmc5883.h"

int main(int argc, char *argv[])
{
    int fd;
    fd = I2C_SLAVE;

    if (argc < 2)
    {
        printf("argument error\n");
        printf("Usage: [dev]\n");
        return -1;
    }

    fd = open(argv[1], O_RDWR);
    if (fd < 0)
    {
        perror("device not found\n");
        exit(1);
    }

    // 初始化HMC5883，参数在hmc5883.h里面设置
    HMC5883_init(fd, Address);

    int16_t raw_x, raw_y, raw_z; // 之前是int8_t，现已改正
    float mag_x, mag_y, mag_z;

    if (HMC5883_selftest(fd, Address) == -1)
    {
        printf("self test not passed\n");
        // return -1;
    }
    else
    {
        printf("self test:PASS\n");
    }

    printf("Mag is:\n");
    usleep(1000 * 500);
    while (1)
    {
        // 获取原始数据
        raw_x = GetMagData_Raw(fd, Address, HMC_XMSB_REG);
        usleep(1000 * 10);
        raw_y = GetMagData_Raw(fd, Address, HMC_YMSB_REG);
        usleep(1000 * 10);
        raw_z = GetMagData_Raw(fd, Address, HMC_ZMSB_REG);
        usleep(1000 * 10);

        // 当设置为0 0 1灵敏度时候取1090
        mag_x = (float)raw_x / 1090;
        mag_y = (float)raw_y / 1090;
        mag_z = (float)raw_z / 1090;

        printf("X-axis:%6fGs  Y-axis:%6fGs  Z-axis:%6fGs    \r", mag_x, mag_y, mag_z);
        // wait for 70'000 useconds
        usleep(1000 * 70);
    }
    return 0;
}
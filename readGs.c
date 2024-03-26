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

    // 初始化HMC5883
    HMC5883_init(fd, Address);

    int16_t raw_x, raw_y, raw_z; // 之前是int8_t，现已改正
    float mag_x, mag_y, mag_z;

    printf("Mag is:\n");
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
        // 等待77毫秒，别问我为什么，因为比66.6毫秒大就行
        usleep(1000 * 77);
    }
    return 0;
}
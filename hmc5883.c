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

    // initialize HMC5883
    HMC5883_init(fd, Address);
    while (1)
    {
        usleep(1000 * 10);
        printf("Mag_X:%6d\n ", GetMagData_Raw(fd, Address, HMC_XMSB_REG));
        usleep(1000 * 10);
        printf("Mag_Z:%6d\n ", GetMagData_Raw(fd, Address, HMC_ZMSB_REG));
        usleep(1000 * 10);
        printf("Mag_Y:%6d\n ", GetMagData_Raw(fd, Address, HMC_YMSB_REG));
        usleep(1000 * 77);
    }

    close(fd);

    return 0;
}
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h> //挺重要的东西，好用

// HMC5883寄存器定义
#define HMC_CONFIG_A_REG 0X00 // 配置寄存器A

#define HMC_CONFIG_B_REG 0X01 // 配置寄存器B

#define HMC_MODE_REG 0X02 // 模式设置寄存器
// bit0-bit1 模式设置 00为连续测量 01为单一测量

#define HMC_XMSB_REG 0X03 // X输出结果高位
#define HMC_XLSB_REG 0X04 // 低位

#define HMC_ZMSB_REG 0X05 // Z输出结果高位
#define HMC_ZLSB_REG 0X06 // 低位

#define HMC_YMSB_REG 0X07 // Y输出结果高位
#define HMC_YLSB_REG 0X08 // 低位

#define HMC_STATUS_REG 0X09 // 只读的状态
// bit1 数据更新时该位自动锁存,等待用户读取,读取到一半的时候防止数据改变
// bit0 数据已经准备好等待读取了,DRDY引脚也能用

#define HMC_CHEAK_A_REG 0X0A // 三个识别寄存器,用于检测芯片完整性
#define HMC_CHEAK_B_REG 0X0B
#define HMC_CHEAK_C_REG 0X0C

#define HMC_CHECKA_VALUE 0x48 // 三个识别寄存器的默认值
#define HMC_CHECKB_VALUE 0x34
#define HMC_CHECKC_VALUE 0x33

// 从机地址HMC5883L地址
#define Address 0x1E // 默认0x1e

// HMC5883相关函数
static int HMC5883_init(int fd, uint8_t addr); // 初始化，记得自己调
static int i2c_set(int fd, uint8_t addr, uint8_t reg, uint8_t val);
static int i2c_get(int fd, uint8_t addr, uint8_t reg, uint8_t *val);
static short GetMagData_Raw(int fd, uint8_t addr, unsigned char REG_Address);
static int HMC5883_selftest(int fd, uint8_t addr);

static int HMC5883_init(int fd, uint8_t addr)
{
    i2c_set(fd, addr, HMC_CONFIG_A_REG, 0x70);
    // 参见手册12页
    //  bit0-bit1 xyz是否使用偏压,默认为 0 0 对应正常配置
    //  bit2-bit4 数据输出速率, 1 1 0 为最大75HZ 1 0 0 为15HZ 最小 0 0 0 0.75HZ
    //  bit5-bit6每次采样平均数 1 1 为8次(默认) 0 0 为一次
    //  bit7 保留位，无意义，置零即可
    i2c_set(fd, addr, HMC_CONFIG_B_REG, 0x20);
    // bit0-bit4 要求清零
    // bit5-bit7 增益调节，数据越大，增益越小，此处为0 0 1，对应1090的增益
    i2c_set(fd, addr, HMC_MODE_REG, 0x00);
    // bit0-bit1 模式设置 0 0 为连续测量 0 1 为单次测量

    return 0;
}

static int HMC5883_selftest(int fd, uint8_t addr)
{
    uint8_t original_para[3];
    i2c_get(fd, addr, HMC_CONFIG_A_REG, original_para);
    i2c_get(fd, addr, HMC_CONFIG_B_REG, original_para + 1);
    i2c_get(fd, addr, HMC_MODE_REG, original_para + 2);

    int gainList[7] = {230, 330, 390, 440, 660, 820, 1090};

    short test_x, test_z, test_y;

    int reg_b_value = 0xE0;

    i2c_set(fd, addr, HMC_CONFIG_A_REG, 0x71); // self-test mode
    i2c_set(fd, addr, HMC_MODE_REG, 0x00);

    usleep(10 * 1000);

    for (int time = 0; time < 7; time++)
    {
        i2c_set(fd, addr, HMC_CONFIG_B_REG, reg_b_value); // gain = (230)
        usleep(500 * 1000);

        test_x = GetMagData_Raw(fd, Address, HMC_XMSB_REG);
        test_z = GetMagData_Raw(fd, Address, HMC_ZMSB_REG);
        test_y = GetMagData_Raw(fd, Address, HMC_YMSB_REG);

        printf("testing...  gain:%4d   pass!\n", gainList[time]);
        usleep(70 * 1000);
        double lower_limit = 243 * gainList[time] / 390, upper_limit = 575 * gainList[time] / 390;
        if (((test_x < lower_limit) || (test_x > upper_limit)) ||
            ((test_z < lower_limit) || (test_z > upper_limit)) ||
            ((test_y < lower_limit) || (test_y > upper_limit)))
        {
            return -1;
        }
        reg_b_value -= 32; // 0b0010000
    }
    i2c_set(fd, addr, HMC_CONFIG_A_REG, original_para[0]);
    i2c_set(fd, addr, HMC_CONFIG_B_REG, original_para[1]);
    i2c_set(fd, addr, HMC_MODE_REG, original_para[2]);
    return 0;
}

static int i2c_set(int fd, uint8_t addr, uint8_t reg, uint8_t val)
{
    uint8_t tmp[2];

    tmp[0] = reg;
    tmp[1] = val;

    // 设置地址长度：0为7位地址，非0位10位
    ioctl(fd, I2C_TENBIT, 0);

    // 设置从机地址
    if (ioctl(fd, I2C_SLAVE, addr) < 0)
    {
        printf("unable to find iic slave\n");
        close(fd);
        return -1;
    }

    // 设置收不到ACK信号时的重试次数，默认为1，此处改为8
    ioctl(fd, I2C_RETRIES, 8);

    if (write(fd, tmp, 2) == 2)
    {
        return 0;
    }
    else
    {
        return -1;
    }
}

static int i2c_get(int fd, uint8_t addr, uint8_t reg, uint8_t *val)
{
    // 设置地址长度：0为7位地址
    ioctl(fd, I2C_TENBIT, 0);

    // 设置从机地址
    if (ioctl(fd, I2C_SLAVE, addr) < 0)
    {
        printf("unable to find iic slave\n");
        close(fd);
        return -1;
    }

    // 设置收不到ACK时的重试次数
    ioctl(fd, I2C_RETRIES, 8);

    if (write(fd, &reg, 1) == 1)
    {
        if (read(fd, val, 1) == 1)
        {
            return 0;
        }
    }
    else
    {
        return -1;
    }
}

static short GetMagData_Raw(int fd, uint8_t addr, unsigned char REG_Address)
{
    uint8_t high_8 = 0, low_8 = 0;

    i2c_get(fd, addr, REG_Address, &high_8);
    usleep(1000); // 1000微秒
    i2c_get(fd, addr, REG_Address + 1, &low_8);
    return (((short)high_8) << 8) | (short)low_8; // 之前没有强制转换，也有问题，现已改正
}
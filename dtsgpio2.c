#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ide.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/gpio.h>
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <linux/unistd.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_gpio.h>
#include <linux/cdev.h>
#include <linux/device.h>

#define DTSLED_CNT 1
#define DTSLED_NAME "dtsgpio2"
#define LEDOFF 0
#define LEDON 1
#define PULES 2

struct dtsled_dev
{
    dev_t devid;
    struct cdev cdev;
    struct class *class;
    struct device *device;
    int major;
    int minor;
    struct device_node *nd;
    /* led所使用的GPIO编号 */
    int gpioNumber;
};

struct dtsled_dev dtsled;

static int led_open(struct inode *inode, struct file *filp)
{
    /* 设置私有数据 */
    filp->private_data = &dtsled;
    return 0;
}

static ssize_t led_read(struct file *filp, char __user *buf, size_t cnt, loff_t *offt)
{
    return 0;
}

static ssize_t led_write(struct file *filp, const char __user *buf,
                         size_t cnt, loff_t *offt)
{
    int retvalue;
    unsigned char databuf[1];
    unsigned char ledstat;
    uint32_t i;
    struct dtsled_dev *dev = filp->private_data;

    retvalue = copy_from_user(databuf, buf, cnt);
    if (retvalue < 0)
    {
        printk("kernel write failed!\r\n");
        return -EFAULT;
    }

    ledstat = databuf[0]; /* 获取状态值 */

    if (ledstat == LEDON)
    {
        gpio_set_value(dev->gpioNumber, 0);
        // led_switch(LEDON); /* 打开LED灯 */
        printk("ON");
    }
    else if (ledstat == LEDOFF)
    {
        gpio_set_value(dev->gpioNumber, 1);
        // led_switch(LEDOFF); /* 关闭LED灯 */
        printk("OFF");
    }
    else if (ledstat == PULES)
    {
        for (i = 0; i < 100; i++)
        {
            gpio_set_value(dev->gpioNumber, 0);
            ndelay(1);
            gpio_set_value(dev->gpioNumber, 1);
            ndelay(1);
            gpio_set_value(dev->gpioNumber, 0);
            ndelay(1);
        }
    }
    return 0;
}

static int led_release(struct inode *inode, struct file *filp)
{
    return 0;
}

/* 设备操作函数 */
static struct file_operations dtsled_fops = {
    .owner = THIS_MODULE,
    .open = led_open,
    .read = led_read,
    .write = led_write,
    .release = led_release,
};

static int __init led_init(void)
{
    // u32 val = 0;
    int ret = 0;
    // u32 regdata[14];
    // const char *str;
    // struct property *proper;

    /* 1、获取设备节点：gpioled */
    dtsled.nd = of_find_node_by_path("/test");
    if (dtsled.nd == NULL)
    {
        printk("gpio2 node can not found!\r\n");
        return -EINVAL;
    }
    else
    {
        printk("gpio2 node found!\r\n");
    }

     /* 2、 获取设备树中的gpio属性，得到LED所使用的LED编号 */
    //marc-gpio2
    dtsled.gpioNumber = of_get_named_gpio(dtsled.nd, "gpio2", 0);
    if (dtsled.gpioNumber < 0)
    {
        printk("cant get gpio2\r\n");
        return -EINVAL;
    }
    printk("gpio num = %d\r\n", dtsled.gpioNumber);

    /* 3、设置GPIO1_IO03为输出，并且输出高电平，默认关闭LED灯 */
    ret = gpio_direction_output(dtsled.gpioNumber, 1);
    
    if (ret < 0)
    {
        printk("cant set gpio2 to output\r\n");
    }

#if 0
    // get device node
    dtsled.nd = of_find_node_by_path("/mygpio2");
    if (dtsled.nd == NULL)
    {
        printk("alphaled node can not found!\r\n");
        return -EINVAL;
    }
    else
    {
        printk("alphaled node has been found!\r\n");
    }

    // get compatible property
    proper = of_find_property(dtsled.nd, "compatible", NULL);

    if (proper == NULL)
    {
        printk("compatible property find failed!\r\n");
    }
    else
    {
        printk("compatible = %s\r\n", (char *)proper->value);
    }

    ret = of_property_read_string(dtsled.nd, "status", &str);
    if (ret < 0)
    {
        printk("status read failed!\r\n");
    }
    else
    {
        printk("status = %s\r\n", str);
    }

    ret = of_property_read_u32_array(dtsled.nd, "reg", regdata, 10);
    if (ret < 0)
    {
        printk("reg property read failed!\r\n");
    }
    else
    {
        u8 i = 0;
        printk("reg data:\r\n");
        for (i = 0; i < 10; i++)
        {
            printk("%#X ", regdata[i]);
        }
        printk("\r\n");
    };

    IMX6U_CCM_CCGR1 = of_iomap(dtsled.nd, 0);
    SW_MUX_GPIO1_IO02 = of_iomap(dtsled.nd, 1);
    SW_PAD_GPIO1_IO02 = of_iomap(dtsled.nd, 2);
    GPIO1_DR = of_iomap(dtsled.nd, 3);
    GPIO1_GDIR = of_iomap(dtsled.nd, 4);

    /* 2、使能GPIO1时钟 */
    val = readl(IMX6U_CCM_CCGR1);
    val &= ~(3 << 26); /* 清以前的设置 */
    val |= (3 << 26);  /* 设置新值 */
    writel(val, IMX6U_CCM_CCGR1);

    // 3、设置GPIO1_IO03的复用功能，将其复用为GPIO
    // mux mode: 0101
    writel(5, SW_MUX_GPIO1_IO02);

    /*寄存器SW_PAD_GPIO1_IO03设置IO属性
	 *bit 16:0 HYS关闭
	 *bit [15:14]: 00 默认下拉
     *bit [13]: 0 kepper功能
     *bit [12]: 1 pull/keeper使能
     *bit [11]: 0 关闭开路输出
     *bit [7:6]: 10 速度100Mhz
     *bit [5:3]: 110 R0/6驱动能力
     *bit [0]: 0 低转换率
	 */
    writel(0x10B0, SW_PAD_GPIO1_IO02);

    /* 4、设置GPIO1_IO02为输出功能 */
    val = readl(GPIO1_GDIR);
    val &= ~(1 << 2); /* 清除以前的设置 */
    val |= (1 << 2);  /* 设置为输出 */
    writel(val, GPIO1_GDIR);

    /* 5、默认关闭LED */
    val = readl(GPIO1_DR);
    val |= (1 << 2);
    writel(val, GPIO1_DR);

    /* 6、注册字符设备驱动 */
    // 1 create device number
#endif

    if (dtsled.major)
    {
        dtsled.devid = MKDEV(dtsled.major, 0);
        register_chrdev_region(dtsled.devid, DTSLED_CNT,
                               DTSLED_NAME);
    }
    else
    {
        alloc_chrdev_region(&dtsled.devid, 0, DTSLED_CNT,
                            DTSLED_NAME);
        dtsled.major = MAJOR(dtsled.devid);
        dtsled.minor = MINOR(dtsled.devid);
    }
    printk("dts led major =%d, minor=%d\r\n", dtsled.major, dtsled.minor);

    dtsled.cdev.owner = THIS_MODULE;
    cdev_init(&dtsled.cdev, &dtsled_fops);

    cdev_add(&dtsled.cdev, dtsled.devid, DTSLED_CNT);

    dtsled.class = class_create(THIS_MODULE, DTSLED_NAME);

    if (IS_ERR(dtsled.class))
    {
        return PTR_ERR(dtsled.class);
    }

    dtsled.device = device_create(dtsled.class, NULL,
                                  dtsled.devid, NULL,
                                  DTSLED_NAME);

    if (IS_ERR(dtsled.device))
    {
        return PTR_ERR(dtsled.device);
    }

    return 0;
}

static void __exit led_exit(void)
{
    cdev_del(&dtsled.cdev);
    unregister_chrdev_region(dtsled.devid, DTSLED_CNT);

    device_destroy(dtsled.class, dtsled.devid);
    class_destroy(dtsled.class);
}

module_init(led_init);
module_exit(led_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("marc");

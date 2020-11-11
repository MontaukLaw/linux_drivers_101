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

#define DTSBEEP_CNT 1
#define DTSBEEP_NAME "beep"
#define BEEP 0
#define SHUTUP 1

struct dtsbeep_dev
{
    dev_t devid;
    struct cdev cdev;
    struct class *class;
    struct device *device;
    int major;
    int minor;
    struct device_node *nd;
    /* led所使用的GPIO编号 */
    int gpio;
};

struct dtsbeep_dev dtsbeep;

static int beep_open(struct inode *inode, struct file *filp)
{
    /* 设置私有数据 */
    filp->private_data = &dtsbeep;
    return 0;
}

static ssize_t beep_read(struct file *filp, char __user *buf, size_t cnt, loff_t *offt)
{
    return 0;
}

static ssize_t beep_write(struct file *filp, const char __user *buf,
                          size_t cnt, loff_t *offt)
{
    int retvalue;
    unsigned char databuf[1];
    unsigned char beepstat;
    struct dtsbeep_dev *dev = filp->private_data;

    retvalue = copy_from_user(databuf, buf, cnt);
    if (retvalue < 0)
    {
        printk("kernel write failed!\r\n");
        return -EFAULT;
    }

    beepstat = databuf[0]; /* 获取状态值 */

    if (beepstat == BEEP)
    {
        gpio_set_value(dev->gpio, 0);
        printk("ON");
    }
    else if (beepstat == SHUTUP)
    {
        gpio_set_value(dev->gpio, 1);
        printk("OFF");
    }
    return 0;
}

static int beep_release(struct inode *inode, struct file *filp)
{
    return 0;
}

/* 设备操作函数 */
static struct file_operations dtsbeep_fops = {
    .owner = THIS_MODULE,
    .open = beep_open,
    .read = beep_read,
    .write = beep_write,
    .release = beep_release,
};

static int __init beep_init(void)
{
    // u32 val = 0;
    int ret = 0;
    // u32 regdata[14];
    // const char *str;
    // struct property *proper;

    /* 1、获取设备节点：beep */
    dtsbeep.nd = of_find_node_by_path("/beep");
    if (dtsbeep.nd == NULL)
    {
        printk("beep node can not found!\r\n");
        return -EINVAL;
    }
    else
    {
        printk("beep node found!\r\n");
    }

    /* 2、 获取设备树中的gpio属性，得到LED所使用的LED编号 */
    //marc-gpio2
    dtsbeep.gpio = of_get_named_gpio(dtsbeep.nd, "beep-gpio", 0);
    if (dtsbeep.gpio < 0)
    {
        printk("cant get beep\r\n");
        return -EINVAL;
    }
    printk("gpio num = %d\r\n", dtsbeep.gpio);

    /* 3、设置GPIO1_IO03为输出，并且输出高电平，默认关闭LED灯 */
    ret = gpio_direction_output(dtsbeep.gpio, 1);

    if (ret < 0)
    {
        printk("cant set beep to output\r\n");
    }

    if (dtsbeep.major)
    {
        dtsbeep.devid = MKDEV(dtsbeep.major, 0);
        register_chrdev_region(dtsbeep.devid, DTSBEEP_CNT,
                               DTSBEEP_NAME);
    }
    else
    {
        alloc_chrdev_region(&dtsbeep.devid, 0, DTSBEEP_CNT,
                            DTSBEEP_NAME);
        dtsbeep.major = MAJOR(dtsbeep.devid);
        dtsbeep.minor = MINOR(dtsbeep.devid);
    }
    printk("dts led major =%d, minor=%d\r\n", dtsbeep.major, dtsbeep.minor);

    dtsbeep.cdev.owner = THIS_MODULE;

    cdev_init(&dtsbeep.cdev, &dtsbeep_fops);

    cdev_add(&dtsbeep.cdev, dtsbeep.devid, DTSBEEP_CNT);

    dtsbeep.class = class_create(THIS_MODULE, DTSBEEP_NAME);

    if (IS_ERR(dtsbeep.class))
    {
        return PTR_ERR(dtsbeep.class);
    }

    dtsbeep.device = device_create(dtsbeep.class, NULL,
                                  dtsbeep.devid, NULL,
                                  DTSBEEP_NAME);

    if (IS_ERR(dtsbeep.device))
    {
        return PTR_ERR(dtsbeep.device);
    }

    return 0;
}

static void __exit beep_exit(void)
{
    cdev_del(&dtsbeep.cdev);
    unregister_chrdev_region(dtsbeep.devid, DTSBEEP_CNT);

    device_destroy(dtsbeep.class, dtsbeep.devid);
    class_destroy(dtsbeep.class);
}

module_init(beep_init);
module_exit(beep_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("marc");

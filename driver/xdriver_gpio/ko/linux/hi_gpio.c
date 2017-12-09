/**
 * @file ftgpio_fun.c
 *  This source file is gpio special function driver
 * Copyright (C) 2017 Anviz Global Inc.
 *
 * $Revision: 1.0
 * $Date: 2017/12/7
 */

#include <linux/version.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/device.h>
#include <linux/bitops.h>
#include <asm/io.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include <asm/gpio.h>
#include <linux/gpio.h>

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/timer.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/miscdevice.h>
#include <linux/moduleparam.h>

#include <linux/io.h>
#include <linux/irq.h>

#include "hi_gpio.h"

#define GPIO_USE_NAME "hi_gpio"

static dev_t dev_num;
static struct cdev gpio_cdev;
static struct class *gpio_class = NULL;
static u32 open_cnt = 0;
static DEFINE_SEMAPHORE(drv_sem);

//============================================
//      file operations
//============================================
static int gpio_open(struct inode *inode, struct file *filp)
{
    int ret = 0;

    down(&drv_sem);
    if (!open_cnt) {
        open_cnt++;
    } else {
        printk("request gpio functions fail\n");
        ret = -EINVAL;
    }
    up(&drv_sem);

    return ret;
}

static int gpio_release(struct inode *inode, struct file *filp)
{
    int ret = 0;

    down(&drv_sem);
    open_cnt--;
    up(&drv_sem);

    return ret;
}

static long gpio_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    int ret = 0;
	int bit = 0, val = 0;

    if (_IOC_TYPE(cmd) != GPIO_IOC_MAGIC)
        return -ENOTTY;
    if (_IOC_NR(cmd) > GPIO_IOC_MAXNR)
        return -ENOTTY;
    if (_IOC_DIR(cmd) & _IOC_READ)
        ret = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
    else if (_IOC_DIR(cmd) & _IOC_WRITE)
        ret = !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));
    if (ret)
        return -EFAULT;

    switch (cmd) {
    case GPIO_SET_MULTI_PINS_OUT:
        {
            if (copy_from_user(&bit, (void __user *)arg, sizeof(bit))) {
                ret = -EFAULT;
                break;
            }
			val = bit & 0xFF00;
			bit &= 0xFF;
            if ((ret = gpio_request(bit, GPIO_USE_NAME)) != 0) {
				printk(KERN_ERR "%s: gpio_request %u failed\n", __func__, bit);
                goto err;
			}
            if ((ret = gpio_direction_output(bit, val)) != 0) {
                printk(KERN_ERR "%s: gpio_direction_output %u failed\n", __func__, bit);
            }
            gpio_free(bit);
        }
        break;

    case GPIO_SET_MULTI_PINS_IN:
        {
            if (copy_from_user(&bit, (void __user *)arg, sizeof(bit))) {
                ret = -EFAULT;
                break;
            }
			bit &= 0xFF;
            if ((ret = gpio_request(bit, GPIO_USE_NAME)) != 0) {
                printk(KERN_ERR "%s: gpio_request %u failed\n", __func__, bit);
                goto err;
            }
            if ((ret = gpio_direction_input(bit)) != 0) {
                printk(KERN_ERR "%s: gpio_direction_input failed\n", __func__);
            }
            gpio_free(bit);
        }
        break;

    case GPIO_GET_MULTI_PINS_VALUE:
        {
            if (copy_from_user(&bit, (void __user *)arg, sizeof(bit))) {
                ret = -EFAULT;
                break;
            }
			bit &= 0xFF;
            if ((ret = gpio_request(bit, GPIO_USE_NAME)) != 0) {
                printk(KERN_ERR "%s: gpio_request %u failed\n", __func__, bit);
                goto err;
            }
            val = __gpio_get_value(bit);
            gpio_free(bit);
        
	        if (copy_to_user((void __user *)arg, &val, sizeof(val))) {
	            ret = -EFAULT;
	            break;
	        }
    	}
        break;
    default:
        ret = -ENOIOCTLCMD;
        printk(KERN_ERR "Does not support this command.(0x%x)", cmd);
        break;
    }
  err:
    return ret;
}

struct file_operations gpio_fops = {
    .owner = THIS_MODULE,
    .open = gpio_open,
    .release = gpio_release,
    .unlocked_ioctl = gpio_ioctl,
};

static int __init ftgpio_function_init(void)
{
    int ret = 0;
	printk("driver ftgpio compliation time %s %s \n", __DATE__, __TIME__);
    ret = alloc_chrdev_region(&dev_num, 0, 1, GPIO_USE_NAME);
    if (unlikely(ret < 0)) {
        printk(KERN_ERR "%s:alloc_chrdev_region failed\n", __func__);
        goto err3;
    }
    cdev_init(&gpio_cdev, &gpio_fops);
    gpio_cdev.owner = THIS_MODULE;
    gpio_cdev.ops = &gpio_fops;
    ret = cdev_add(&gpio_cdev, dev_num, 1);
    if (unlikely(ret < 0)) {
        printk(KERN_ERR "%s:cdev_add failed\n", __func__);
        goto err2;
    }
    gpio_class = class_create(THIS_MODULE, GPIO_USE_NAME);
    if (IS_ERR(gpio_class)) {
        printk(KERN_ERR "%s:class_create failed\n", __func__);
        goto err1;
    }
    device_create(gpio_class, NULL, gpio_cdev.dev, NULL, GPIO_USE_NAME);
	return ret;
	
err1:
    cdev_del(&gpio_cdev);
err2:
    unregister_chrdev_region(dev_num, 1);
err3:
    return ret;
}

static void __exit ftgpio_function_exit(void)
{
    device_destroy(gpio_class, dev_num);
    class_destroy(gpio_class);
	cdev_del(&gpio_cdev);
	unregister_chrdev_region(dev_num, 1);
}

module_init(ftgpio_function_init);
module_exit(ftgpio_function_exit);
MODULE_AUTHOR("Anviz Global Inc.");
MODULE_DESCRIPTION("GPIO Function Driver");
MODULE_LICENSE("GPL");

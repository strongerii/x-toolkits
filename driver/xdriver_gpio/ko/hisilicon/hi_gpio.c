#include <linux/module.h>
#include <linux/errno.h>
#include <linux/miscdevice.h>
#include <linux/fcntl.h>

#include <linux/init.h>
#include <linux/delay.h>
#include <linux/proc_fs.h>
#include <linux/workqueue.h>

#include <asm/uaccess.h>
#include <asm/system.h>
#include <asm/io.h>
#include <linux/time.h>

#include "hi_gpio.h"


#define GPIO00_BASE (0x20140000)
#define GPIO01_BASE (0x20150000)
#define GPIO02_BASE (0x20160000)
#define GPIO03_BASE (0x20170000)
#define GPIO04_BASE (0x20180000)
#define GPIO05_BASE (0x20190000)
#define GPIO06_BASE (0x201A0000)
#define GPIO07_BASE (0x201B0000)
#define GPIO08_BASE (0x201C0000)
#define GPIO09_BASE (0x201D0000)
#define GPIO10_BASE (0x201E0000)
#define GPIO11_BASE (0x201F0000)

unsigned int GPIO_REG[]=
{
    GPIO00_BASE,
    GPIO01_BASE,
    GPIO02_BASE,
    GPIO03_BASE,
    GPIO04_BASE,
    GPIO05_BASE,
    GPIO06_BASE,
    GPIO07_BASE,
    GPIO08_BASE,
    GPIO09_BASE,
    GPIO10_BASE,
    GPIO11_BASE,
};

unsigned int GPIO_DIR[]=
{
    IO_ADDRESS(GPIO00_BASE + 0x400),
    IO_ADDRESS(GPIO01_BASE + 0x400),
    IO_ADDRESS(GPIO02_BASE + 0x400),
    IO_ADDRESS(GPIO03_BASE + 0x400),
    IO_ADDRESS(GPIO04_BASE + 0x400),
    IO_ADDRESS(GPIO05_BASE + 0x400),
    IO_ADDRESS(GPIO06_BASE + 0x400),
    IO_ADDRESS(GPIO07_BASE + 0x400),
    IO_ADDRESS(GPIO08_BASE + 0x400),
    IO_ADDRESS(GPIO09_BASE + 0x400),
    IO_ADDRESS(GPIO10_BASE + 0x400),
    IO_ADDRESS(GPIO11_BASE + 0x400),
};

unsigned char GPIO_BIT[]=
{
    (1 << 0),    /* GPIO X_0 */
    (1 << 1),    /* GPIO X_1 */
    (1 << 2),    /* GPIO X_2 */
    (1 << 3),    /* GPIO X_3 */
    (1 << 4),    /* GPIO X_4 */
    (1 << 5),    /* GPIO X_5 */
    (1 << 6),    /* GPIO X_6 */
    (1 << 7),    /* GPIO X_7 */
};

#define HW_REG(reg)         *((volatile unsigned int *)(reg))
#define GPIO_BIT_REG(X, Y) (IO_ADDRESS(GPIO_REG[X] + ((GPIO_BIT[Y])<<2)))

static int gpioSetMode(unsigned long arg)
{
    unsigned int  memaddr;

    unsigned int regvalue;
    
    unsigned char gpioReg;
    unsigned char gpioBit;
    unsigned char gpioDir;
    unsigned char gpioValue;

    gpioReg = (arg&0xFF000000)>>24;
    gpioBit = (arg&0x00FF0000)>>16;
    gpioDir = (arg&0x0000FF00)>>8;
    gpioValue = (arg&0x000000FF);

    //printk("%s:gpioReg%02x, gpioBit=%02x, gpioDir=%02x, gpioValue=%02x\n", __FUNCTION__, gpioReg, gpioBit, gpioDir ,gpioValue);
    if((gpioReg > 11) || (gpioBit > 7))
        return -1;

    memaddr = GPIO_BIT_REG(gpioReg, gpioBit);

    //���÷���Ĵ���
    regvalue = HW_REG(GPIO_DIR[gpioReg]);
    if(gpioDir)
        regvalue |= GPIO_BIT[gpioBit];
    else
        regvalue &= ~GPIO_BIT[gpioBit];
    HW_REG(GPIO_DIR[gpioReg]) = regvalue;

    //����ֵ�Ĵ���
    if(gpioDir)
    {
        regvalue = HW_REG(memaddr);

        if(gpioValue)
            regvalue |= GPIO_BIT[gpioBit];
        else
            regvalue &= ~GPIO_BIT[gpioBit];
        
        HW_REG(memaddr) = regvalue;
    }

    return 0;
}

static int gpioSet(unsigned long arg)
{
    unsigned int  memaddr;

    unsigned int regvalue;
    
    unsigned char gpioReg;
    unsigned char gpioBit;

    gpioReg = (arg&0xFF000000)>>24;
    gpioBit = (arg&0x00FF0000)>>16;

    if((gpioReg > 11) || (gpioBit > 7))
        return -1;
    //printk("%s:gpioReg%02x, gpioBit=%02x\n", __FUNCTION__, gpioReg, gpioBit);

    memaddr = GPIO_BIT_REG(gpioReg, gpioBit);
    
    regvalue = HW_REG(memaddr);

    regvalue |= GPIO_BIT[gpioBit];
    
    HW_REG(memaddr) = regvalue;

    return 0;
}

static int gpioClr(unsigned long arg)
{
    unsigned int  memaddr;

    unsigned int regvalue;
    
    unsigned char gpioReg;
    unsigned char gpioBit;

    gpioReg = (arg&0xFF000000)>>24;
    gpioBit = (arg&0x00FF0000)>>16;

    if((gpioReg > 11) || (gpioBit > 7))
        return -1;
    //printk("%s:gpioReg%02x, gpioBit=%02x\n", __FUNCTION__, gpioReg, gpioBit);

    memaddr = GPIO_BIT_REG(gpioReg, gpioBit);
    
    regvalue = HW_REG(memaddr);

    regvalue &= ~GPIO_BIT[gpioBit];
    
    HW_REG(memaddr) = regvalue;

    return 0;
}

static int gpioGet(unsigned long arg)
{
    unsigned int  memaddr;

    unsigned int regvalue;
    
    unsigned char gpioReg;
    unsigned char gpioBit;

    gpioReg = (arg&0xFF000000)>>24;
    gpioBit = (arg&0x00FF0000)>>16;

    if((gpioReg > 11) || (gpioBit > 7))
        return -1;
    //printk("%s:gpioReg%02x, gpioBit=%02x\n", __FUNCTION__, gpioReg, gpioBit);

    memaddr = GPIO_BIT_REG(gpioReg, gpioBit);
    
    regvalue = HW_REG(memaddr);

    regvalue &= GPIO_BIT[gpioBit];
    
    if(regvalue)
        return 1;

    return 0;
}

static int reg_read(unsigned long arg, unsigned int *regvalue)
{
    *regvalue = HW_REG(IO_ADDRESS(arg));

    return 0;
}

static int reg_write(unsigned long arg, unsigned int *regvalue)
{
    HW_REG(IO_ADDRESS(arg)) = *regvalue;
    return 0;
}

//int gpioDriver_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
long gpioDriver_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    unsigned int regvalue;
    struct DRV_gpio_ioctl_data val;
    
	if(_IOC_TYPE(cmd) != DRV_gpio_MAGIC) 
        return - EINVAL;    
    if(_IOC_NR(cmd) > DRV_gpio_MAX_NR) 
        return - EINVAL;
    
    switch(cmd)
    {
        case DRV_gpioSetMode:
                return gpioSetMode(arg);
                break;

        case DRV_gpioSet:
                return gpioSet(arg);
                break;
                
        case DRV_gpioClr:
                return gpioClr(arg);
                break;

        case DRV_gpioGet:
                return gpioGet(arg);
                break;
                
        case DRV_reg_read:
                reg_read(arg, &regvalue);
                return regvalue;
                break;

        case DRV_reg_write:
			    if(copy_from_user(&val, (struct DRV_gpio_ioctl_data *)arg, sizeof(struct DRV_gpio_ioctl_data)))
                    return -EFAULT;

                //printk("regwrite:%X:%X\n", val.uRegAddr, val.uRegValue);
                return reg_write(val.uRegAddr, &val.uRegValue);
                break;

        default:
                return -1;
    }
    return 0;
}

int gpioDriver_open(struct inode * inode, struct file * file)
{
    return 0;
}

int gpioDriver_close(struct inode * inode, struct file * file)
{
    return 0;
}

static struct file_operations gpioDriver_fops = {
    .owner      = THIS_MODULE,
    .unlocked_ioctl = gpioDriver_ioctl,
    .open       = gpioDriver_open,
    .release    = gpioDriver_close
};


static struct miscdevice gpioDriver_dev = {
   .minor		= MISC_DYNAMIC_MINOR,
   .name		= "gpioDriver",
   .fops  = &gpioDriver_fops,
};

static int __init gpioDriver_init(void)
{
    int ret = 1;

    ret = misc_register(&gpioDriver_dev);
    if(0 != ret)
    	return -1;
        
    return 0;    
}

static void __exit gpioDriver_exit(void)
{
    misc_deregister(&gpioDriver_dev);
}


module_init(gpioDriver_init);
module_exit(gpioDriver_exit);

MODULE_LICENSE("GPL");


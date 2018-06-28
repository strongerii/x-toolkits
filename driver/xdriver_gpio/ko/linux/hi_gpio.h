#ifndef _FTGPIO_API_H_
#define _FTGPIO_API_H_

#include <linux/ioctl.h>

#define GPIO_NUM 3

//============================================================================
// I/O control ID
//============================================================================
/* Use 'g' as magic number */
#define GPIO_IOC_MAGIC		'g'
#define GPIO_SET_MULTI_PINS_OUT		_IOW(GPIO_IOC_MAGIC, 1, int)
#define GPIO_SET_MULTI_PINS_IN		_IOW(GPIO_IOC_MAGIC, 2, int)
#define GPIO_GET_MULTI_PINS_VALUE	_IOW(GPIO_IOC_MAGIC, 3, int)
#define GPIO_IOC_MAXNR 3

#endif /*_FTGPIO_API_H_*/

/*
servoPi_modul

Obsadi GPIO 2 a GPIO 3 na RPI P1 pro IRQ.

Moje zapojeni:
GPIO 2 - yellow wire
GPIO 3 - red wire

*/

#include <linux/init.h>
#include <linux/module.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>
#include <linux/device.h>

#define IRC1 2 /* GPIO 2 -> IRC1 */
#define IRC2 3 /* GPIO 3 -> IRC2 */

#define IRC1_name	"GPIO2_irc1"
#define IRC2_name	"GPIO3_irc2"

#define LEFT	-1
#define RIGHT	1

#define HIGH	1
#define LOW	0

#define DEVICE_NAME 	"irc"

typedef struct {
	volatile uint32_t ircA_old;
	volatile uint32_t ircB_old;
	volatile uint32_t act_pos;
	atomic_t used_count;
} irc_instance;

irc_instance irc0;

int dev_major=0;

int irc1_irq_num = 0;
int irc2_irq_num = 0;

static struct class *irc_class;

volatile int movement[16];

static void init_move(void){
	/* A_old B_old A_now B_now*/
	movement[0] = 0;
	movement[1] = RIGHT;
	movement[2] = LEFT;
	movement[3] = 0;
	movement[4] = LEFT;
	movement[5] = 0;
	movement[6] = 0;
	movement[7] = RIGHT;
	/**/
	movement[8] = RIGHT;
	movement[9] = 0;
	movement[10] = 0;
	movement[11] = LEFT;
	movement[12] = 0;
	movement[13] = LEFT;
	movement[14] = RIGHT;
	movement[15] = 0;
}

int getWay(int ircA_old, int ircB_old, int ircA_now, int ircB_now) {
	return movement[8*ircA_old+4*ircB_old+ircA_now*2 + ircB_now*1];
/*	if(ircA_old == HIGH){*/
/*		if(ircB_old == HIGH){*/
/*			if(ircA_now == HIGH){*/
/*				if(ircB_now == HIGH){*/
/*					return 0;*/
/*				}else if(ircB_now == LOW){*/
/*					return RIGHT;*/
/*				}*/
/*			}else if(ircA_now == LOW){*/
/*				if(ircB_now == HIGH){*/
/*					return LEFT;  */
/*				}else if(ircB_now == LOW){*/
/*					return 0;*/
/*				}*/
/*			}*/
/*		}else if(ircB_old == LOW){*/
/*			if(ircA_now == HIGH){*/
/*				if(ircB_now == HIGH){*/
/*					return LEFT;*/
/*				}else if(ircB_now == LOW){*/
/*					return 0;*/
/*				}*/
/*			}else if(ircA_now == LOW){*/
/*				if(ircB_now == HIGH){*/
/*					return 0;*/
/*				}else if(ircB_now == LOW){*/
/*					return RIGHT;*/
/*				}*/
/*			}*/
/*		}*/
/*	}else if(ircA_old == LOW){*/
/*		if(ircB_old == HIGH){*/
/*			if(ircA_now == HIGH){*/
/*				if(ircB_now == HIGH){*/
/*					return RIGHT;*/
/*				}else if(ircB_now == LOW){*/
/*					return 0;*/
/*				}*/
/*			}else if(ircA_now == LOW){*/
/*				if(ircB_now == HIGH){*/
/*					return 0;*/
/*				}else if(ircB_now == LOW){*/
/*					return LEFT;*/
/*				}*/
/*			}*/
/*		}else if(ircB_old == LOW){*/
/*			if(ircA_now == HIGH){*/
/*				if(ircB_now == HIGH){*/
/*					return 0;*/
/*				}else if(ircB_now == LOW){*/
/*					return LEFT;*/
/*				}*/
/*			}else if(ircA_now == LOW){*/
/*				if(ircB_now == HIGH){*/
/*					return RIGHT;*/
/*				}else if(ircB_now == LOW){*/
/*					return 0;*/
/*				}*/
/*			}*/
/*		}*/
/*	}*/
/*	*/
/*	return 0;*/
} /* getWay */

static irqreturn_t irc_irq_handler(int irq, void *dev){
	volatile int Anow, Bnow;
	irc_instance *irc = (irc_instance*)dev;
        volatile int pom;
        Anow = gpio_get_value(IRC1);
        Bnow = gpio_get_value(IRC2);
        pom = getWay(irc->ircA_old,irc->ircB_old, Anow, Bnow);
/*        if(pom == 0){*/
/*		printk(KERN_NOTICE "irc nestiha\n");*/
/*	}*/
	irc->act_pos += pom;
        irc->ircA_old = Anow;
	irc->ircB_old = Bnow;
/*	printk(KERN_NOTICE "%u\n", irc->act_pos);*/
        return IRQ_HANDLED;
} /* irc_irq_handler */

ssize_t irc_read(struct file *file, char *buffer, size_t length, loff_t *offset) {
	irc_instance *irc = (irc_instance*)(file->private_data);
/*	uint32_t pos;*/
	int bytes_to_copy;
	int ret;
	uint32_t pos;
	if(!irc){
		printk(KERN_ERR "irc_read: no instance\n");
		return -ENODEV;
	}

	if (length < sizeof(uint32_t)) {
		printk(KERN_DEBUG "Trying to read less bytes than a irc message, \n");
		printk(KERN_DEBUG "this will always return zero.\n");
		return 0;
	}
	
	pos = *(volatile uint32_t*)&irc->act_pos;
	
	ret = copy_to_user(buffer, &pos, sizeof(uint32_t));

	buffer += sizeof(uint32_t);
	
	bytes_to_copy = length-sizeof(uint32_t);
	if(ret)	
		return -EFAULT;

	return length-bytes_to_copy;
} /* irc_read */

int irc_open(struct inode *inode, struct file *file) {
	int dev_minor = MINOR(file->f_dentry->d_inode->i_rdev);
	irc_instance *irc;
	if(dev_minor > 0){
		printk(KERN_ERR "There is no hardware support for the device file with minor nr.: %d\n", dev_minor);
	}
	irc = &irc0;
	
	atomic_inc(&irc->used_count);

	file->private_data = irc;
	return 0;
} /* irc_open */

int irc_relase(struct inode *inode, struct file *file) {
	irc_instance *irc = (irc_instance*)(file->private_data);
	if(!irc){
		printk(KERN_ERR "irc_read: no instance\n");
		return -ENODEV;
	}
	if(atomic_dec_and_test(&irc->used_count)){
		printk(KERN_DEBUG "Last irc user finished\n");
	}
	
	return 0;
} /* irc_relase */

struct file_operations irc_fops={
	.owner=THIS_MODULE,
	.read=irc_read,
	.write=NULL,
/*	.poll=irc_poll,*/
	.open=irc_open,
	.release=irc_relase,
};

static int servoPi_init(void) {
	int res;
	int dev_minor = 0;
	
	struct device *this_dev;
	
	printk(KERN_NOTICE "servoPi init started\n");
	
	irc_class=class_create(THIS_MODULE, DEVICE_NAME);
	res=register_chrdev(dev_major,DEVICE_NAME, &irc_fops);
	if (res<0) {
		printk(KERN_ERR "Error registering driver.\n");
		goto register_error;
	}
	if(dev_major == 0){
		dev_major = res;
	}
	this_dev=device_create(irc_class, NULL, MKDEV(dev_major, dev_minor), &irc0,  "irc%d", dev_minor);
		      
	if(IS_ERR(this_dev)){
		printk(KERN_ERR "problem to create device \"irc%d\" in the class \"irc\"\n", dev_minor);
		return (-1);
	}
		      
		      
	if(gpio_request(IRC1, IRC1_name) != 0){
		printk(KERN_ERR "failed request GPIO 2\n");
		return (-1); 
	}
	    
	if(gpio_request(IRC2, IRC2_name) != 0){
		printk(KERN_ERR "failed request GPIO 3\n");
		gpio_free(IRC1);
		return (-1);
	}
	    
	if(gpio_direction_input(IRC1) != 0){
		printk(KERN_ERR "failed set direction input GPIO 2\n");
		gpio_free(IRC1);
		gpio_free(IRC2);
		return (-1);
	}
	    
	if(gpio_direction_input(IRC2) != 0){
		printk(KERN_ERR "failed set direction input GPIO 3\n");
		gpio_free(IRC1);
		gpio_free(IRC2);
		return (-1);
	}
	    
	irc1_irq_num = gpio_to_irq(IRC1);
	if(irc1_irq_num < 0){
		printk(KERN_ERR "failed get IRQ number GPIO 2\n");
		gpio_free(IRC1);
		gpio_free(IRC2);
		return (-1);
	}
	    
	irc2_irq_num = gpio_to_irq(IRC2);
	if(irc2_irq_num < 0){
		printk(KERN_ERR "failed get IRQ number GPIO 3\n");
		gpio_free(IRC1);
		gpio_free(IRC2);
		return (-1);
	}
	
	if(request_irq((unsigned int)irc1_irq_num, irc_irq_handler, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, "irc1_irq", &irc0) != 0){
		printk(KERN_ERR "failed request IRQ GPIO 2\n");
		gpio_free(IRC1);
		gpio_free(IRC2);
		return (-1);
	}
	if(request_irq((unsigned int)irc2_irq_num, irc_irq_handler, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, "irc2_irq", &irc0) != 0){
		printk(KERN_ERR "failed request IRQ GPIO 3\n");
		gpio_free(IRC1);
		gpio_free(IRC2);
		free_irq((unsigned int)irc1_irq_num, NULL);
		return (-1);
	}
	init_move();
	printk(KERN_NOTICE "servoPi init done\n");
	return 0;
register_error:
	class_destroy(irc_class);
	return -ENODEV;
} /* servoPi_init */

static void servoPi_exit(void) {
	int dev_minor = 0;
	free_irq((unsigned int)irc1_irq_num, &irc0);
	free_irq((unsigned int)irc2_irq_num, &irc0);
	gpio_free(IRC1);
	gpio_free(IRC2);
	device_destroy(irc_class, MKDEV(dev_major, dev_minor));
	class_destroy(irc_class);
	unregister_chrdev(dev_major,DEVICE_NAME);
	
	printk(KERN_NOTICE "servoPi modul closed\n");
} /* servoPi_exit */

module_init(servoPi_init);
module_exit(servoPi_exit);

MODULE_LICENSE("GPL");
MODULE_VERSION("1.0");
MODULE_DESCRIPTION("servoPi module for driving servo");
MODULE_AUTHOR("Radek");

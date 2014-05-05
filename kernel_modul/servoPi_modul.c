/*
servoPi_modul_4irq.c

Obsadi GPIO 2, 3, 4, 23, 24 na RPI P1 pro vstup.

Moje zapojeni:
připojit propojovací obvod

*/

#include <linux/init.h>
#include <linux/module.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>
#include <linux/device.h>

#define IRC1	2 /* GPIO 2 -> IRC1 */
#define IRC2	3 /* GPIO 3 -> IRC2 */
#define IRC3 	24
#define IRC4	4
#define IRQ	23

#define IRC1_name	"GPIO2_irc1"
#define IRC2_name	"GPIO3_irc2"
#define IRC3_name	"GPIO24_irc1"
#define IRC4_name	"GPI04_irc2"
#define IRQ_name	"GPIO23_irq"

#define LEFT	-1
#define RIGHT	1

#define HIGH	1
#define LOW	0

#define DEVICE_NAME 	"irc"


atomic_t used_count;
volatile uint32_t pozice = 0;

volatile char predesly = 0;
volatile char smer = 1;

int dev_major=0;

int irc1_irq_num = 0;
int irc2_irq_num = 0;
int irc3_irq_num = 0;
int irc4_irq_num = 0;

static struct class *irc_class;

/*
irc_irq_handlerAN:
	Kanál IRC 1 (= 3) při náběžné hraně určí směr podle kanálu 2 (= 4).
*/
static irqreturn_t irc_irq_handlerAN(int irq, void *dev){
	
	if(smer == RIGHT){
		if(predesly == 4){
			pozice++;
			predesly = 1;
			return IRQ_HANDLED;
		}
	}else{
		if(predesly == 3){
			pozice--;
			predesly = 1;
			return IRQ_HANDLED;
		}
	}
		
	if(gpio_get_value(IRC2) == HIGH){
		pozice++;
		smer = RIGHT;
	}else{
		pozice--;
		smer = LEFT;
	}
	predesly = 1;
        return IRQ_HANDLED;
} /* irc_irq_handlerAN */

/*
irc_irq_handlerAS:
	Kanál IRC 3 (= 1) při sestupné hraně určí směr podle kanálu 2 (= 4).
*/
static irqreturn_t irc_irq_handlerAS(int irq, void *dev){

	if(smer == RIGHT){
		if(predesly == 3){
			pozice++;
			predesly = 2;
			return IRQ_HANDLED;
		}
	}else{
		if(predesly == 4){
			pozice--;
			predesly = 2;
			return IRQ_HANDLED;
		}
	}
		
	if(gpio_get_value(IRC2) == LOW){
		pozice++;
		smer = RIGHT;
	}else{
		pozice--;
		smer = LEFT;
	}
	predesly = 2;
        return IRQ_HANDLED;
} /* irc_irq_handlerAS */

/*
irc_irq_handlerBS:
	Kanál IRC 2 (= 4) při sestupné hraně určí směr podle kanálu 1 (= 3).
*/
static irqreturn_t irc_irq_handlerBS(int irq, void *dev){
	if(smer == RIGHT){
		if(predesly == 1){
			pozice++;
			predesly = 3;
			return IRQ_HANDLED;
		}
	}else{
		if(predesly == 2){
			pozice--;
			predesly = 3;
			return IRQ_HANDLED;
		}
	}
	
	if(gpio_get_value(IRC1) == HIGH){
		pozice++;
		smer = RIGHT;
	}else{
		pozice--;
		smer = LEFT;
	}
	predesly = 3;
        return IRQ_HANDLED;
} /* irc_irq_handlerBS */

/*
irc_irq_handlerAN:
	Kanál IRC 4 (= 2) při náběžné hraně určí směr podle kanálu 1 (= 3).
*/
static irqreturn_t irc_irq_handlerBN(int irq, void *dev){
	if(smer == RIGHT){
		if(predesly == 2){
			pozice++;
			predesly = 4;
			return IRQ_HANDLED;
		}
	}else{
		if(predesly == 1){
			pozice--;
			predesly = 4;
			return IRQ_HANDLED;
		}
	}
		
	if(gpio_get_value(IRC1) == LOW){
		pozice++;
		smer = RIGHT;
	}else{
		pozice--;
		smer = LEFT;
	}
	predesly = 4;
        return IRQ_HANDLED;
} /* irc_irq_handler */

ssize_t irc_read(struct file *file, char *buffer, size_t length, loff_t *offset) {
	int bytes_to_copy;
	int ret;
	uint32_t pos;

	if (length < sizeof(uint32_t)) {
		printk(KERN_DEBUG "Trying to read less bytes than a irc message, \n");
		printk(KERN_DEBUG "this will always return zero.\n");
		return 0;
	}
	
	pos = pozice;
	
	ret = copy_to_user(buffer, &pos, sizeof(uint32_t));

	buffer += sizeof(uint32_t);
	
	bytes_to_copy = length-sizeof(uint32_t);
	if(ret)	
		return -EFAULT;

	return length-bytes_to_copy;
} /* irc_read */

int irc_open(struct inode *inode, struct file *file) {
	int dev_minor = MINOR(file->f_dentry->d_inode->i_rdev);
	if(dev_minor > 0){
		printk(KERN_ERR "There is no hardware support for the device file with minor nr.: %d\n", dev_minor);
	}
	
	atomic_inc(&used_count);

	file->private_data = NULL;
	return 0;
} /* irc_open */

int irc_relase(struct inode *inode, struct file *file) {

	if(atomic_dec_and_test(&used_count)){
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

void free_irq_fn(void){
		free_irq((unsigned int)irc1_irq_num, NULL);
		free_irq((unsigned int)irc3_irq_num, NULL);
		free_irq((unsigned int)irc2_irq_num, NULL);
		free_irq((unsigned int)irc4_irq_num, NULL);
}

void free_fn(void){
	gpio_free(IRC1);
	gpio_free(IRC2);
	gpio_free(IRC3);
	gpio_free(IRC4);
	gpio_free(IRQ);
}

/*
inicializacePropojeni:
	Zinicializuje GPIO 2, 3, 4, 23 a 24 a nastaví pro vstup.
*/

int inicializacePropojeni(void){
	if(gpio_request(IRC1, IRC1_name) != 0){
		printk(KERN_ERR "failed request GPIO 2\n");
		return (-1); 
	}
	
	if(gpio_request(IRC2, IRC2_name) != 0){
		printk(KERN_ERR "failed request GPIO 3\n");
		gpio_free(IRC1);
		return (-1);
	}
	
	if(gpio_request(IRC3, IRC3_name) != 0){
		printk(KERN_ERR "failed request GPIO 24\n");
		gpio_free(IRC1);
		gpio_free(IRC2);
		return (-1);
	}
	
	if(gpio_request(IRC4, IRC4_name) != 0){
		printk(KERN_ERR "failed request GPIO 4\n");
		gpio_free(IRC1);
		gpio_free(IRC2);
		gpio_free(IRC3);
		return (-1);
	}
	
	if(gpio_request(IRQ, IRQ_name) != 0){
		printk(KERN_ERR "failed request GPIO 23\n");
		gpio_free(IRC1);
		gpio_free(IRC2);
		gpio_free(IRC3);
		gpio_free(IRC4);
		return (-1);
	}
	
	if(gpio_direction_input(IRC1) != 0){
		printk(KERN_ERR "failed set direction input GPIO 2\n");
		free_fn();
		return (-1);
	}
	    
	if(gpio_direction_input(IRC2) != 0){
		printk(KERN_ERR "failed set direction input GPIO 3\n");
		free_fn();
		return (-1);
	}
	
	if(gpio_direction_input(IRC3) != 0){
		printk(KERN_ERR "failed set direction input GPIO 24\n");
		free_fn();
		return (-1);
	}
	if(gpio_direction_input(IRC4) != 0){
		printk(KERN_ERR "failed set direction input GPIO 4\n");
		free_fn();
		return (-1);
	}
	if(gpio_direction_input(IRQ) != 0){
		printk(KERN_ERR "failed set direction input GPIO 23\n");
		free_fn();
		return (-1);
	}
	return 0;
}

static int servoPi_init(void) {
	int res;
	int dev_minor = 0;
	int pom = 0;
	
	struct device *this_dev;
	
	printk(KERN_NOTICE "servoPi init started\n");
	printk(KERN_NOTICE "verze bez tabulky (4x funce na 4 GPIO) - FAST\n");
/*	printk(KERN_NOTICE "testovaci verze - pouze pricitani\n");*/
	irc_class=class_create(THIS_MODULE, DEVICE_NAME);
	res=register_chrdev(dev_major,DEVICE_NAME, &irc_fops);
	if (res<0) {
		printk(KERN_ERR "Error registering driver.\n");
		class_destroy(irc_class);
		return -ENODEV;
		/*goto register_error;*/
	}
	if(dev_major == 0){
		dev_major = res;
	}
	this_dev=device_create(irc_class, NULL, MKDEV(dev_major, dev_minor), NULL,  "irc%d", dev_minor);
		      
	if(IS_ERR(this_dev)){
		printk(KERN_ERR "problem to create device \"irc%d\" in the class \"irc\"\n", dev_minor);
		return (-1);
	}
	
	pom = inicializacePropojeni();	      
	if(pom == -1){
		printk(KERN_ERR "Inicializace GPIO se nezdarila");
		return (-1);
	}

	irc1_irq_num = gpio_to_irq(IRC1);
	if(irc1_irq_num < 0){
		printk(KERN_ERR "failed get IRQ number GPIO 2\n");
		free_fn();
		return (-1);
	}
	
	irc2_irq_num = gpio_to_irq(IRC2);
	if(irc2_irq_num < 0){
		printk(KERN_ERR "failed get IRQ number GPIO 3\n");
		free_fn();
		return (-1);
	}
	
	irc3_irq_num = gpio_to_irq(IRC3);
	if(irc3_irq_num < 0){
		printk(KERN_ERR "failed get IRQ number GPIO 24\n");
		free_fn();
		return (-1);
	}
	
	irc4_irq_num = gpio_to_irq(IRC4);
	if(irc4_irq_num < 0){
		printk(KERN_ERR "failed get IRQ number GPIO 4\n");
		free_fn();
		return (-1);
	}
	
	if(request_irq((unsigned int)irc1_irq_num, irc_irq_handlerAN, IRQF_TRIGGER_RISING, "irc1_irqAS", NULL) != 0){
		printk(KERN_ERR "failed request IRQ GPIO 2\n");
		free_fn();
		return (-1);
	}
	if(request_irq((unsigned int)irc3_irq_num, irc_irq_handlerAS, IRQF_TRIGGER_FALLING, "irc3_irqAN", NULL) != 0){
		printk(KERN_ERR "failed request IRQ GPIO 24\n");
		free_fn();
		free_irq((unsigned int)irc1_irq_num, NULL);
		return (-1);
	}
	if(request_irq((unsigned int)irc2_irq_num, irc_irq_handlerBS, IRQF_TRIGGER_FALLING, "irc2_irqBS", NULL) != 0){
		printk(KERN_ERR "failed request IRQ GPIO 3\n");
		free_fn();
		free_irq((unsigned int)irc1_irq_num, NULL);
		free_irq((unsigned int)irc3_irq_num, NULL);
		return (-1);
	}
	if(request_irq((unsigned int)irc4_irq_num, irc_irq_handlerBN, IRQF_TRIGGER_RISING, "irc4_irqBN", NULL) != 0){
		printk(KERN_ERR "failed request IRQ GPIO 4\n");
		free_fn();
		free_irq((unsigned int)irc1_irq_num, NULL);
		free_irq((unsigned int)irc3_irq_num, NULL);
		free_irq((unsigned int)irc2_irq_num, NULL);
		return (-1);
	}
	printk(KERN_NOTICE "servoPi init done\n");
	return 0;
	
} /* servoPi_init */

static void servoPi_exit(void) {
	int dev_minor = 0;
	free_irq_fn();
	free_fn();
	device_destroy(irc_class, MKDEV(dev_major, dev_minor));
	class_destroy(irc_class);
	unregister_chrdev(dev_major,DEVICE_NAME);
	
	printk(KERN_NOTICE "servoPi modul closed\n");
} /* servoPi_exit */

module_init(servoPi_init);
module_exit(servoPi_exit);

MODULE_LICENSE("GPL");
MODULE_VERSION("1.1");
MODULE_DESCRIPTION("servoPi module for driving servo");
MODULE_AUTHOR("Radek Mečiar");

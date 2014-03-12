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

#define IRC1 2 /* GPIO 2 -> IRC1 */
#define IRC2 3 /* GPIO 3 -> IRC2 */

#define IRC1_name	"GPIO2_irc1"
#define IRC2_name	"GPIO3_irc2"

#define LEFT	1
#define RIGHT	2

int irc1_irq_num = 0;
int irc2_irq_num = 0;

char smer = LEFT;



/*static void freeGPIO(){
	gpio_free(IRC1);
	gpio_free(IRC2);
}*/ /* freeGPIO */

/*static void freeIRQ(){
	free_irq((unsigned int)irc1_irq_num, NULL);
	free_irq((unsigned int)irc2_irq_num, NULL);
} */ /* freeIRQ */

static int irc_smer_read_proc(char *buffer, char **start, off_t offset, int size, int *eof, void *data)
{
	char *left = "left\n";
	char *right = "right\n";
	int len_r = 6;
	int len_l = 5;
        /*
         * We only support reading the whole string at once.
         */
	if(size < len_r){
		return (-1);
	}
        /*
         * If file position is non-zero, then assume the string has
         * been read and indicate there is no more data to be read.
         */
        if (offset != 0)
                return 0;
        /*
         * We know the buffer is big enough to hold the string.
         */
        if(smer == LEFT){
        	strcpy(buffer, left);
        	*eof = 1;
        	return len_l;
        }else if(smer == RIGHT){
        	strcpy(buffer, right);
        	*eof = 1;
        	return len_r;
        }
        /*
         * Signal EOF.
         */
        *eof = 1;

        return 1;

}
static irqreturn_t irc_irq_handler(int irq, void *dev)
{
        /*struct motorek *m = dev_id;
        int state_a, state_b;

        state_a = gpio_get_value(IRC1);
        state_b = gpio_get_value(IRC2);

        int irc_state = (state_a ? 1 : 0) | (state_b ? 2 : 0);
        atomic_add(table[TRANSITION(m->last_irc_state, irc_state)], &m->pos);
        m->last_irc_state = irc_state;*/
	printk(KERN_NOTICE "Interrupt funguje!\n");
        return IRQ_HANDLED;
} /* irc_irq_handler */

static int servoPi_init(void)
{

	printk(KERN_NOTICE "servoPi init started\n");
	if(gpio_request(IRC1, IRC1_name) != 0){
		printk(KERN_ERR "failed request GPIO 2\n");
		return (-1); 
	}
	    
	if(gpio_request(IRC2, IRC2_name) != 0){
		printk(KERN_ERR "failed request GPIO 3\n");
		gpio_free(IRC1); /* uvolnim predesle pozadovane GPIO */
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
	
	if(request_irq((unsigned int)irc1_irq_num, irc_irq_handler, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, "irc1_irq", NULL) != 0){
		printk(KERN_ERR "failed request IRQ GPIO 2\n");
		gpio_free(IRC1);
		gpio_free(IRC2);
		return (-1);
	}
	if(request_irq((unsigned int)irc2_irq_num, irc_irq_handler, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, "irc2_irq", NULL) != 0){
		printk(KERN_ERR "failed request IRQ GPIO 3\n");
		gpio_free(IRC1);
		gpio_free(IRC2);
		free_irq((unsigned int)irc1_irq_num, NULL);
		return (-1);
	}
	
	/*if (create_proc_read_entry("irc_smer", 0, NULL, irc_smer_read_proc, NULL) == 0) {
                printk(KERN_ERR "Unable to register \"Hello, world!\" proc file\n");
                return (-1);
        }*/
	
	printk(KERN_NOTICE "servoPi init done\n");
	return 0;
} /* servoPi_init */

static void servoPi_exit(void)
{
	gpio_free(IRC1);
	gpio_free(IRC2);
	free_irq((unsigned int)irc1_irq_num, NULL);
	free_irq((unsigned int)irc2_irq_num, NULL);
	printk(KERN_NOTICE "servoPi modul closed\n");
} /* servoPi_exit */

module_init(servoPi_init);
module_exit(servoPi_exit);

MODULE_LICENSE("GPL");
MODULE_VERSION("0.0.1");
MODULE_DESCRIPTION("servoPi module for driving servo");
MODULE_AUTHOR("Radek");

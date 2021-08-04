/***************************************************************************//**
*  \file       Bari_char_driver.c
*
*  \details    Simple driver
*
*  \author     Bari shamli
*
* *******************************************************************************/
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/slab.h>                 //kmalloc()
#include <linux/uaccess.h>              //copy_to/from_user()
#include <linux/ioctl.h> 
#include <linux/proc_fs.h>
#include <linux/kthread.h>
#include <linux/wait.h>                 // Required for the wait queues
#include <linux/sysfs.h> 
#include <linux/kobject.h> 



/* 
** I am using the 5.4.0-80-generic. So I have set this as 504.
** If you are using the kernel 3.10, then set this as 310,
** and for kernel 5.1, set this as 501. Because the API proc_create()
** changed in kernel above v5.5.
**
*/ 
#define LINUX_KERNEL_VERSION  504

#define mem_size        1024           //Memory Size

#define WR_VALUE _IOW('a','a',int32_t*)
#define RD_VALUE _IOR('a','b',int32_t*)

//creating the dev with our custom major and minor number
//dev_t dev = MKDEV(235, 0);

//Allocating dev dynamicly
dev_t dev = 0;
static struct class *dev_class;
static struct cdev bari_cdev;

 
int valueETX, arr_valueETX[4];
char *nameETX;
int cb_valueETX = 0;

uint8_t *kernel_buffer;

int32_t value = 0;


char bari_array[20]="try_proc_array\n";
static int len = 1;
static struct proc_dir_entry *parent;



uint32_t read_count = 0;
static struct task_struct *wait_thread; 
int wait_queue_flag = 0;

//DECLARE_WAIT_QUEUE_HEAD(wait_queue);
wait_queue_head_t wait_queue;


volatile int etx_value = 0;
struct kobject *kobj_ref;


/*
** Function Prototypes
*/
static int      __init bari_driver_init(void);
static void     __exit bari_driver_exit(void);
static int      bari_open(struct inode *inode, struct file *file);
static int      bari_release(struct inode *inode, struct file *file);
static ssize_t  bari_read(struct file *filp, char __user *buf, size_t len,loff_t * off);
static ssize_t  bari_write(struct file *filp, const char *buf, size_t len, loff_t * off);
static long     bari_ioctl(struct file *file, unsigned int cmd, unsigned long arg);

/***************** Procfs Functions *******************/
static int      open_proc(struct inode *inode, struct file *file);
static int      release_proc(struct inode *inode, struct file *file);
static ssize_t  read_proc(struct file *filp, char __user *buffer, size_t length,loff_t * offset);
static ssize_t  write_proc(struct file *filp, const char *buff, size_t len, loff_t * off);



/*************** Sysfs functions **********************/
static ssize_t  sysfs_show(struct kobject *kobj, 
                        struct kobj_attribute *attr, char *buf);
static ssize_t  sysfs_store(struct kobject *kobj, 
                        struct kobj_attribute *attr,const char *buf, size_t count);
struct kobj_attribute etx_attr = __ATTR(etx_value, 0660, sysfs_show, sysfs_store);




static struct 	file_operations fops =
{
    .owner      = THIS_MODULE,
    .read       = bari_read,
    .write      = bari_write,
    .open       = bari_open,
    .unlocked_ioctl = bari_ioctl,
    .release    = bari_release,
};


#if ( LINUX_KERNEL_VERSION > 505 )
/*
** procfs operation sturcture
*/
static struct proc_ops proc_fops = {
        .proc_open = open_proc,
        .proc_read = read_proc,
        .proc_write = write_proc,
        .proc_release = release_proc
};
#else //LINUX_KERNEL_VERSION < 505
/*
** procfs operation sturcture
*/
static struct file_operations proc_fops = {
        .open = open_proc,
        .read = read_proc,
        .write = write_proc,
        .release = release_proc
};
#endif //LINUX_KERNEL_VERSION > 505



 
module_param(valueETX, int, S_IRUSR|S_IWUSR);                      //integer value
module_param(nameETX, charp, S_IRUSR|S_IWUSR);                     //String
module_param_array(arr_valueETX, int, NULL, S_IRUSR|S_IWUSR);      //Array of integers






/*
** This function will be called when we read the sysfs file
*/
static ssize_t sysfs_show(struct kobject *kobj, 
                struct kobj_attribute *attr, char *buf)
{
        pr_info("Sysfs - Read!!!\n");
        return sprintf(buf, "%d", etx_value);
}
/*
** This function will be called when we write the sysfsfs file
*/
static ssize_t sysfs_store(struct kobject *kobj, 
                struct kobj_attribute *attr,const char *buf, size_t count)
{
        pr_info("Sysfs - Write!!!\n");
        sscanf(buf,"%d",&etx_value);
        return count;
}












/*
** Thread function
*/
static int wait_function(void *unused)
{
        
        while(1) {
                pr_info("Waiting For Event...\n");
                wait_event_interruptible(wait_queue, wait_queue_flag != 0 );
                if(wait_queue_flag == 2) {
                        pr_info("Event Came From Exit Function\n");
                        return 0;
                }
                pr_info("Event Came From Read Function - %d\n", ++read_count);
                wait_queue_flag = 0;
        }
        do_exit(0);
        return 0;
}


/*
** This function will be called when we open the procfs file
*/
static int open_proc(struct inode *inode, struct file *file)
{
    pr_info("proc file opend.....\t");
    return 0;
}
/*
** This function will be called when we close the procfs file
*/
static int release_proc(struct inode *inode, struct file *file)
{
    pr_info("proc file released.....\n");
    return 0;
}
/*
** This function will be called when we read the procfs file
*/
static ssize_t read_proc(struct file *filp, char __user *buffer, size_t length,loff_t * offset)
{
    pr_info("proc file read.....\n");
    if(len)
    {
        len=0;
    }
    else
    {
        len=1;
        return 0;
    }
    
    if( copy_to_user(buffer,bari_array,20) )
    {
        pr_err("Data Send : Err!\n");
    }

    return length;;
}
/*
** This function will be called when we write the procfs file
*/
static ssize_t write_proc(struct file *filp, const char *buff, size_t len, loff_t * off)
{
    pr_info("proc file wrote.....\n");
    
    if( copy_from_user(bari_array,buff,len) )
    {
        pr_err("Data Write : Err!\n");
    }
    
    return len;
}




/*
** This function will be called when we open the Device file
*/
static int bari_open(struct inode *inode, struct file *file)
{
        /*Creating Physical memory*/
        if((kernel_buffer = kmalloc(mem_size , GFP_KERNEL)) == 0){
            pr_info("Cannot allocate memory in kernel\n");
            return -1;
        }
        pr_info("Device File Opened...!!!\n");
        return 0;
}
/*
** This function will be called when we close the Device file
*/
static int bari_release(struct inode *inode, struct file *file)
{
        kfree(kernel_buffer);
        pr_info("Device File Closed...!!!\n");
        return 0;
}
/*
** This function will be called when we read the Device file
*/
static ssize_t bari_read(struct file *filp, char __user *buf, size_t len, loff_t *off)
{
	
	wait_queue_flag = 1;
    wake_up_interruptible(&wait_queue);
	
    //Copy the data from the kernel space to the user-space
    if( copy_to_user(buf, kernel_buffer, mem_size) )
    {
        pr_err("Data Read : Err!\n");
    }
    pr_info("Data Read : Done!\n");
    

    
    return mem_size;
}
/*
** This function will be called when we write the Device file
*/
static ssize_t bari_write(struct file *filp, const char __user *buf, size_t len, loff_t *off)
{
   //Copy the data to kernel space from the user-space
   if( copy_from_user(kernel_buffer, buf, len) )
   {
       pr_err("Data Write : Err!\n");
   }
   pr_info("Data Write : Done!\n");
   return len;
}




/*
** This function will be called when we write IOCTL on the Device file
*/
static long bari_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
         switch(cmd) {
                case WR_VALUE:
                        if( copy_from_user(&value ,(int32_t*) arg, sizeof(value)) )
                        {
                                pr_err("Data Write : Err!\n");
                        }
                        pr_info("Value = %d\n", value);
                        break;
                case RD_VALUE:
                        if( copy_to_user((int32_t*) arg, &value, sizeof(value)) )
                        {
                                pr_err("Data Read : Err!\n");
                        }
                        break;
                default:
                        pr_info("Default\n");
                        break;
        }
        return 0;
}








 /*----------------------Module_param_cb()--------------------------------*/
int notify_param(const char *val, const struct kernel_param *kp)
{
        int res = param_set_int(val, kp); // Use helper for write variable
        if(res==0) {
                printk(KERN_INFO "Call back function called...\n");
                printk(KERN_INFO "New value of cb_valueETX = %d\n", cb_valueETX);
                return 0;
        }
        return -1;
}
 
 
const struct kernel_param_ops my_param_ops = 
{
        .set = &notify_param, // Use our setter ...
        .get = &param_get_int, // .. and standard getter
};
 
module_param_cb(cb_valueETX, &my_param_ops, &cb_valueETX, S_IRUGO|S_IWUSR );
/*-------------------------------------------------------------------------*/ 
 
 
 
/*
** Module Init function
*/
static int __init bari_driver_init(void)
{    
    int i;
    
    printk(KERN_INFO "Welcome to my module\n");
    

    printk(KERN_INFO "ValueETX = %d  \n", valueETX);
    printk(KERN_INFO "cb_valueETX = %d  \n", cb_valueETX);
    printk(KERN_INFO "NameETX = %s \n", nameETX);
    for (i = 0; i < (sizeof arr_valueETX / sizeof (int)); i++) {
           printk(KERN_INFO "Arr_value[%d] = %d\n", i, arr_valueETX[i]);
    }
    
   
    /*Allocating Major number dynamicaly */
    if((alloc_chrdev_region(&dev, 0, 1, "Bari_Dev")) <0){
          printk(KERN_INFO "Cannot allocate major number for device 1\n");
          return -1;
    }
    
    pr_info("Major = %d Minor = %d \n",MAJOR(dev), MINOR(dev));

    /*Allocating Major number statically */    
    //register_chrdev_region(dev, 1, "Bari_Device");
    
    
    /*Creating cdev structure*/
    cdev_init(&bari_cdev,&fops);
        
    /*Adding character device to the system*/
    if((cdev_add(&bari_cdev,dev,1)) < 0){
        pr_err("Cannot add the device to the system\n");
        goto r_class;
    }
    
 
    /*Creating struct class*/
   if((dev_class = class_create(THIS_MODULE,"Bari_class")) == NULL)
   {
        pr_err("Cannot create the struct class for device\n");
        goto r_class;
   }
 
   /*Creating device*/
   if((device_create(dev_class,NULL,dev,NULL,"Bari_Device")) == NULL){
       pr_err("Cannot create the Device\n");
       goto r_device;
   }
    
    
    
   /*Create proc directory. It will create a directory under "/proc" */
   parent = proc_mkdir("bari",NULL);
        
   if( parent == NULL )
   {
        pr_info("Error creating proc entry");
        goto r_device;
   }
        
   /*Creating Proc entry under "/proc/bari/" */
   proc_create("bari_proc", 0666, parent, &proc_fops);
   
   
   
   //Initialize wait queue
   init_waitqueue_head(&wait_queue);
   
   
   //Create the kernel thread with name 'mythread'
   wait_thread = kthread_create(wait_function, NULL, "WaitThread");
   if (wait_thread) {
          pr_info("Thread Created successfully\n");
          wake_up_process(wait_thread);
   } else
          pr_info("Thread creation failed\n");
   
 
   /*Creating a directory in /sys/kernel/ */
   kobj_ref = kobject_create_and_add("etx_sysfs",kernel_kobj);
 
   /*Creating sysfs file for etx_value*/
   if(sysfs_create_file(kobj_ref,&etx_attr.attr)){
         pr_err("Cannot create sysfs file......\n");
         goto r_sysfs;
    }
 
 
 
 
    printk(KERN_INFO "Kernel Module Inserted Successfully...\n");
      
    return 0;
    
r_sysfs:
       kobject_put(kobj_ref); 
       sysfs_remove_file(kernel_kobj, &etx_attr.attr);  
r_device:
        class_destroy(dev_class);
r_class:
        unregister_chrdev_region(dev,1);
		return -1;
    
}

/*
** Module Exit function
*/
static void __exit bari_driver_exit(void)
{
	
	wait_queue_flag = 2;
    wake_up_interruptible(&wait_queue);
    
    kobject_put(kobj_ref); 
    sysfs_remove_file(kernel_kobj, &etx_attr.attr);
	
	/* Removes single proc entry */
    //remove_proc_entry("etx/etx_proc", parent);
        
    /* remove complete /proc/etx */
    proc_remove(parent);
		
	device_destroy(dev_class,dev);
    class_destroy(dev_class);
    cdev_del(&bari_cdev);
	unregister_chrdev_region(dev, 1);
    printk(KERN_INFO "Kernel Module Removed Successfully...\n");
}
 
module_init(bari_driver_init);
module_exit(bari_driver_exit);
 
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Barishamli@gmail.com");
MODULE_DESCRIPTION("A simple hello world driver (Automatically Creating a Device file)");
MODULE_VERSION("1:1.0");

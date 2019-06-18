#include <linux/init.h>           // Macros used to mark up functions e.g. __init __exit
#include <linux/module.h>         // Core header for loading LKMs into the kernel
#include <linux/device.h>         // Header to support the kernel Driver Model
#include <linux/kernel.h>         // Contains types, macros, functions for the kernel
#include <linux/fs.h>             // Header for the Linux file system support
#include <linux/uaccess.h>          // Required for the copy to user function+
#include <linux/i2c.h>

#define  DEVICE_NAME "i2c_mse"    ///< The device will appear at /dev/ebbchar using this value
#define  CLASS_NAME  "i2c"        ///< The device class -- this is a character device driver

MODULE_LICENSE("GPL");            ///< The license type -- this affects available functionality
MODULE_AUTHOR("Gonzalo Lavigna");    ///< The author -- visible when you use modinfo
MODULE_DESCRIPTION("A simple Linux char driver for I2C and MPU9250");  ///< The description -- see modinfo
MODULE_VERSION("0.1");            ///< A version number to inform users

static int    majorNumber;                  ///< Stores the device number -- determined automatically
static char   message[256] = {0};           ///< Memory for the string that is passed from userspace
static short  size_of_message;              ///< Used to remember the size of the string stored
static int    numberOpens = 0;              ///< Counts the number of times the device is opened
static struct class*  ebbcharClass  = NULL; ///< The device-driver class struct pointer
static struct device* ebbcharDevice = NULL; ///< The device-driver device struct pointer
static struct i2c_client *modClient;


//Lugar donde esta la temperatura.
char ADDRESS[] = {0x41};

static const struct i2c_device_id mympu9250_i2c_id[] = {
{ "mympu9250", 0 },
{ }
};
MODULE_DEVICE_TABLE(i2c, mympu9250_i2c_id);
static const struct of_device_id mympu9250_of_match[] = {
    { .compatible = "mse,mympu9250" },
    { }
};
MODULE_DEVICE_TABLE(of, mympu9250_of_match);



// The prototype functions for the character driver -- must come before the struct definition
static int     dev_open(struct inode *, struct file *);
static int     dev_release(struct inode *, struct file *);
static ssize_t dev_read(struct file *, char *, size_t, loff_t *);
static ssize_t dev_write(struct file *, const char *, size_t, loff_t *);
static loff_t  device_lseek(struct file *file, loff_t offset, int orig);


static struct file_operations fops =
{
   .open = dev_open,
   .read = dev_read,
   .write = dev_write,
   .release = dev_release,
   .llseek = device_lseek,
};

static int __init ebbchar_init(void){
   printk(KERN_INFO "EBBChar: Initializing the EBBChar LKM\n");

   // Try to dynamically allocate a major number for the device -- more difficult but worth it
   majorNumber = register_chrdev(0, DEVICE_NAME, &fops);
   if (majorNumber<0){
      printk(KERN_ALERT "EBBChar failed to register a major number\n");
      return majorNumber;
   }
   printk(KERN_INFO "EBBChar: registered correctly with major number %d\n", majorNumber);

   // Register the device class
   ebbcharClass = class_create(THIS_MODULE, CLASS_NAME);
   if (IS_ERR(ebbcharClass)){                // Check for error and clean up if there is
      unregister_chrdev(majorNumber, DEVICE_NAME);
      printk(KERN_ALERT "Failed to register device class\n");
      return PTR_ERR(ebbcharClass);          // Correct way to return an error on a pointer
   }
   printk(KERN_INFO "EBBChar: device class registered correctly\n");

   // Register the device driver
   ebbcharDevice = device_create(ebbcharClass, NULL, MKDEV(majorNumber, 0), NULL, DEVICE_NAME);
   if (IS_ERR(ebbcharDevice)){               // Clean up if there is an error
      class_destroy(ebbcharClass);           // Repeated code but the alternative is goto statements
      unregister_chrdev(majorNumber, DEVICE_NAME);
      printk(KERN_ALERT "Failed to create the device\n");
      return PTR_ERR(ebbcharDevice);
   }
   printk(KERN_INFO "EBBChar: device class created correctly\n"); // Made it! device was initialized
   return 0;
}


static void __init ebbchar_exit(void){
   device_destroy(ebbcharClass, MKDEV(majorNumber, 0));     // remove the device
   class_unregister(ebbcharClass);                          // unregister the device class
   class_destroy(ebbcharClass);                             // remove the device class
   unregister_chrdev(majorNumber, DEVICE_NAME);             // unregister the major number
   printk(KERN_INFO "EBBChar: Goodbye from the LKM!\n");
}

static int dev_open(struct inode *inodep, struct file *filep){
   int i=0;
   int rv;
   char buf[1] = {0x6B};
   char mpu9250_output_buffer[21];
   numberOpens++;
   printk(KERN_INFO "EBBChar: Device has been opened %d time(s)\n", numberOpens);
   printk(KERN_INFO "UPDATE\n");
   //Lectura de 22 registros como en la SAPI lo hago para saber que el dispositivo sigue respondiendo
   ///TODO:Despues en aplicacion finales hay que sacarlo.
   buf[0]    = 0x3B;
   pr_info("READ 21 REGISTER FROM ADDRES 0x3B\n");
   rv = i2c_master_send(modClient,buf,1);
   rv = i2c_master_recv(modClient,mpu9250_output_buffer,21);
   pr_info("Datos Recibido: %0d\n",rv);
   for (i = 0; i < 21; i++)
   {
       pr_info("REGISTRO=0x%02X --> Valor: 0x%02X\n",buf[0]+i,mpu9250_output_buffer[i]);
   }
   return 0;
}

static ssize_t dev_read(struct file *filep, char *buffer, size_t len, loff_t *offset){
   int error_count = 0;
   int ret;
   //El offset esta configurado cuando se hace la funcion de lseek.
   pr_info("i2c: Start Reg Address MPU9250: 0x%02X \n",(char)(*offset));
   //Esto es para tenerlo sin el correspondiente casteo//pr_info("Offset: %lld \n",(*offset));
   //Con el read siempre me llevo el primer address-->Recordar que el address siempre es de un byte para este dispositivo.
   ADDRESS[0] = (char)(*offset);
   //Seteo el Address a partir del cual quiero escribir
   ret = i2c_master_send(modClient,ADDRESS,1);     
   if(ret < 0){
      printk(KERN_INFO "i2C: No se pudo configurar el registro para leer los datos desde el MPU9250\n");
   }

   pr_info("i2c: Cantidad de bytes configurados para leer = %d",len);
   ret = i2c_master_recv(modClient,message,len);     
   if(ret != len){
      printk(KERN_INFO "i2C: No se pudieron recibir %0d bytes desde el MPU9250\n", len);
   }
   //La idea es mandarle datos al usuario.
   error_count = copy_to_user(buffer,message,ret);
   
   if (error_count==0){            // if true then have success
      printk(KERN_INFO "EBBChar: Sent %d characters to the user\n", ret);
      return (ret=0);            // clear the position to the start and return 0
   }
   else {
      printk(KERN_INFO "EBBChar: Failed to send %d characters to the user\n", error_count);
      return -EFAULT;              // Failed -- return a bad address message (i.e. -14)
   }
}

static ssize_t dev_write(struct file *filep, const char *buffer, size_t len, loff_t *offset){
   int ret;
   char buf[256];
   pr_info("i2c: Start Reg Address MPU9250: 0x%02X \n",(char)(*offset));
   pr_info("i2c: Cantidad de bytes a escribir: %0d \n",len);
   //Cargo el address que viene en el offset
   buf[0] = (char)(*offset);
   //Agarro los datos provenientes del usuario
   ret = copy_from_user(message,buffer,len);
   size_of_message = strlen(message);   
   pr_info("i2c: Bytes recibidos desde el usuario:%d\n",size_of_message);
   //Copio el mensaje proveniente del usuario en el mensaje a enviar al usuario
   //Recordar que el address que hay que poner al principio hay que ponerlo y por eso va el +1.
   strcpy(buf+1,message);
   //Escribo por el I2C con el address al principio.
   ret = i2c_master_send(modClient,buf,size_of_message);
   pr_info("i2c: Se escribieron %d bytes a través del I2C\n",ret);

   
   //sprintf(message, "%s(%zu letters)", buffer, len);   // appending received string with its length
   //size_of_message = strlen(message);                 // store the length of the stored message
   //printk(KERN_INFO "EBBChar: Received %zu characters from the user\n", len);
   return len;
}

static int dev_release(struct inode *inodep, struct file *filep){
      int i=0;
   int rv;
   char buf[1] = {0x6B};
   char mpu9250_output_buffer[21];
      printk(KERN_INFO "EBBChar: Device successfully closed\n");
   //Lectura de 22 registros como en la SAPI lo hago para saber que el dispositivo sigue respondiendo
   ///TODO:Despues en aplicacion finales hay que sacarlo.
   buf[0]    = 0x3B;
   pr_info("READ 21 REGISTER FROM ADDRES 0x3B\n");
   rv = i2c_master_send(modClient,buf,1);
   rv = i2c_master_recv(modClient,mpu9250_output_buffer,21);
   pr_info("Datos Recibido: %0d\n",rv);
   for (i = 0; i < 21; i++)
   {
       pr_info("REGISTRO=0x%02X --> Valor: 0x%02X\n",buf[0]+i,mpu9250_output_buffer[i]);
   }
   return 0;
}


static loff_t device_lseek(struct file *file, loff_t offset, int orig) {
    loff_t new_pos = 0;
    printk(KERN_INFO "mympu9250 : lseek function in work\n");
   //No importa la configuracion del offset siempre se configura con el valor del parametro.
   //Esto es porque no tenemos ningun buffer y la idea es accederlo como un mapa de registros.
    switch(orig) {
        case 0 : /*seek set*/
            new_pos = offset;
            break;
        case 1 : /*seek cur*/
            new_pos = offset;
            break;
        case 2 : /*seek end*/
            new_pos = offset;
            break;
    }
    file->f_pos = new_pos;
    return new_pos;
}
// SPDX-License-Identifier: GPL-2.0
#include <linux/init.h>
#include <linux/module.h>
#include <linux/i2c.h>

//Funcion a a ejecutarse cuando se instale el kernel object
static int mympu9250_probe(struct i2c_client *client,const struct i2c_device_id *id){
    int i=0;
    int rv;
    char buf[1] = {0x6B};
    char wr_buf[2] = {0x6B,0x01};
    char rd_buf[2];
    char rv_data; 
    char mpu9250_output_buffer[21];
    
    ebbchar_init();
    modClient = client;
   //En la instanciacion de este driver se hacen algunas lecturas y escrituras spare para probar
   //la conexion.
   ///TODO:En el futuro hay que sacarlas.
    pr_info("PROBE:mympu9250\n");   
    pr_info("WRITE ADDRESS 0x6B con 0x1\n");
    rv = i2c_master_send(client,wr_buf,2);
    pr_info("i2c_master_send: 0x%02X\n",rv);
    pr_info("READ ADDRESS 0x6B\n");
    rv = i2c_master_send(client,buf,1);
    rv = i2c_master_recv(client,&rv_data,1);
    pr_info("i2c_master_receive: 0x%02X Numero de Datos: %0d\n",rv_data,rv);
    wr_buf[0] = 0x6A;
    wr_buf[1] = 0x20;
    buf[0]    = 0x6A;
    pr_info("WRITE ADDRESS 0x6A con 0x20\n");
    rv = i2c_master_send(client,wr_buf,2);
    pr_info("i2c_master_send: 0x%02X\n",rv);
    pr_info("READ ADDRESS 0x6A\n");
    rv = i2c_master_send(client,buf,1);
    rv = i2c_master_recv(client,rd_buf,2);
    pr_info("i2c_master_receive: 0x%02X 0x%02X Numero de Datos: %0d\n",rd_buf[0],rd_buf[1],rv);
    buf[0]    = 0x3B;
    pr_info("READ 21 REGISTER FROM ADDRES 0x3B\n");
    rv = i2c_master_send(client,buf,1);
    rv = i2c_master_recv(client,mpu9250_output_buffer,21);
    pr_info("Datos Recibido: %0d\n",rv);
    for (i = 0; i < 21; i++)
    {
        pr_info("REGISTRO=0x%02X --> Valor: 0x%02X\n",buf[0]+i,mpu9250_output_buffer[i]);
    }
    return 0;
}

static int mympu9250_remove(struct i2c_client *client)
{
    pr_info("REMOVE:mympu9250\n");
    ebbchar_exit();
    return 0;
}

static struct i2c_driver mympu9250_i2c_driver = {
    .driver = {
        .name = "mympu9250",
        .of_match_table = mympu9250_of_match,
    },
    .probe = mympu9250_probe,
    .remove = mympu9250_remove,
    .id_table = mympu9250_i2c_id,
};
module_i2c_driver(mympu9250_i2c_driver);

MODULE_LICENSE("GPL");


#include<stdio.h>
#include<stdlib.h>
#include<errno.h>
#include<fcntl.h>
#include<string.h>
#include<unistd.h>

#define TEMPSCALE       333.87f
#define TEMPOFFSET      21.0f
#define TEMPREG         0x41
#define PWRMGMNTREG_1   0x6B
#define USERCTRLREG     0x6A

static char   buffer[256] = {0}; 

int main (void){
int ret,fd;
int cant_lectura,cant_escritura;
int offset;
char wr_buff[256] = {0};
float temp;

fd = open("/dev/i2c_mse", O_RDWR);             // Open the device with read/write access
   if (fd < 0){
      perror("Failed to open the device...");
      return errno;
   }   

sleep(1);

//Lectura de la temperatura
lseek(fd,TEMPREG,SEEK_SET);
cant_lectura = 2;
printf("Se leeran %d bytes \n",cant_lectura);
printf("Leyendo desde el dispositivo \n");
ret = read(fd,buffer,cant_lectura);
if(ret < 0){
   printf("Failed to read the message from the device.");
   close(fd);
   return -1;
}
printf("REGISTRO=0x%02X --> Valor: 0x%02X\n",TEMPREG,buffer[0]);
printf("REGISTRO=0x%02X --> Valor: 0x%02X\n",TEMPREG+1,buffer[1]);

//Calculo de la temperatura
temp = (float)(buffer[0]<<8);
temp = temp + (float)buffer[1];
temp = ((((float)temp - TEMPOFFSET)/TEMPSCALE) + TEMPOFFSET);
printf("Temperatura:%2.2f\n",temp);
sleep(1);


//Offset para configurar la escritura de un registro
offset = PWRMGMNTREG_1;
lseek(fd,offset,SEEK_SET);
wr_buff[0] = 0x1;
cant_escritura = 1;
printf("Se escribiran %d bytes al registro: 0x%02X \n",cant_escritura,PWRMGMNTREG_1);
printf("Escribiendo desde el dispositivo \n");
ret = write(fd,wr_buff,cant_escritura);
if(ret < 0){
   printf("Falla al escribir mensaje al dispositivo\n");
   close(fd);
   return -1;
}
sleep(1);

//Offset para configurar la escritura de un registro
offset = USERCTRLREG;
lseek(fd,offset,SEEK_SET);
wr_buff[0] = 0x20;
cant_escritura = 1;
printf("Se escribiran %d bytes al registro: 0x%02X \n",cant_escritura,USERCTRLREG);
printf("Escribiendo desde el dispositivo \n");
ret = write(fd,wr_buff,cant_escritura);
if(ret < 0){
   printf("Falla al escribir mensaje al dispositivo\n");
   close(fd);
   return -1;
}
sleep(1);
printf("End of program\n");
close(fd);

return 0;
}
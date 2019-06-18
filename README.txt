Para compilar el codigo de usuario hacer que se llame test_write.c:
-->arm-linux-gnueabi-gcc test_write.c -o test_write

Para compilar el driver que se llama mympu9250.c:
-->export ARCH=arm
-->export CROSS_COMPILE=arm-linux-gnueabi-
-->make

Obviamente hay que seguir la estructura de carpetas donde esta el linux src.

Para utilizar el driver, hacer un lseek(ADDRESS), esta ADDRESS de 1 byte que es el registro que se quiere tocar.
Despues solo hay que hacer read o write con un buffer del tamaño dependiendo de que datos se quieren leer o se quieren escribir.


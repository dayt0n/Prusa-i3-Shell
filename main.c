// prusa - communicate with 3D printers over serial
//
// made by dayt0n
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include "serial_functions.h"

int main(int argc, char* argv[]) {
	int buf_max = 256;
	int fd = -1;
	int baudrate = 115200; //default baudrate
	int timeout = 5000;
	char buf[buf_max];
	char read[256];
	char* readstr;

	if(argc <= 1) {
		printf("usage: %s [/dev/cu.*]\n",argv[0]);
		return -1;
	}
	if (fd != -1) {
		serial_close(fd);
		printf("closed previous serial port\n");
	}
	fd = serial_init(argv[1],baudrate);
	if(fd == -1) {
		fprintf(stderr,"%s","failed to open port\n");
		return -1;
	}
	printf("opened serial port\n");
	serial_flush(fd);
	memset(buf,0,buf_max);
	while(1) {
		printf("Prusa> ");
		fgets(read,sizeof(read),stdin);
		readstr = read;
		if(strcmp("/exit\n",readstr) == 0) {
			break;
		}
		int n = serial_write(fd,readstr);
		if (n != 0) {
			printf("error writing\n");
		}
		serial_read_until(fd,buf,'\n',buf_max,timeout);
		printf("%s",buf);
	}
	serial_close(fd);
	printf("closed serial port\n");
	return 0;
}
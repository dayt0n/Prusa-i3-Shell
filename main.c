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
#include <ctype.h>
#include "serial_functions.h"

int buf_max = 256;
int timeout = 5000;

void usage(char* argv) {
	printf("usage: %s [OPTIONS] [/dev/cu.*]\n",argv);
	printf("Options:\n\n-a [X/Y/Z] [0-150]\tassign the coordinate of an axis\n");
	printf("-d\t\t\tset Z axis to home\n");
	printf("-l\t\t\tlist all items on SD Card (if present)\n");
	printf("-r\t\t\trestart printer\n");
	printf("-s\t\t\topen a serial shell\n");
}

// for creating commands
char* addVars(char *s1, char *s2)
{
    char *result = malloc(strlen(s1)+strlen(s2)+1);
    strcpy(result, s1);
    strcat(result, s2);
    return result;
}

int findTrigger(int fd, char* buf, char* axis) {
	int n = serial_write(fd,"M119\n");
	if(n != 0) {
		return -1;
	}
	char* max = addVars(axis,"_max: ");
	char* min = addVars(axis,"_min: ");
	char* maxOpen = addVars(max,"open");
	char* minOpen = addVars(min,"open");
	serial_read_until(fd,buf,'Q',buf_max,timeout);
	char* maxstr = strstr(buf,max);
	char* minstr = strstr(buf,min);
	minstr[strlen(minstr) - (strlen(minstr) - 11)] = 0;
	maxstr[strlen(maxstr) - (strlen(maxstr) - 11)] = 0;
	printf("MAX: %s\nMIN: %s\n",maxstr,minstr);
	if(strcmp(maxOpen,maxstr) != 0) {
		//printf("%s triggered on maximum\n",axis);
		return 0;
	}
	else if(strcmp(minOpen,minstr) != 0) {
		//printf("%s triggered on minimum\n",axis);
		return 0;
	}
	else {
		//printf("%s is not triggered at all\n",axis);
		return -1;
	}
	return -2;
}

/* this doesn't move the axis at all, it just tells the printer new coordinates 
 and you decide the stopping points */
int assignCoordinate(int fd, char* buf, char* plane, char* coordinate) {
	char* cmdDescript = "G92 ";
	char* cmdOne = addVars(cmdDescript,plane);
	char* cmdTwo = addVars(cmdOne," ");
	char* cmdThree = addVars(cmdTwo,coordinate);
	char* cmd = addVars(cmdThree,"\n");
	int n = serial_write(fd,cmd);
	if(n != 0)
		return -1;
	serial_read_until(fd,buf,'\n',buf_max,timeout);
	//printf("%s",buf);
	if(strcmp("ok\n",buf) == 0 || strcmp("echo:SD init fail\n",buf) == 0)
		printf("%s axis set to %s\n",plane,coordinate);
	else
		printf("failed to set axis\n");
	return 0;
}
// list all the printable items on the SD card inserted on the back of the LCD
int listSD(int fd, char* buf) {
	char* cmd = "M20\n";
	int n = serial_write(fd,cmd);
	if(n != 0)
		return -1;
	serial_read_until(fd,buf,'\n',buf_max,timeout);
	printf("%s",buf);
	return 0;
}
// if anything ever goes wrong, just restart it
int restartPrinter(int fd, char* buf) {
	char* cmd = "M999\n";
	int n = serial_write(fd,cmd);
	if(n != 0) 
		return -1;
	serial_read_until(fd,buf,'\n',buf_max,timeout);
	printf("%s",buf);
	return 0;
}

int goHome(int fd, char* buf) {
	int n;
	int f;
	printf("Setting speed...\n");
	char* speedcmd[] = { "M220 S400\n", "M220 S100\n" };
	n = serial_write(fd,speedcmd[0]);
	if(n != 0) {
		return -1;
	}
	printf("Sending X, Y, and Z home...\n");
	char* Ycmd[] = { "G92 Y 300\n", "G1 Y 2\n", "G92 Y 0\n" };
	for( int i = 0; i < 2; i++){
		n = serial_write(fd,Ycmd[i]);
		if(n != 0) {
			return -1;
		}
		//serial_read_until(fd,buf,'\n',buf_max,timeout);
		//printf("%s",buf);
	}
	f = findTrigger(fd,buf,"y");
	if(f != 0) {
		for( int i = 0; i < 3; i++){
			n = serial_write(fd,Ycmd[i]);
			if(n != 0) {
				return -1;
			}
		}
	}

	char* Xcmd[] = { "G92 X 0\n", "G1 X 300\n", "G1 X-210\n", "G92 X 0\n" };
	for( int i = 0; i < 2; i++) {
		n = serial_write(fd,Xcmd[i]);
		if(n != 0) {
			return -1;
		}
		//serial_read_until(fd,buf,'\n',buf_max,timeout);
		//printf("%s",buf);
	}
	f = findTrigger(fd,buf,"x");
	if(f != 0) {
		for( int i = 0; i < 4; i++){
			n = serial_write(fd,Xcmd[i]);
			if(n != 0) {
				return -1;
			}
		}
	}
	
	char* Zcmd[] = { "G92 Z 300\n", "G1 Z 2\n", "G92 Z 0\n" };
	for( int i = 0; i < 2; i++){
		n = serial_write(fd,Zcmd[i]);
		if(n != 0) {
			return -1;
		}
		//serial_read_until(fd,buf,'\n',buf_max,timeout);
		//printf("%s",buf);
	}
	f = findTrigger(fd,buf,"z");
	if(f != 0) {
		for( int i = 0; i < 3; i++){
			n = serial_write(fd,Zcmd[i]);
			if(n != 0) {
				return -1;
			}
		}
	}
	printf("Setting to default speed...\n");
	n = serial_write(fd,speedcmd[1]);
	if(n != 0) {
		return -1;
	}
	return 0;
}

void printOutput(int fd, char* buf) {
	int n;
	n = serial_read_until(fd,buf,'\n',buf_max,0);
	printf("%s",buf);
}

int beginShell(int fd, char* buf) {
	printOutput(fd,buf);
	printOutput(fd,buf);
	printOutput(fd,buf);
	printOutput(fd,buf);
	printOutput(fd,buf);
	printOutput(fd,buf);
	printOutput(fd,buf);
	return 0;
}

int main(int argc, char* argv[]) {
	int opt = 0;
	int fd = -1;
	int baudrate = 115200; //default baudrate
	char read[256];
	char* readstr;
	char buf[buf_max];

	if(argc <= 2) {
		usage(argv[0]);
		return -1;
	}
	if(strcmp(argv[1],"-h") == 0) {
		usage(argv[0]);
		return 0;
	}
	// initialization
	if (fd != -1) {
		serial_close(fd);
		printf("closed previous serial port\n");
	}
	fd = serial_init(argv[2],baudrate);
	if(fd == -1) {
		fprintf(stderr,"%s","failed to open port\n");
		return -1;
	}
	printf("opened serial port\n");
	serial_flush(fd);
	memset(buf,0,buf_max);
	// now we have a connection to the serial device (probably printer in this case)
	while ((opt = getopt(argc,argv,"adlprs:")) != -1) {
		switch(opt) {
			case 'a':
				if(argc <= 4) {
					printf("not enough arguments.\n");
					usage(argv[0]);
					break;
				}
				assignCoordinate(fd,buf,argv[3],argv[4]);
				break;
			case 'd':
				goHome(fd,buf);
				break;
			case 'l':
				listSD(fd,buf);
				break;
			case 'p':
				findTrigger(fd,buf,argv[3]);
				break;
			case 'r':
				restartPrinter(fd,buf);
				break;
			case 's':
				// looping shell
				beginShell(fd,buf);
				while(1) {
					beginloop:
					printf("Prusa> ");
					fgets(read,sizeof(read),stdin);
					readstr = read;
					if(strcmp("/exit\n",readstr) == 0) {
						break;
					}
					if(strcmp("/home\n",readstr) == 0) {
						goHome(fd,buf);
						goto beginloop;
					}
					int n = serial_write(fd,readstr);
					if (n != 0)
						printf("error writing\n");
					serial_read_until(fd,buf,'\n',buf_max,timeout);
					printf("%s",buf);
				}
				break;
			default:
				usage(argv[0]);
				break;
		}
	}
	serial_close(fd);
	printf("closed serial port\n");
	return 0;
}
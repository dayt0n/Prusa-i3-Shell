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
//#include <curses.h>
#include "serial_functions.h"

int buf_max = 256;
int timeoutz = 5000;

int check(int fd,int n) {
	if (n != 0) {
		serial_close(fd);
	}
	return -1;
}
// Graphics 
#ifdef _WIN32
#include <conio.h>
void clearScreen() {
	clrscr();
}
#else
void clearScreen() {
  const char* CLEAR_SCREE_ANSI = "\e[1;1H\e[2J";
  write(STDOUT_FILENO,CLEAR_SCREE_ANSI,12);
  printf("\r \b");
}
#endif

void emptyBed() {
	printf("|-----------------|\n");
	for(int i = 0; i < 10; i++) {
		printf("|                 |\n");
	}
	printf("|-----------------|\n");
}

void topLeftBed() {
	printf("|-----------------|\n");
	printf("| *               |\n");
	for(int i = 0; i < 9; i++) {
		printf("|                 |\n");
	}
	printf("|-----------------|\n");
}

void topRightBed() {
	printf("|-----------------|\n");
	printf("|               * |\n");
	for(int i = 0; i < 9; i++) {
		printf("|                 |\n");
	}
	printf("|-----------------|\n");
}

void middleBed() {
	printf("|-----------------|\n");
	for(int i = 0; i < 5; i++) {
		printf("|                 |\n");
	}
	printf("|        *        |\n");
	for(int i = 0; i < 4; i++) {
		printf("|                 |\n");
	}
	printf("|-----------------|\n");
}

void bottomLeftBed() {
	printf("|-----------------|\n");
	for(int i = 0; i < 9; i++) {
		printf("|                 |\n");
	}
	printf("| *               |\n");
	printf("|-----------------|\n");
}

void bottomRightBed() {
	printf("|-----------------|\n");
	for(int i = 0; i < 9; i++) {
		printf("|                 |\n");
	}
	printf("|               * |\n");
	printf("|-----------------|\n");
}

void allBeds() {
	printf("|-----------------|\n");
	printf("| *             * |\n");
	for(int i = 0; i < 4; i++) {
		printf("|                 |\n");
	}
	printf("|        *        |\n");
	for(int i = 0; i < 4; i++) {
		printf("|                 |\n");		
	}
	printf("| *             * |\n");
	printf("|-----------------|\n");
}
// end Graphics
void usage(char* argv) {
	printf("usage: %s [OPTIONS] [/dev/cu.*]\n",argv);
	printf("Options:\n\n-a [X/Y/Z] [0-150]\tassign the coordinate of an axis\n");
	printf("-b\t\t\tstart assisted leveling process\n");
	printf("-d\t\t\tset axis to home\n");
	printf("-f\t\t\tswitch filament\n");
	printf("-l\t\t\tlist all items on SD Card (if present)\n");
	printf("-p [X/Y/Z]\t\ttest to see which endstops are triggered\n");
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
	serial_read_until(fd,buf,'Q',buf_max,timeoutz);
	char* maxstr = strstr(buf,max);
	char* minstr = strstr(buf,min);
	minstr[strlen(minstr) - (strlen(minstr) - 11)] = 0;
	maxstr[strlen(maxstr) - (strlen(maxstr) - 11)] = 0;
	//printf("MAX: %s\nMIN: %s\n",maxstr,minstr);
	if(strcmp(maxOpen,maxstr) != 0 || strcmp(minOpen,minstr) != 0) {
		printf("%s endstop hit\n",axis);
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
	serial_read_until(fd,buf,'\n',buf_max,timeoutz);
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
	serial_read_until(fd,buf,'\n',buf_max,timeoutz);
	printf("%s",buf);
	return 0;
}
// if anything ever goes wrong, just restart it
int restartPrinter(int fd, char* buf) {
	char* cmd = "M999\n";
	int n = serial_write(fd,cmd);
	if(n != 0) 
		return -1;
	serial_read_until(fd,buf,'\n',buf_max,timeoutz);
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
		//serial_read_until(fd,buf,'\n',buf_max,timeoutz);
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

	char* Xcmd[] = { "G92 X 0\n", "G1 X 300\n", "G1 X 10\n", "G92 X 0\n" };
	for( int i = 0; i < 2; i++) {
		n = serial_write(fd,Xcmd[i]);
		if(n != 0) {
			return -1;
		}
		//serial_read_until(fd,buf,'\n',buf_max,timeoutz);
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
		//serial_read_until(fd,buf,'\n',buf_max,timeoutz);
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

int getExtruderTemp(int fd,char*buf) {
	int n = 0;
	int ret = 0;
	n = serial_write(fd,"M105 X0\n");
	check(fd,n);
	serial_read_until(fd,buf,'\n',buf_max,timeoutz);
	serial_read_until(fd,buf,'/',buf_max,timeoutz);
	char* newbuf = strstr(buf,":");
	newbuf++;
	char* newnewbuf = strchr(newbuf,'.');
	*newnewbuf = 0;
	ret = atoi(newbuf);
	return ret;
}

int cooldown(int fd) {
	int n = 0;
	n = serial_write(fd,"M104 S0\n");
	check(fd,n);
	n = serial_write(fd,"M140 S0\n");
	check(fd,n);
	return 0;
}

int switchFilament(int fd, char* buf) {
	int n = 0;
	float percent = 0.0;
	printf("Please begin by cutting off the filament at the very top of the extrusion motor, then press [Enter]\n");
	getchar();
	printf("Heating to 230 Celcius...\n");
	int temp = getExtruderTemp(fd,buf);
	serial_read_until(fd,buf,'\n',buf_max,timeoutz);
	n = serial_write(fd,"M104 S230\n");
	check(fd,n);
	while(getExtruderTemp(fd,buf) != 230) {
		percent = (getExtruderTemp(fd,buf) / 230) * 100;
		printf("\r%3g",percent);
		sleep(5);
	}
	printf("\n[done]\n");
	goHome(fd,buf);
	serial_write(fd,"G92 Z 0\n");
	serial_write(fd,"G1 Z 50\n");
	printf("Place the end of the new filament in the motor, then press [Enter]\n");
	getchar();
	printf("Keep pushing the filament in until the gear latches on\n");
	printf("Extruding...\n");
	char* halt = "M0\n";
	printf("Press enter once the filament is extruding in the new color\nPress [Enter] to stop extruding\n");
	char* extrude[] = {"G1 E 64\n", "G92 E 0\n"};
	for(int i = 0;i < 20;i++) {
		if(i % 2 == 0) {
			serial_write(fd,extrude[0]);
		} else {
			serial_write(fd,extrude[1]);
		}
	}
	printf("Switching to cooldown mode...");
	cooldown(fd);
	printf("[done]\n");
	return 0;
}

void printOutput(int fd, char* buf) {
	int n;
	n = serial_read_until(fd,buf,'Q',buf_max,0);
	printf("%s",buf);
}

int beginShell(int fd, char* buf) {
	printOutput(fd,buf);
	return 0;
}


int assistedLeveling(int fd, char* buf) {
	serial_read_until(fd,buf,'\n',buf_max,timeoutz); // to reduce wait time for command execution
	clearScreen();
	printf("Entering Assisted Leveling mode...\nUse a piece of paper as needed to test distance. Press [Enter] to start\n");
	emptyBed();
	serial_write(fd,"G92 Y 300\n");
	serial_write(fd,"G1 Y 0\n");
	// set y to be zero
	serial_write(fd,"G92 Y 0\n");
	getchar();
	// begin top left leveling
	clearScreen();
	printf("Assisted Leveling\n=================\n");
	topLeftBed();
	serial_write(fd,"G1 Y 250\n");
	serial_write(fd,"G1 X 300\n");
	serial_write(fd,"G1 X-10\n");
	serial_write(fd,"G92 Z 300\n");
	serial_write(fd,"G1 Z 0\n");
	serial_write(fd,"G92 Z 0\n");
	printf("Press [Enter] to continue to next location\n");
	getchar();
	// end top left leveling
	// begin top right leveling
	serial_write(fd,"G1 Z 10\n");
	clearScreen();
	printf("Assisted Leveling\n=================\n");
	topRightBed();
	serial_write(fd,"G1 X 10\n");
	serial_write(fd,"M121\n"); // disable endstop detection (for Z). this only works on Marlin, so we're lucky!
	serial_write(fd,"G1 Z 0\n");
	printf("Press [Enter] to continue to next location\n");
	getchar();
	// end top right leveling
	// begin bottom left leveling
	serial_write(fd,"G1 Z 10\n");
	clearScreen();
	printf("Assisted Leveling\n=================\n");
	bottomLeftBed();
	serial_write(fd,"M120\n"); // re-enable endstop detection (for X)
	serial_write(fd,"G1 X 300\n");
	serial_write(fd,"G1 X-10\n");
	serial_write(fd,"G92 Y 300\n");
	serial_write(fd,"G1 Y 0\n");
	serial_write(fd,"M121\n"); // disable endstop detection again (for Z)
	serial_write(fd,"G1 Z 0\n");
	printf("Press [Enter] to continue to next location\n");
	getchar();
	// end bottom left leveling
	// begin bottom right leveling
	serial_write(fd,"G1 Z 10\n");
	clearScreen();
	printf("Assisted Leveling\n=================\n");
	bottomRightBed();
	serial_write(fd,"G1 X 10\n");
	serial_write(fd,"G1 Z 0\n");
	printf("Press [Enter] to go to the next step\n");
	getchar();
	printf("Disabling stepper motors for free movement...\n");
	serial_write(fd,"M84\n"); // disables stepper motors
	clearScreen();
	printf("Assisted Leveling\n=================\n");
	allBeds();
	printf("Move the X and Y axis around to make sure everything is level, then press [Enter]\n");
	getchar();
	serial_write(fd,"M17\n"); // re-enable stepper motors
	printf("Homing...\n");
	goHome(fd,buf);
	printf("Assisted Leveling completed!\n");
	// end bottom right leveling
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
	while ((opt = getopt(argc,argv,"abdflprs:")) != -1) {
		switch(opt) {
			case 'a':
				if(argc <= 4) {
					printf("not enough arguments.\n");
					usage(argv[0]);
					break;
				}
				assignCoordinate(fd,buf,argv[3],argv[4]);
				break;
			case 'b':
				assistedLeveling(fd,buf);
				break;
			case 'd':
				goHome(fd,buf);
				break;
			case 'f': 
				switchFilament(fd,buf);
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
						serial_read_until(fd,buf,'q',buf_max,timeoutz);
						goto beginloop;
					}
					if(strcmp("/switch\n",readstr) == 0) {
						switchFilament(fd,buf);
						goto beginloop;
					}
					int n = serial_write(fd,readstr);
					if (n != 0)
						printf("error writing\n");
					serial_read_until(fd,buf,'\n',buf_max,timeoutz);
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
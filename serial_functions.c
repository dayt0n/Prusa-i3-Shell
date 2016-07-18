#include <stdint.h>
#include <string.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int serial_init(char* port, int baudrate) {
	struct termios toptions;
	int fd;
	fd = open(port, O_RDWR | O_NONBLOCK );
	if (fd == -1) {
		fprintf(stderr, "%s", "unable to open port\n");
		return -1;
	}
	if (tcgetattr(fd,&toptions) < 0) {
		fprintf(stderr,"%s","couldn't get term attributes\n");
		return -1;
	}
	speed_t rate = baudrate;
	switch(baudrate) {
		case 4800:   rate=B4800;   break;
		case 9600:   rate=B9600;   break;
		#ifdef B14400
		case 14400:  rate=B14400;  break;
		#endif
		case 19200:  rate=B19200;  break;
		#ifdef B28800
		case 28800:  rate=B28800;  break;
		#endif
		case 57600:  rate=B57600;  break;
		case 115200: rate=B115200; break;
	}
	cfsetispeed(&toptions,rate);
	cfsetospeed(&toptions,rate);
	toptions.c_cflag &= ~PARENB;
	toptions.c_cflag &= ~CSTOPB;
	toptions.c_cflag &= ~CSIZE;
	toptions.c_cflag |= CS8;
	toptions.c_cflag &= ~CRTSCTS;
	toptions.c_cflag |= CREAD | CLOCAL;
	toptions.c_iflag &= ~(IXON | IXOFF | IXANY);
	toptions.c_lflag &= ~(ICANON | ECHO | ECHOE |ISIG);
	toptions.c_oflag &= ~OPOST;
	toptions.c_cc[VMIN] = 0;
	toptions.c_cc[VTIME] = 0;
	tcsetattr(fd,TCSANOW,&toptions);
	if(tcsetattr(fd,TCSAFLUSH,&toptions) < 0) {
		fprintf(stderr,"%s","couldn't set term attributes\n");
		return -1;
	}
	return fd;
}

int serial_close(int fd) {
	return close(fd);
}

int serial_write(int fd, char* string) {
	int len = strlen(string);
	int n = write(fd,string,len);
	if(n != len) {
		fprintf(stderr,"%s","couldn't write entire string to serial\n");
		return -1;
	}
	return 0;
}

int serial_write_byte(int fd, uint8_t byte) {
	int n = write(fd,&byte,1);
	if( n != 1)
		return -1;
	return 0;
}

int serial_read_until(int fd,char* buffer,char until,int buf_max,int timeout) {
	char b[1];
	int i = 0;
	do {
		int n = read(fd,b,1);
		if(n == -1)
			return -1;
		if( n == 0) {
			usleep(1 * 1000);
			timeout--;
			if( timeout == 0)
				return -2;
			continue;
		}
		buffer[i] = b[0];
		i++;
	} while( b[0] != until && i < buf_max && timeout > 0);
	buffer[i] = 0;
	return 0;
}

int serial_flush(int fd) {
	sleep(2);
	return tcflush(fd,TCIOFLUSH);
}

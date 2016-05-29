#include <stdint.h>

int serial_init(char* port, int baudrate);
int serial_close(int fd);
int serial_write(int fd, char* string);
int serial_write_byte(int fd, uint8_t byte);
int serial_read_until(int fd, char* buffer, char until, int buf_max, int timeout);
int serial_flush(int fd);
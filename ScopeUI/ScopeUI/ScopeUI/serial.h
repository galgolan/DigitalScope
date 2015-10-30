#ifndef _SERIAL_H_
#define _SERIAL_H_

#include <stdbool.h>

typedef struct SerialPortSettings
{
	char* portName;
	int baudrate;
	int stopbits;
	int databits;
	int parity;	
} SerialPortSettings;

bool serial_open();

bool serial_write(const char * buffer, int size);

// reads data from the port into a pre-allocated buffer
// returns the number of bytes read
int serial_read(char* buffer, int size);

bool serial_close();

#endif

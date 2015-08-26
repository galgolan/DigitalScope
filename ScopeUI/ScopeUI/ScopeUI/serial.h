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

bool serial_open(SerialPortSettings settings);

bool serial_write(char* data, int bytes_to_send);

bool serial_read(char* buffer, int size);

bool serial_close();

#endif

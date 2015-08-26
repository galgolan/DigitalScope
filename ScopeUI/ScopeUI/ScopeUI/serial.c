#include <Windows.h>
#include <stdio.h>

#include "common.h"
#include "serial.h"

// Declare variables and structures
static HANDLE hSerial = INVALID_HANDLE_VALUE;
DCB dcbSerialParams = { 0 };
COMMTIMEOUTS timeouts = { 0 };

bool serial_open(SerialPortSettings settings)
{
	char portPath[16];

	// example
	settings.baudrate = CBR_115200;
	settings.stopbits = DATABITS_8;
	settings.parity = NOPARITY;
	settings.portName = "COM3";	
	settings.databits = 8;
	settings.stopbits = ONESTOPBIT;

	strcpy(portPath, "\\\\.\\");
	strcat(portPath, settings.portName);

	hSerial = CreateFile(portPath, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hSerial == INVALID_HANDLE_VALUE)
	{
		fprintf(stderr, "Error openning serial port");
		return FALSE;
	}

	dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
	if (GetCommState(hSerial, &dcbSerialParams) == 0)
	{
		fprintf(stderr, "Error getting device state\n");
		CloseHandle(hSerial);
		return FALSE;
	}

	dcbSerialParams.BaudRate = settings.baudrate;
	dcbSerialParams.ByteSize = settings.databits;
	dcbSerialParams.StopBits = settings.stopbits;
	dcbSerialParams.Parity = settings.parity;
	if (SetCommState(hSerial, &dcbSerialParams) == 0)
	{
		fprintf(stderr, "Error setting device parameters\n");
		CloseHandle(hSerial);
		return FALSE;
	}

	// Set COM port timeout settings
	timeouts.ReadIntervalTimeout = 50;
	timeouts.ReadTotalTimeoutConstant = 50;
	timeouts.ReadTotalTimeoutMultiplier = 10;
	timeouts.WriteTotalTimeoutConstant = 50;
	timeouts.WriteTotalTimeoutMultiplier = 10;
	if (SetCommTimeouts(hSerial, &timeouts) == 0)
	{
		fprintf(stderr, "Error setting timeouts\n");
		CloseHandle(hSerial);
		return FALSE;
	}

	return TRUE;
}

bool serial_write(char* data, int bytes_to_send)
{
	// Send specified text (remaining command line arguments)
	DWORD bytes_written, total_bytes_written = 0;
	fprintf(stderr, "Sending bytes...");
	if (!WriteFile(hSerial, bytes_to_send, 5, &bytes_written, NULL))
	{
		fprintf(stderr, "Error\n");
		CloseHandle(hSerial);
		return FALSE;
	}
	fprintf(stderr, "%d bytes written\n", bytes_written);

	return TRUE;
}

bool serial_read(char* buffer, int size)
{

}

bool serial_close()
{
	// Close serial port
	fprintf(stderr, "Closing serial port...");
	if (CloseHandle(hSerial) == 0)
	{
		fprintf(stderr, "Error\n");
		return FALSE;
	}
	fprintf(stderr, "OK\n");

	return TRUE;
}
#define _CRT_SECURE_NO_WARNINGS
#include <Windows.h>
#include <stdio.h>

#include "serial.h"
#include "config.h"

#define SERIAL_CONFIG_GROUP	"serial"
#define SERIAL_READ_TIMEOUT_MS	100

// Declare variables and structures
static HANDLE hSerial = INVALID_HANDLE_VALUE;
static DCB dcbSerialParams = { 0 };
static COMMTIMEOUTS timeouts = { 0 };

int serial_config_get_parity()
{
	char* paritySetting = config_get_string(SERIAL_CONFIG_GROUP, "parity");
	if (!strcmp(paritySetting, "none"))	return NOPARITY;
	else if (!strcmp(paritySetting, "odd"))	return ODDPARITY;
	else if (!strcmp(paritySetting, "even"))	return EVENPARITY;
	else if (!strcmp(paritySetting, "mark"))	return ODDPARITY;
	else if (!strcmp(paritySetting, "space"))	return SPACEPARITY;
	else return -1;
}

int serial_config_get_stop_bits()
{
	int stopBits = config_get_int(SERIAL_CONFIG_GROUP, "stopbits");
	switch (stopBits)
	{
	case 1:
		return ONESTOPBIT;
	case 15:
		return ONE5STOPBITS;
	case 2:
		return TWOSTOPBITS;
	default:
		return -1;
	}
}

SerialPortSettings serial_read_settings()
{
	SerialPortSettings settings;

	settings.baudrate = config_get_int(SERIAL_CONFIG_GROUP, "baudrate");	
	settings.parity = serial_config_get_parity();
	settings.portName = config_get_string(SERIAL_CONFIG_GROUP, "port");
	settings.databits = config_get_int(SERIAL_CONFIG_GROUP, "databits");
	settings.stopbits = serial_config_get_stop_bits();

	return settings;
}

bool serial_open()
{
	char portPath[24];
	SerialPortSettings settings = serial_read_settings();

	sprintf(portPath, "\\\\.\\%s", settings.portName);

	hSerial = CreateFile(settings.portName, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, 0);
	if (hSerial == INVALID_HANDLE_VALUE)
	{
		DWORD error = GetLastError();
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

bool serial_write(char * buffer, int size)
{
	OVERLAPPED osWrite = { 0 };
	DWORD dwWritten;
	DWORD dwRes;
	BOOL fRes;

	// Create this write operation's OVERLAPPED structure's hEvent.
	osWrite.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (osWrite.hEvent == NULL)
	{
		// error creating overlapped event handle
		return FALSE;
	}

	// Issue write.
	if (!WriteFile(hSerial, buffer, size, &dwWritten, &osWrite))
	{
		if (GetLastError() != ERROR_IO_PENDING)
		{
			// WriteFile failed, but isn't delayed. Report error and abort.
			fRes = FALSE;
		}
		else
		{
			// Write is pending.
			dwRes = WaitForSingleObject(osWrite.hEvent, INFINITE);
		}
		switch (dwRes)
		{
			// OVERLAPPED structure's event has been signaled. 
		case WAIT_OBJECT_0:
			if (!GetOverlappedResult(hSerial, &osWrite, &dwWritten, FALSE))
				fRes = FALSE;
			else
			{
				// Write operation completed successfully.
				fRes = TRUE;
			}
			break;

		default:
			// An error has occurred in WaitForSingleObject.
			// This usually indicates a problem with the
			// OVERLAPPED structure's event handle.
			fRes = FALSE;
			break;
		}
	}
	else
	{
		// WriteFile completed immediately.
		fRes = TRUE;
	}

	CloseHandle(osWrite.hEvent);
	return fRes;
}

int serial_read(char* buffer, int size)
{
	DWORD dwRead;
	OVERLAPPED osReader = { 0 };

	// Create the overlapped event. Must be closed before exiting
	// to avoid a handle leak.
	osReader.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	if (osReader.hEvent == NULL)
		return -1;	// error creating event

	// Issue read operation
	if (!ReadFile(hSerial, buffer, size, &dwRead, &osReader))
	{
		if (GetLastError() != ERROR_IO_PENDING)     // read not delayed?
		{
			// Error in communications; report it.
			CloseHandle(osReader.hEvent);
			return -1;
		}
		else
		{
			DWORD dwRes = WaitForSingleObject(osReader.hEvent, SERIAL_READ_TIMEOUT_MS);
			if (dwRes == WAIT_OBJECT_0)
			{
				// read completed successfully
				if (!GetOverlappedResult(hSerial, &osReader, &dwRead, FALSE))
				{
					CloseHandle(osReader.hEvent);
					return -1;	// error
				}
				else
				{
					CloseHandle(osReader.hEvent);
					return dwRead;	// success overlapped
				}
			}
			else if (dwRes == WAIT_TIMEOUT)
			{
				// buffer not yet ready
				return 0;
			}
			else
			{
				CloseHandle(osReader.hEvent);
				return -1;	//error in overlapped struct event handle
			}
		}
	}
	else
	{
		CloseHandle(osReader.hEvent);
		return dwRead;	// success immediate
	}
}

bool serial_close()
{
	if (hSerial == INVALID_HANDLE_VALUE)
		return TRUE;

	// Close serial port
	fprintf(stderr, "Closing serial port...");
	if (CloseHandle(hSerial) == 0)
	{
		hSerial = INVALID_HANDLE_VALUE;
		fprintf(stderr, "Error\n");
		return FALSE;
	}
	hSerial = INVALID_HANDLE_VALUE;
	fprintf(stderr, "OK\n");
	return TRUE;
}
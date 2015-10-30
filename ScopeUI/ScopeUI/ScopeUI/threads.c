#include <Windows.h>
#include <stdbool.h>
 
#include "threads.h"
 
bool WaitForSingleObjectSimple(HANDLE handle, unsigned long timeout)
{
    DWORD waitRes = WaitForSingleObject(handle, timeout);
    if(waitRes != WAIT_OBJECT_0)                // TODO: check if error code
        return FALSE;   
     
    return TRUE;
}
 
bool WaitForMutex(HANDLE mutex, unsigned long timeout)
{
    return WaitForSingleObjectSimple(mutex, timeout);
}

bool CloseHandleHelper(HANDLE* handle)
{
	if(*handle != INVALID_HANDLE_VALUE)
	{
		CloseHandle(*handle);
		*handle = INVALID_HANDLE_VALUE;
		return TRUE;
	}
	return FALSE;
}
 
bool WaitForSemaphore(HANDLE semaphore, long count, unsigned long timeout)
{
    int i;
 
    for(i=0; i<count; i++)
    {
        bool result = WaitForSingleObjectSimple(semaphore, timeout);
        if(result == FALSE)
        {
            // error aquiring lock, release locks aquired so far
            ReleaseSemaphoreSimple(semaphore, i);
            return FALSE;
        }
    }
    return TRUE;
}
 
bool ReleaseSemaphoreSimple(HANDLE semaphore, long count)
{
    return ReleaseSemaphore(semaphore, count, NULL);
}
 
HANDLE CreateThreadSimple(LPTHREAD_START_ROUTINE threadProc, LPVOID args)
{
    LPTHREAD_START_ROUTINE startFunction = ( LPTHREAD_START_ROUTINE ) threadProc;
    return CreateThread(
        NULL,
        0,
        startFunction,
        args,
        0,
        NULL);
}

HANDLE CreateInheritableSemaphore(long initialCount, long maxCount, LPCTSTR name)
{
	SECURITY_ATTRIBUTES attributes;
	attributes.nLength = sizeof(SECURITY_ATTRIBUTES);
	attributes.bInheritHandle = TRUE;
	attributes.lpSecurityDescriptor = NULL;
	return CreateSemaphore(&attributes, initialCount, maxCount, name);
}
 
HANDLE CreateSemaphoreSimple(long initialCount, long maxCount)
{
    return CreateSemaphore(NULL, initialCount, maxCount, NULL);
}
 
HANDLE CreateMutexSimple()
{
    return CreateMutex(NULL, FALSE, NULL);
}
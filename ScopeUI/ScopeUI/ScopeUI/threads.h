#ifndef _THREADS_H_
#define _THREADS_H_
 
#include <Windows.h>
#include <stdbool.h>

// macro for easy locking of mutexes
#define TRY_LOCK(ERROR_RESULT,MSG,MUTEX,TIMEOUT)	{if(!WaitForMutex(MUTEX, TIMEOUT)) \
													{ \
														printf(MSG); \
														return ERROR_RESULT; \
													}}

// defines pointer to a thread procedure. Used when creating new threads.
//typedef DWORD WINAPI (*ThreadFunc)(LPVOID);
 
/*
Waits for a mutex
Parameters: mutex - handle to a mutex, timeout - timeout to wait in ms
Returns: TRUE - success, FALSE - failure
*/
bool WaitForMutex(HANDLE mutex, unsigned long timeout);
 
/*
Waits for a semaphore (decrease)
Parameters: semaphore - handle to a semaphore, count - by how many to decrease the semaphore, timeout - timeout to wait in ms
Returns: TRUE - success, FALSE - failure
*/
bool WaitForSemaphore(HANDLE semaphore, long count, unsigned long timeout);

/*
Waits for a single object (thread) to finish.
Parameters:
	handle - the handle of the thread to wait on
	timeout - the max number of milliseconds to wait until the thread terminates.
Returns:
	TRUE - when the thread was terminated in the requested timeout
	FALSE - if there was an error waiting or the thread didnt finish in time
*/
bool WaitForSingleObjectSimple(HANDLE handle, unsigned long timeout);
 
/*
Releases (increases) a semaphore)
Parameters: semaphore - handle to a semaphore, count - by how many to increase the semaphore
Returns: TRUE - success, FALSE - failure
*/
bool ReleaseSemaphoreSimple(HANDLE semaphore, long count);
 
/*
Creates a thread
Parameters: threadFunc - pointer to the method to start, args - pointer to arguments type
Returns: handle of the thread or NULL if failed
*/
HANDLE CreateThreadSimple(LPTHREAD_START_ROUTINE threadProc, LPVOID args);
 
/*
Creates a semaphore
Parameters: initialCount - initial count of the semaphore, maxCount - maximum allowed count of the semaphore
Returns: handle to the semaphore or NULL of failed
*/
HANDLE CreateSemaphoreSimple(long initialCount, long maxCount);

/*
Creates an inheritable named semaphore
Parameters: initialCount - initial count of the semaphore, maxCount - maximum allowed count of the semaphore
name - the name of the semaphore
Returns: handle to the semaphore or NULL of failed
*/
HANDLE CreateInheritableSemaphore(long initialCount, long maxCount, LPCTSTR name);
 
/*
Creates a Mutex
Returns: handle to the mutex or null if failed
*/
HANDLE CreateMutexSimple();

// Closes a handle (if needed)
// This method identifies handles needed to be closed when they are not INVALID_HANDLE_VALUE
// Returns:
//	TRUE-if the handle was closed
//	FALSE-if it was already closed
bool CloseHandleHelper(HANDLE* handle);
 
#endif
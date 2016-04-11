#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include "resource.h"
#include "MainWindow.h"
#include "Utils.h"

#ifndef INITIAL_BUFFER_SIZE
#define INITIAL_BUFFER_SIZE 8192
#endif
#define BYTES_PER_READ      132


static int SpawnGui(HINSTANCE hInstance, int nCmdShow);
static int ReadAndCopyStandardInput(HINSTANCE hInstance, HANDLE hStdIn);
static BOOL CreateGuiProcess(PROCESS_INFORMATION *pi);


/** Callback function for EnumThreadWindows */
BOOL CALLBACK GetListWindow(HWND hwnd, LPARAM lParam)
{
	*((HWND *)lParam) = hwnd;
	/* The app only has one window so terminate the enumeration */
	return FALSE;
}


/** Main function */
int WINAPI
WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpszCmdLine, int nCmdShow)
{
	int                 returnValue          = 0;
	HANDLE              hStdIn;
	
	hInstance = hInst;
	
	GetAppTitle();
	
	/* If called from the command-line, we run a second copy of the executable.
	** This instance gathers the input from stdin while the second instances 
	** displays a window to the user.  This is so the list can be displayed 
	** without blocking the command prompt
	*/
	if (strstr(lpszCmdLine, "/g") != NULL || strstr(lpszCmdLine, "/G") != NULL)
	{
		returnValue = SpawnGui(hInstance, nCmdShow);
	}
	else if ((hStdIn = GetStdHandle(STD_INPUT_HANDLE)) != INVALID_HANDLE_VALUE)
	{
		returnValue = ReadAndCopyStandardInput(hInstance, hStdIn);
	}
	else
	{
		returnValue = 1;
	}
	return returnValue;
}


static int
SpawnGui(HINSTANCE hInstance, int nCmdShow)
{
	HWND    hMainWindow = NULL;
	HACCEL  acceleratorTable;
	MSG     msg;
	int     returnValue;
	
	if ((hMainWindow = CreateMainWindow(hInstance)) != NULL)
	{
			
			/* Load accelerator table */
			acceleratorTable = LoadAccelerators(hInstance, 
				MAKEINTRESOURCE(IDA_MAIN)
			);

			/* Show window and start message pump */
			ShowWindow(hMainWindow, nCmdShow);
			UpdateWindow(hMainWindow);
			while (GetMessage(&msg, (HWND)NULL, 0, 0) == TRUE){
				if (acceleratorTable != NULL && 
					!TranslateAccelerator(hMainWindow, acceleratorTable, &msg))
				{
					TranslateMessage(&msg);
					DispatchMessage(&msg);
				}
			}
			return msg.wParam;
	}
	returnValue = GetLastError();
	DisplayErrorMessage((HWND)NULL, IDS_ERROR_CREATING_WINDOW);
	return returnValue;
}


static int
ReadAndCopyStandardInput(HINSTANCE hInstance, HANDLE hStdIn)
{
	PROCESS_INFORMATION pi;
	HWND    hMainWindow = NULL;
	LPSTR               lpBuffer;
	COPYDATASTRUCT      *lpCopyStruct = NULL;
	DWORD               dwBufferSize;
	DWORD               dwBytesRead;
	DWORD               dwTotalBytesRead;
	LPSTR               lpToken = NULL;
	BOOL                bFileRead;
	int                 returnValue = 1;
	
	if (CreateGuiProcess(&pi))
	{
		/* Wait for process to stabilise */
		WaitForInputIdle(pi.hProcess, INFINITE);
		/* Find the process main window */
		EnumThreadWindows(pi.dwThreadId, GetListWindow, (LPARAM) &hMainWindow);
		if (hMainWindow != NULL)
		{
			/* Read standard input and send it over to the other window */
			lpCopyStruct = malloc(sizeof(COPYDATASTRUCT));
			
			dwTotalBytesRead = 0;
			dwBufferSize     = INITIAL_BUFFER_SIZE;
			
			lpBuffer = (LPSTR)malloc(dwBufferSize);
			lpBuffer[0] = '\0';
			for (;;)
			{
				bFileRead = ReadFile(
					hStdIn,
					(LPVOID) &lpBuffer[dwTotalBytesRead],
					BYTES_PER_READ,
					&dwBytesRead,
					NULL
				);
				if (bFileRead && dwBytesRead > 0)
				{
					dwTotalBytesRead += dwBytesRead;
					if (dwTotalBytesRead >= (dwBufferSize - BYTES_PER_READ))
					{
						dwBufferSize *= 2;
						lpBuffer = (LPSTR)realloc(lpBuffer, dwBufferSize);
					}
				}
				else
				{
					break;
				}
			}
			if (lstrlen(lpBuffer) > 0)
			{
				/* Break the buffer into lines and send each one to the window */
				lpToken = strtok(lpBuffer, "\r\n");
				while (lpToken !=NULL)
				{
					if (lstrlen(lpToken) > 0)
					{
						lpCopyStruct->cbData = lstrlen(lpToken) + 1;
						lpCopyStruct->lpData = (LPVOID)lpToken;
						SendMessage(hMainWindow, WM_COPYDATA, 
							(WPARAM)NULL, (LPARAM)lpCopyStruct);
					}
					lpToken = strtok((char *)NULL, "\r\n");
				}
			}
			free(lpBuffer);
			returnValue = 0;
		}
		else
		{
			returnValue = GetLastError();
			DisplayErrorMessage((HWND)NULL, IDS_ERROR_FINDING_WINDOW);
		}
	}
	else
	{
		returnValue = GetLastError();
		DisplayErrorMessage((HWND)NULL, IDS_ERROR_CREATING_PROCESS);
	}
	return returnValue;
}


static BOOL
CreateGuiProcess(LPPROCESS_INFORMATION pi)
{
	BOOL  returnValue = FALSE;
	DWORD dwCmdLineSize = MAX_PATH * 2;
	LPSTR lpCmdLine;
	STARTUPINFO si;
	
	lpCmdLine = (LPSTR)malloc(dwCmdLineSize);
	if (GetModuleFileName((HMODULE)NULL, lpCmdLine, MAX_PATH) != 0)
	{
		/* Start another instance of this app but with "/g" command-line switch */
		lstrcat(lpCmdLine, " /g");

		si.cb          =  sizeof(STARTUPINFO);
		si.lpReserved  =  NULL;
		si.lpDesktop   =  NULL;
		si.lpTitle     =  NULL;
		si.dwFlags     =  0L;
		si.cbReserved2 =  0;
		si.lpReserved2 =  NULL;
		
		returnValue = CreateProcess(
			(LPCTSTR)NULL,
			lpCmdLine,
			(LPSECURITY_ATTRIBUTES)NULL,
			(LPSECURITY_ATTRIBUTES)NULL,
			FALSE,
			NORMAL_PRIORITY_CLASS,
			NULL,
			NULL,
			&si,
			pi
		);
		
		free(lpCmdLine);
	}
	return returnValue;
}

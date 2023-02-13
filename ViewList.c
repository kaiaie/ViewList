#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <psapi.h>
#include <tlhelp32.h>
#include "resource.h"
#include "MainWindow.h"
#include "Utils.h"

#ifndef INITIAL_BUFFER_SIZE
#define INITIAL_BUFFER_SIZE 8192
#endif
#define BYTES_PER_READ      132


static BOOL IsTheSpawnedProcess(LPSTR lpszExePath);
static int StartGui(HINSTANCE hInstance, int nCmdShow);
static int ReadAndCopyStandardInput(HINSTANCE hInstance, HANDLE hStdIn, 
	LPSTR lpszExePath, LPSTR lpszCmdLine);
static BOOL CreateGuiProcess(PROCESS_INFORMATION *pi, LPSTR lpszExePath,
	LPSTR lpszCmdLine);
static DWORD GetParentProcessID();


/** \brief Application entry point */
int WINAPI
WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpszCmdLine, int nCmdShow)
{
	int     returnValue = 0;
	HANDLE  hStdIn;
	CHAR    szExeFilePath[MAX_PATH];
	
	hInstance = hInst;
	
	GetAppTitle();
	
	GetModuleFileName
	(
		(HMODULE)NULL, 
		szExeFilePath, 
		MAX_PATH
	);
	
	/* If called from the command-line, we run a second copy of the executable.
	** This instance gathers the input from stdin while the second instance 
	** displays a window to the user.  This is so the list can continue to be 
	** displayed without blocking the command prompt
	*/
	if (IsTheSpawnedProcess(szExeFilePath))
	{
		returnValue = StartGui(hInstance, nCmdShow);
	}
	else if ((hStdIn = GetStdHandle(STD_INPUT_HANDLE)) != INVALID_HANDLE_VALUE)
	{
		returnValue = ReadAndCopyStandardInput(hInstance, hStdIn, szExeFilePath, lpszCmdLine);
	}
	else
	{
		returnValue = 1;
	}
	return returnValue;
}


/** \brief Event loop for GUI portion of app */
static int
StartGui(HINSTANCE hInstance, int nCmdShow)
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


/** \brief Callback function for EnumThreadWindows */
BOOL CALLBACK GetListWindow(HWND hwnd, LPARAM lParam)
{
	*((HWND *)lParam) = hwnd;
	/* The app only has one window so terminate the enumeration */
	return FALSE;
}


/** \brief Starts another copy of the process to display the GUI, then post 
*** whatever gets read from standard input to it
*/
static int
ReadAndCopyStandardInput(HINSTANCE hInstance, HANDLE hStdIn, LPSTR lpszExePath, LPSTR lpszCmdLine)
{
	PROCESS_INFORMATION pi;
	HWND                hMainWindow = NULL;
	LPSTR               lpBuffer;
	COPYDATASTRUCT      *lpCopyStruct = NULL;
	DWORD               dwBufferSize;
	DWORD               dwBytesRead;
	DWORD               dwTotalBytesRead;
	LPSTR               lpToken = NULL;
	BOOL                bFileRead;
	int                 returnValue = 1;
	
	if (CreateGuiProcess(&pi, lpszExePath, lpszCmdLine))
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
				bFileRead = ReadFile
				(
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
CreateGuiProcess(LPPROCESS_INFORMATION pi, LPSTR lpszExePath, LPSTR lpszCmdLine)
{
	BOOL        returnValue = FALSE;
	STARTUPINFO si;
	
	si.cb          =  sizeof(STARTUPINFO);
	si.lpReserved  =  NULL;
	si.lpDesktop   =  NULL;
	si.lpTitle     =  NULL;
	si.dwFlags     =  0L;
	si.cbReserved2 =  0;
	si.lpReserved2 =  NULL;
	
	returnValue = CreateProcess
	(
		lpszExePath,
		lpszCmdLine,
		(LPSECURITY_ATTRIBUTES)NULL,
		(LPSECURITY_ATTRIBUTES)NULL,
		FALSE,
		NORMAL_PRIORITY_CLASS,
		NULL,
		NULL,
		&si,
		pi
	);
	
	return returnValue;
}


/** \brief Returns TRUE if this is the GUI process */
static BOOL
IsTheSpawnedProcess(LPSTR lpszExePath)
{
	BOOL bResult = FALSE;
	DWORD pidParent;
	BOOL bSuccess;
	
	pidParent = GetParentProcessID();
	if (pidParent != 0)
	{
		HANDLE hParent = OpenProcess
		(
			SYNCHRONIZE | PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
            FALSE,
			pidParent
		);
		if (hParent != INVALID_HANDLE_VALUE)
		{
			/* We only want the first module name */
			HMODULE modules[1];
			DWORD cbNeeded;
			bSuccess = EnumProcessModules
			(
				hParent,
				modules,
				sizeof(HMODULE),
				&cbNeeded
			);
			if (bSuccess)
			{
				CHAR szParentExeName[MAX_PATH];
				bSuccess = GetModuleFileNameEx
				(
					hParent,
					modules[0],
					szParentExeName,
					MAX_PATH
				);
				if (bSuccess)
				{
					bResult = CompareString
					(
						LOCALE_SYSTEM_DEFAULT,
						NORM_IGNORECASE,
						lpszExePath,
						-1,
						szParentExeName,
						-1
					) == CSTR_EQUAL;
				}
				else
				{
					DisplayErrorMessage((HWND)NULL, IDS_ERROR_MODULES);
					ExitProcess(GetLastError());					
				}
			}
			CloseHandle(hParent);
		}
	}
	else
	{
		DisplayErrorMessage((HWND)NULL, IDS_ERROR_PARENT);
		ExitProcess(GetLastError());
	}
	return bResult;
}


/** \brief Returns the parent PID of the current process
*** \see https://www.codeproject.com/Articles/9893/Get-Parent-Process-PID
**/
static DWORD
GetParentProcessID()
{
	DWORD pidMine = GetCurrentProcessId();
	DWORD pidParent = (DWORD)0;
	HANDLE hThSnapshot;

	hThSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, pidMine);
	if (hThSnapshot != INVALID_HANDLE_VALUE)
	{
		PROCESSENTRY32 peProcessEntry;
		BOOL bMoreProcesses;
		peProcessEntry.dwSize = sizeof(PROCESSENTRY32);
		bMoreProcesses = Process32First(hThSnapshot, &peProcessEntry);
		while (bMoreProcesses)
		{
			if (peProcessEntry.th32ProcessID == pidMine)
			{
				pidParent = peProcessEntry.th32ParentProcessID;
				break;
			}
			bMoreProcesses = Process32Next(hThSnapshot, &peProcessEntry);
		}
		CloseHandle(hThSnapshot);
	}
	return pidParent;
}

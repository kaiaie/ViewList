/**
*** \author Ken Keenan <mailto:ken@kaia.ie>
*** \file Utils.c
*** Miscellaneous utility functions and variables
**/
#include <stdlib.h>
#include "resource.h"
#include "Utils.h"

/* Globals */
HINSTANCE hInstance;
LPTSTR    lpAppTitle;

/* Constants */
static const LPTSTR REG_PATH     = "Software\\Kaia\\ViewList";
static const LPTSTR DIFF_PROGRAM = "DiffProgram";

/** Returns the name of the external diff program from the Registry */
LPTSTR
GetDiffProgram()
{
	LPTSTR returnValue = NULL;
	DWORD  dwBufferSize  = 0;
	DWORD  dwType;
	HKEY   hkey;
	
	if (RegOpenKeyEx(
		HKEY_CURRENT_USER, 
		REG_PATH, 
		(DWORD)0, 
		KEY_READ, 
		&hkey) == ERROR_SUCCESS)
	{
		/* First, find out how big a buffer we need */
		if (RegQueryValueEx(
			hkey, 
			DIFF_PROGRAM,
			(LPDWORD)NULL,
			&dwType,
			(LPBYTE)NULL,
			&dwBufferSize) == ERROR_SUCCESS)
		{
			if (dwType == REG_SZ)
			{
				/* Now retrieve the value */
				returnValue = (LPTSTR)malloc(dwBufferSize);
				if(RegQueryValueEx(
					hkey, 
					DIFF_PROGRAM,
					(LPDWORD)NULL,
					&dwType,
					(LPBYTE)returnValue,
					&dwBufferSize) != ERROR_SUCCESS)
				{
					free(returnValue);
					returnValue = NULL;
				}
			}
		}
		RegCloseKey(hkey);
	}
	return returnValue;
}


/** Returns the string to use for the application window's title bar */
void
GetAppTitle()
{
	lpAppTitle = LoadStringFromResource(IDS_APP_TITLE);
}


/** Displays the specified error message in a message box */
void
DisplayErrorMessage(HWND hWnd, UINT messageCode)
{
	LPTSTR lpMessage;
	
	if ((lpMessage = LoadStringFromResource(messageCode)) != NULL)
	{
		MessageBox(hWnd, 
			lpMessage, 
			lpAppTitle, 
			MB_OK | MB_ICONEXCLAMATION
		);		
	}
	free(lpMessage);
}


/** Loads a string from the stringtable, increasing buffer size if necessary.
*** It is the responsibility of the caller to free it afterwards
**/
LPTSTR
LoadStringFromResource(UINT uID)
{
	LPTSTR lpBuffer     = NULL;
	LPTSTR lpTemp       = NULL;
	DWORD  dwBufferSize = 64;
	int    result       = 0;
	
	if ((lpBuffer = malloc(dwBufferSize)) == NULL)
	{
		DisplayErrorMessage((HWND)NULL, IDS_ERROR_OUT_OF_MEMORY);
		ExitProcess(0);
	}
	do
	{
		result = LoadString(hInstance, uID, lpBuffer, dwBufferSize);
		if (result == 0)
		{
			/* Error */
			if (lpBuffer != NULL)
			{
				free(lpBuffer);
				lpBuffer = NULL;
				break;
			}
		}
		else if ((result + 1) <= dwBufferSize)
		{
			/* String fit into buffer */
			break;
		}
		/* Buffer too small: enlarge */
		dwBufferSize *= 2;
		if ((lpTemp = realloc(lpBuffer, dwBufferSize)) == NULL)
		{
				DisplayErrorMessage((HWND)NULL, IDS_ERROR_OUT_OF_MEMORY);
				ExitProcess(0);
		}
		lpBuffer = lpTemp;
	} while (TRUE);
	return lpBuffer;
}

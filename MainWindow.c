#include <stdlib.h>
#include "MainWindow.h"
#include "resource.h"
#include "Utils.h"

static WNDPROC wpListBoxProc;
static HFONT   listFont = NULL;

static void
NotImplemented(HWND hwnd)
{
	DisplayErrorMessage(hwnd, IDS_WARN_NOT_IMPLEMENTED);
}


/** Open selected item */
static void
DoOpenItem(HWND hwnd)
{
	int 	 nListCount;
	int 	 ii;
	LPSTR  lpFile;
	size_t cbFile;
	
	nListCount = SendMessage(hwnd, LB_GETCOUNT, (WPARAM)0, (LPARAM)0);
	for (ii = 0; ii < nListCount; ++ii)
	{
		if (SendMessage(hwnd, LB_GETSEL, (WPARAM)ii, (LPARAM)0) > 0)
		{
			cbFile = (size_t)SendMessage(hwnd, LB_GETTEXTLEN, (WPARAM)ii, (LPARAM)0);
			lpFile = malloc(cbFile + 1);
			if (SendMessage(hwnd, LB_GETTEXT, (WPARAM)ii, (LPARAM)lpFile) > 0)
			{
				ShellExecute(
					hwnd, 
					"open", 
					lpFile, 
					(LPTSTR)NULL,
					(LPCTSTR) NULL,
					SW_SHOWNORMAL
				);
			}
			free(lpFile);
		}
	}
}


/** Copy selected items to the Clipboard */
static void
DoCopySelected(HWND hwnd)
{
	HGLOBAL hCopyBuffer = 0;
	LPTSTR  lpCopyBuffer = NULL;
	DWORD   dwBufferSize = 0;
	int     ii, jj;
	
	for (ii = 0; ii < 2; ++ii)
	{
		if (ii == 1)
		{
			hCopyBuffer  = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, dwBufferSize);
			lpCopyBuffer = GlobalLock(hCopyBuffer);
			FillMemory(lpCopyBuffer, dwBufferSize, (BYTE)0);
		}
		for (jj = 0; jj < SendMessage(hwnd, LB_GETCOUNT, (WPARAM)0, (LPARAM)0); ++jj)
		{
			if (SendMessage(hwnd, LB_GETSEL, (WPARAM)jj, (LPARAM)0) > 0)
			{
				if (ii == 0)
				{
					dwBufferSize += SendMessage(hwnd, LB_GETTEXTLEN, (WPARAM)jj, (LPARAM)0) + 2;
				}
				else
				{
					SendMessage(
						hwnd, 
						LB_GETTEXT, 
						(WPARAM)jj, 
						(LPARAM)&lpCopyBuffer[lstrlen(lpCopyBuffer)]
					);
					lstrcat(lpCopyBuffer, "\r\n");
				}
			}
		}
		if (ii == 0) dwBufferSize += 1;
	}
	GlobalUnlock(hCopyBuffer);
	if (OpenClipboard(hwnd))
	{
		EmptyClipboard();
		SetClipboardData(CF_TEXT, hCopyBuffer);
		CloseClipboard();
	}
}


/** Display options dialog */
static void
DoOptions(HWND hwnd)
{
	NotImplemented(hwnd);
}


/** Run external diff program */
static void
DoDiff(HWND hwnd)
{
	int                 nSelCount;
	int                 anSelected[2];
	int                 cbBufferSize = 0;
	LPTSTR              lpDiffCommand = NULL;
	static LPTSTR       lpDiffProgram = NULL;
	PROCESS_INFORMATION pi;
	STARTUPINFO         si;
	
	nSelCount = SendMessage(hwnd, LB_GETSELCOUNT, (WPARAM)0, (LPARAM)0);
	if (lpDiffProgram == NULL) lpDiffProgram = GetDiffProgram();
	if (lpDiffProgram != NULL && nSelCount == 2)
	{
		if (SendMessage(hwnd, LB_GETSELITEMS, (WPARAM)2, (LPARAM)anSelected) > 0)
		{
			cbBufferSize = 
				lstrlen(lpDiffProgram)
				+ 1
				+ SendMessage(hwnd, LB_GETTEXTLEN, (WPARAM)anSelected[0], (LPARAM) 0)
				+ 3
				+ SendMessage(hwnd, LB_GETTEXTLEN, (WPARAM)anSelected[1], (LPARAM) 0)
				+ 3;
			lpDiffCommand = (LPSTR)malloc(cbBufferSize);
			FillMemory(lpDiffCommand, cbBufferSize, (BYTE)0);
			lstrcat(lpDiffCommand, lpDiffProgram);
			lstrcat(lpDiffCommand, " \"");
			SendMessage(
				hwnd, 
				LB_GETTEXT, 
				(WPARAM)anSelected[0], 
				(LPARAM)&lpDiffCommand[lstrlen(lpDiffCommand)]
			);
			lstrcat(lpDiffCommand, "\" \"");
			SendMessage(
				hwnd, 
				LB_GETTEXT, 
				(WPARAM)anSelected[1], 
				(LPARAM)&lpDiffCommand[lstrlen(lpDiffCommand)]
			);
			lstrcat(lpDiffCommand, "\"");
			si.cb          =  sizeof(STARTUPINFO);
			si.lpReserved  =  NULL;
			si.lpDesktop   =  NULL;
			si.lpTitle     =  NULL;
			si.dwFlags     =  0L;
			si.cbReserved2 =  0;
			si.lpReserved2 =  NULL;
			CreateProcess(
				(LPCTSTR)NULL,
				lpDiffCommand,
				(LPSECURITY_ATTRIBUTES)NULL,
				(LPSECURITY_ATTRIBUTES)NULL,
				FALSE,
				NORMAL_PRIORITY_CLASS,
				NULL,
				NULL,
				&si,
				&pi
			);			
			free(lpDiffCommand);
		}
	}
	else
	{
		MessageBeep(MB_ICONASTERISK);
	}
}

/** Select all items in list */
static void
DoSelectAll(HWND hwnd)
{
	int nListCount;
	int ii;
	
	nListCount = SendMessage(hwnd, LB_GETCOUNT, (WPARAM)0, (LPARAM)0);
	for (ii = 0; ii < nListCount; ++ii)
	{
		if (SendMessage(hwnd, LB_GETSEL, (WPARAM)ii, (LPARAM)0) == 0)
		{
			SendMessage(hwnd, LB_SETSEL, (WPARAM)TRUE, (LPARAM)ii);
		}
	}
}


static void
DoOpenInTextEditor(HWND hwnd)
{
	NotImplemented(hwnd);
}


static void
DoOpenInBinaryEditor(HWND hwnd)
{
	NotImplemented(hwnd);
}


static void
DoHelpAbout(HWND hwnd)
{
	LPTSTR lpMessage = NULL;
	
	if ((lpMessage = LoadStringFromResource(IDS_APP_ABOUT_MSG)) != NULL)
	{
		MessageBox(hwnd, 
			lpMessage, 
			lpAppTitle, 
			MB_OK | MB_ICONINFORMATION
		);		
	}
	if (lpMessage != NULL) free(lpMessage);
}


/** Window procedure */
LRESULT APIENTRY
MainWindowProc(
	HWND hwnd, 
	UINT uMsg, 
	WPARAM wParam, 
	LPARAM lParam
)
{
	WORD           wCmdID;
	COPYDATASTRUCT *cds = NULL;
	LPSTR          lpLocalCopy;
	
	switch(uMsg)
	{
		case WM_COPYDATA:
			/* This is data being sent from the other instance of the app */
			cds = (COPYDATASTRUCT *) lParam;
			if (cds->lpData != NULL)
			{
				/* Make a local copy of the data */
				lpLocalCopy = (LPSTR)malloc(cds->cbData);
				CopyMemory(lpLocalCopy, cds->lpData, cds->cbData);
				SendMessage(hwnd, LB_ADDSTRING, (WPARAM)0, (LPARAM)lpLocalCopy);
			}
			return TRUE;
		case WM_LBUTTONDBLCLK:
			DoOpenItem(hwnd);
			return 0;
		case WM_COMMAND:
			wCmdID = LOWORD(wParam);
			if (wCmdID != IDM_FILE_EXIT)
			{
				if (wCmdID == IDM_FILE_OPEN)
					DoOpenItem(hwnd);
				else if (wCmdID == IDM_FILE_DIFF)
					DoDiff(hwnd);
				else if (wCmdID == IDM_FILE_OPENTEXT)
					DoOpenInTextEditor(hwnd);
				else if (wCmdID == IDM_FILE_OPENBINARY)
					DoOpenInBinaryEditor(hwnd);
				else if (wCmdID == IDM_EDIT_COPY)
					DoCopySelected(hwnd);
				else if (wCmdID == IDM_EDIT_SELECTALL)
					DoSelectAll(hwnd);
				else if (wCmdID == IDM_EDIT_OPTIONS)
					DoOptions(hwnd);
				else if (wCmdID == IDM_HELP_ABOUT)
					DoHelpAbout(hwnd);
				return 0;
			}
			/* FALL THROUGH */
		case WM_CLOSE:
			/* Remove subclassing */
			SetWindowLong(hwnd, GWL_WNDPROC, (LONG)wpListBoxProc);
			DestroyWindow(hwnd);
			if (listFont != NULL) DeleteObject(listFont);
			PostQuitMessage(0);
			return 0;
		default:
			return CallWindowProc(wpListBoxProc, hwnd, uMsg, wParam, lParam);
	}
}     

/** Create and initialise the application window */
HWND
CreateMainWindow(HINSTANCE hInst)
{
	HWND hMainWindow = NULL;
	NONCLIENTMETRICS    ncm;
	HICON               appIcon;

	hMainWindow = CreateWindowEx(
		WS_EX_CLIENTEDGE,
		TEXT("LISTBOX"), 
		lpAppTitle, 
		WS_OVERLAPPEDWINDOW | WS_VSCROLL | LBS_EXTENDEDSEL, 
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		(HWND)NULL, 
		LoadMenu(hInst, MAKEINTRESOURCE(IDM_MAIN)), 
		hInst, 
		(LPVOID)NULL
	);
	if (hMainWindow != NULL)
	{
		/* Subclass window so we can cleanly handle WM_CLOSE */
		wpListBoxProc = (WNDPROC)SetWindowLong(
			hMainWindow, 
			GWL_WNDPROC, 
			(LONG)MainWindowProc
		);
		/* Get font to use */
		ncm.cbSize = sizeof(NONCLIENTMETRICS);
		if (SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, (PVOID)&ncm, 0))
		{
			listFont = CreateFontIndirect(&(ncm.lfMessageFont));
			SendMessage(hMainWindow, WM_SETFONT, (WPARAM)listFont, 
					MAKELPARAM(TRUE, 0));
		}
		
		/* Set icon */
		if ((appIcon = LoadIcon(hInst, MAKEINTRESOURCE(0))) != NULL)
		{
			SendMessage(hMainWindow, WM_SETICON, (WPARAM)TRUE, (LPARAM)appIcon);
		}
	}
	return hMainWindow;
}

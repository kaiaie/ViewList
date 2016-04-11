#ifndef UTILS_H
#define UTILS_H
#include <windows.h>

extern HINSTANCE hInstance;
extern LPTSTR    lpAppTitle;

LPTSTR GetDiffProgram();
void   GetAppTitle();
void   DisplayErrorMessage(HWND hWnd, UINT messageCode);
LPTSTR LoadStringFromResource(UINT uID);

#endif



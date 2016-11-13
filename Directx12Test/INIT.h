#ifndef _INIT_H_
#define _INIT_H_

#include <stdio.h>
#include <windows.h>
#include <tchar.h>
#include <wrl.h>	

class INIT
{
public:
	static int Window(INIT* init, HINSTANCE hInstance, LPCWSTR WINDOW_CLASS, int WINDOW_WIDTH, int WINDOW_HEIGHT, DWORD WINDOW_STYLE, LPCWSTR WINDOW_TITLE);
	static bool InitWindow(INIT* init, HINSTANCE hInstance, LPCWSTR WINDOW_CLASS, int WINDOW_WIDTH, int WINDOW_HEIGHT, DWORD WINDOW_STYLE, LPCWSTR WINDOW_TITLE);
	static HWND GetHwnd() {return m_hwnd;}

protected:
	static LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

private:
	static HWND m_hwnd;
};	
#endif // !_INIT_H_
#include "INIT.h"


HWND INIT::m_hwnd = nullptr;

// ウィンドウプロシージャ
LRESULT CALLBACK INIT::WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	INIT* instance = reinterpret_cast<INIT*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

	switch (message)
	{
	case WM_CREATE:
		{
			// Save the DXSample* passed in to CreateWindow.
			LPCREATESTRUCT pCreateStruct = reinterpret_cast<LPCREATESTRUCT>(lParam);
			SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pCreateStruct->lpCreateParams));
		}
		return 0;

	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}


bool INIT::InitWindow(INIT* init, HINSTANCE hInstance, LPCWSTR WINDOW_CLASS, int WINDOW_WIDTH, int WINDOW_HEIGHT, DWORD WINDOW_STYLE, LPCWSTR WINDOW_TITLE)
{

	// ウィンドウを作成
	WNDCLASSEX	windowClass = {0};
	// ウィンドウクラスを登録
	windowClass.cbSize = sizeof(WNDCLASSEX);
	windowClass.style = CS_HREDRAW | CS_VREDRAW;
	windowClass.lpfnWndProc = WindowProc;
	windowClass.hInstance = hInstance;
	windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	windowClass.lpszClassName = WINDOW_CLASS;
	RegisterClassEx(&windowClass);

	RECT windowRect = { 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT };

	AdjustWindowRect(&windowRect, WINDOW_STYLE, FALSE);

	// ウィンドウを作成
	m_hwnd = CreateWindow(
		WINDOW_CLASS,
		WINDOW_TITLE, 
		WINDOW_STYLE,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		windowRect.right - windowRect.left,
		windowRect.bottom - windowRect.top,
		nullptr,		// We have no parent window, NULL.
		nullptr,		// We aren't using menus, NULL.
		hInstance,
		init);		// We aren't using multiple windows, NULL.

	ShowWindow(m_hwnd, SW_SHOW);
	UpdateWindow(m_hwnd);

	return true;
}

int INIT::Window(INIT* init, HINSTANCE hInstance, LPCWSTR WINDOW_CLASS, int WINDOW_WIDTH, int WINDOW_HEIGHT, DWORD WINDOW_STYLE, LPCWSTR WINDOW_TITLE)
{
	// ウィンドウの初期化.
	if (!InitWindow(init, hInstance, WINDOW_CLASS, WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_STYLE, WINDOW_TITLE))
	{
		MessageBox(m_hwnd, _T("ウィンドウの初期化が失敗しました"), _T("InitWindow"), MB_OK | MB_ICONEXCLAMATION);
		return 0;
	}

	return 1;
}

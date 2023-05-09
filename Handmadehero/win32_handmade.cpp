#include <Windows.h>

#define local_persist static
#define global_variable static
#define internal static

global_variable bool Running;
global_variable BITMAPINFO BitmapInfo;
global_variable VOID* BitmapMemory;


internal void Win32ResizeDIBSection(int Width, int Height)
{

	BitmapInfo.bmiHeader.biSize = sizeof(BitmapInfo.bmiHeader);
	BitmapInfo.bmiHeader.biWidth = Width;
	BitmapInfo.bmiHeader.biHeight = Height;
	BitmapInfo.bmiHeader.biPlanes = 1;
	BitmapInfo.bmiHeader.biBitCount = 32;
	BitmapInfo.bmiHeader.biCompression = BI_RGB;	

	int BytesPerPixel = 4;
	int BitmapMemorySize = (Width * Height) * BytesPerPixel;
	//BitmapMemory = VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT, )
}

internal void Win32UpdateWindow(HDC DeviceContext, int Width, int Height, int X, int Y)
{
	StretchDIBits(DeviceContext, X, Y, Width, Height, X, Y, Width, Height,
		 BitmapMemory,
		 &BitmapInfo,
			DIB_RGB_COLORS, SRCCOPY);
}

LRESULT DefWindowProcA(
	HWND   hWnd,
	UINT   Msg,
	WPARAM wParam,
	LPARAM lParam
);

LRESULT CALLBACK Win32MainWindowCallBack(
	HWND Window,
	UINT Message,
	WPARAM WParam,
	LPARAM LParam
)
{
	LRESULT Result = 0;

	switch (Message) 
	{
	case WM_SIZE: 
	{
		RECT ClientRect;
		GetClientRect(Window, &ClientRect);
		int Width = ClientRect.right - ClientRect.left;
		int Height = ClientRect.bottom - ClientRect.top;
		Win32ResizeDIBSection(Width, Height);
		OutputDebugString(L"WM_SIZE\n");
	}break;

	case WM_DESTROY: 
	{
		Running = false;
	}break;

	case WM_CLOSE: 
	{
		Running = false;
	}break;

	case WM_ACTIVATEAPP: 
	{
		OutputDebugString(L"WM_ACTIVATEAPP\n");
	}break;

	case WM_PAINT: 
	{
		PAINTSTRUCT Paint;
		HDC DeviceContext = BeginPaint(Window, &Paint);
		int Width = Paint.rcPaint.right - Paint.rcPaint.left;
		int Height = Paint.rcPaint.bottom - Paint.rcPaint.top;
		int X = Paint.rcPaint.left;
		int Y = Paint.rcPaint.top;
		Win32UpdateWindow(DeviceContext, Width, Height, X, Y);
		EndPaint(Window, &Paint);
	}break;

	default: 
	{
		Result = DefWindowProcA(Window, Message, WParam, LParam);
		//		OutputDebugString(L"DEFAULT\n");
	}
	}

	return (Result);
}

int CALLBACK WinMain(HINSTANCE Instance, HINSTANCE PrevInstance, LPSTR CmdLine, int CmdShow) 
{

	WNDCLASSA WindowClass = {};

	WindowClass.lpfnWndProc = Win32MainWindowCallBack;
	WindowClass.hInstance = Instance;
	//	HICON     hIcon;
	WindowClass.lpszClassName = "HandMadeHeroWindowClass";

	if (RegisterClassA(&WindowClass)) 
	{
		HWND WindowHandler = CreateWindowExA(
			0,
			WindowClass.lpszClassName,
			"Handmade Hero",
			WS_OVERLAPPEDWINDOW | WS_VISIBLE,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			0,
			0,
			Instance,
			0
		);
		if (WindowHandler) 
		{
			MSG Message;
			Running = true;
			while (Running) 
			{
				BOOL MessageResult = GetMessage(&Message, 0, 0, 0);
				if (MessageResult > 0) 
				{
					TranslateMessage(&Message);
					DispatchMessage(&Message);
				}
				else 
				{
					break;
				}
			}

		}
		else 
		{
			// TODO
		}
	}
	else 
	{
		// TODO
	}

	return 0;
}
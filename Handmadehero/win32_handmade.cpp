#include <Windows.h>
#include <stdint.h>

#define local_persist static
#define global_variable static
#define internal static

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

global_variable bool Running;

global_variable BITMAPINFO BitmapInfo;
global_variable VOID* BitmapMemory;
global_variable int BitmapWidth;
global_variable int BitmapHeight;
global_variable int BytesPerPixel = 4;


internal void RenderWeirdGradient(int BlueOffset, int GreenOffset)
{

	int Width = BitmapWidth;

	int BitmapMemorySize = (BitmapWidth * BitmapHeight) * BytesPerPixel;
	BitmapMemory = VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);

	int Pitch = Width * BytesPerPixel;
	uint8* Row = (uint8*)BitmapMemory;

	for (int Y = 0; Y < BitmapHeight; Y++)
	{
		uint32* Pixel = (uint32*)Row;

		for (int X = 0; X < BitmapWidth; X++)
		{
			uint8 Blue = (X + BlueOffset);
			uint8 Green = (Y + GreenOffset);
			*Pixel++ = ((Green << 8) | Blue);
		}

		Row += Pitch;
	}
}

internal void Win32ResizeDIBSection(int Width, int Height)
{

	if (BitmapMemory) 
	{
		VirtualFree(BitmapMemory, 0, MEM_RELEASE);
	}

	BitmapWidth = Width;
	BitmapHeight = Height;


	BitmapInfo.bmiHeader.biSize = sizeof(BitmapInfo.bmiHeader);
	BitmapInfo.bmiHeader.biWidth = BitmapWidth;
	BitmapInfo.bmiHeader.biHeight = -BitmapHeight;
	BitmapInfo.bmiHeader.biPlanes = 1;
	BitmapInfo.bmiHeader.biBitCount = 32;
	BitmapInfo.bmiHeader.biCompression = BI_RGB;	

}

internal void Win32UpdateWindow(HDC DeviceContext, RECT *WindowRec, int Width, int Height, int X, int Y)
{
	int WindowWidth = WindowRec->right - WindowRec->left;
	int WindowHeight = WindowRec->bottom - WindowRec->top;

	StretchDIBits(DeviceContext, 0, 0, BitmapWidth, BitmapHeight, 0, 0, WindowWidth, WindowHeight, 
		BitmapMemory, &BitmapInfo, DIB_RGB_COLORS, SRCCOPY);
}

LRESULT CALLBACK Win32MainWindowCallBack(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam)
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

		RECT ClientRect;
		GetClientRect(Window, &ClientRect);

		Win32UpdateWindow(DeviceContext, &ClientRect, Width, Height, X, Y);
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
		HWND Window = CreateWindowExA(
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
		if (Window) 
		{
			int XOffset = 0;
			int YOffset = 0;
			Running = true;
			while (Running) 
			{
				MSG Message;

				while (PeekMessageA(&Message, 0, 0, 0, PM_REMOVE)) 
				{
					if (Message.message == WM_QUIT) 
					{
						Running = false;
					}

					TranslateMessage(&Message);
					DispatchMessage(&Message);
					
				}
				
				HDC DeviceContext = GetDC(Window);
				RECT ClientRect;
				GetClientRect(Window, &ClientRect);
				int WindowWidth = ClientRect.right - ClientRect.left;
				int WindowHeight = ClientRect.bottom - ClientRect.top;
				RenderWeirdGradient(XOffset, YOffset);
				Win32UpdateWindow(DeviceContext, &ClientRect, WindowWidth, WindowHeight, 0, 0);
				ReleaseDC(Window, DeviceContext);
				XOffset++;
				YOffset++;

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
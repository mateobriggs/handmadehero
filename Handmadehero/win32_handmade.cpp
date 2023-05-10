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

struct win32_offscreen_buffer {
	BITMAPINFO Info;
	VOID* Memory;
	int Width;
	int Height;
	int BytesPerPixel;
	int Pitch;
};

global_variable bool Running;
global_variable win32_offscreen_buffer GlobalBackBuffer;

internal void RenderWeirdGradient(win32_offscreen_buffer Buffer, int BlueOffset, int GreenOffset)
{

	int Width = Buffer.Width;

	uint8* Row = (uint8*)Buffer.Memory;

	for (int Y = 0; Y < Buffer.Height; Y++)
	{
		uint32* Pixel = (uint32*)Row;

		for (int X = 0; X < Buffer.Width; X++)
		{
			uint8 Blue = (X + BlueOffset);
			uint8 Green = (Y + GreenOffset);
			*Pixel++ = ((Green << 8) | Blue);
		}

		Row += Buffer.Pitch;
	}
}

internal void Win32ResizeDIBSection(win32_offscreen_buffer* Buffer, int Width, int Height)
{

	if (Buffer->Memory) 
	{
		VirtualFree(Buffer->Memory, 0, MEM_RELEASE);
	}

	Buffer->Width = Width;
	Buffer->Height = Height;
	Buffer->BytesPerPixel = 4;

	Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader);
	Buffer->Info.bmiHeader.biWidth = Buffer->Width;
	Buffer->Info.bmiHeader.biHeight = -Buffer->Height;
	Buffer->Info.bmiHeader.biPlanes = 1;
	Buffer->Info.bmiHeader.biBitCount = 32;
	Buffer->Info.bmiHeader.biCompression = BI_RGB;	
	Buffer->Pitch = Width * Buffer->BytesPerPixel;
	int BitmapMemorySize = (Buffer->Width * Buffer->Height) * Buffer->BytesPerPixel;
	Buffer->Memory = VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);


}

struct win32_window_dimension
{
	int Width;
	int Height;
};

internal win32_window_dimension Win32GetWindowDimension(HWND Window)
{
	win32_window_dimension Result;

	RECT ClientRect;
	GetClientRect(Window, &ClientRect);

	Result.Width = ClientRect.right - ClientRect.left;
	Result.Height = ClientRect.bottom - ClientRect.top;

	return(Result);
}

internal void Win32DisplayBufferInWindow(win32_offscreen_buffer Buffer, HDC DeviceContext, int WindowWidth, int WindowHeight, int Width, int Height, int X, int Y)
{

	StretchDIBits(DeviceContext, 0, 0, WindowWidth, WindowHeight, 0, 0, Buffer.Width, Buffer.Height,
		Buffer.Memory, &Buffer.Info, DIB_RGB_COLORS, SRCCOPY);

}

LRESULT CALLBACK Win32MainWindowCallBack(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam)
{
	LRESULT Result = 0;

	switch (Message) 
	{
	case WM_SIZE: 
	{
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

		win32_window_dimension WindowDimension = Win32GetWindowDimension(Window);

		Win32DisplayBufferInWindow(GlobalBackBuffer, DeviceContext, WindowDimension.Width, WindowDimension.Height, Width, Height, X, Y);
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

	WindowClass.style = CS_HREDRAW | CS_VREDRAW;
	WindowClass.lpfnWndProc = Win32MainWindowCallBack;
	WindowClass.hInstance = Instance;
	//	HICON     hIcon;
	WindowClass.lpszClassName = "HandMadeHeroWindowClass";
	Win32ResizeDIBSection(&GlobalBackBuffer, 1280, 720);



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
				win32_window_dimension WindowDimension = Win32GetWindowDimension(Window);

				Win32DisplayBufferInWindow(GlobalBackBuffer, DeviceContext, WindowDimension.Width, WindowDimension.Height, WindowDimension.Width, WindowDimension.Height, 0, 0);
				RenderWeirdGradient(GlobalBackBuffer, XOffset, YOffset);
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
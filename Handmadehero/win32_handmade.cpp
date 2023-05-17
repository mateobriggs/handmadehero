#include <Windows.h>
#include <stdint.h>
#include <Xinput.h>
#include <dsound.h>
#include <math.h>


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

typedef int32 bool32;

typedef float real32;
typedef double real64;
#define Pi32 3.14159265359f

#include "Handmade.h"
#include "Handmade.cpp"

// INPUT GET STATE
#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE* pState)
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub)
{
	return(ERROR_DEVICE_NOT_CONNECTED);
}
global_variable x_input_get_state* XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_

// INPUT SET STATE
#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION* pVibration)
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub)
{
	return(ERROR_DEVICE_NOT_CONNECTED);
}
global_variable x_input_set_state* XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_

#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND * ppDS, LPUNKNOWN pUnkOuter)
typedef DIRECT_SOUND_CREATE(direct_sound_create);

#define CREATE_SOUND_BUFFER(name) HRESULT WINAPI name(LPCDSBUFFERDESC pcDSBufferDesc, LPDIRECTSOUNDBUFFER *ppDSBuffer, LPUNKNOWN pUnkOuter)
typedef CREATE_SOUND_BUFFER(create_sound_buffer);


struct win32_offscreen_buffer {
	BITMAPINFO Info;
	VOID* Memory;
	int Width;
	int Height;
	int Pitch;
};
internal void Win32LoadXInput()
{
	HMODULE XInputLibrary = LoadLibraryA("xinput1_4.dll");
	if (XInputLibrary)
	{
		XInputGetState = (x_input_get_state*)GetProcAddress(XInputLibrary, "XInputGetState");
		XInputSetState = (x_input_set_state*)GetProcAddress(XInputLibrary, "XInputSetState");
	}
}

global_variable bool GlobalRunning;
global_variable win32_offscreen_buffer GlobalBackBuffer;
global_variable LPDIRECTSOUNDBUFFER GlobalSecondaryBuffer;

internal void Win32InitDSound(HWND Window, int32 BufferSize, int32 SamplesPerSecond)
{
	//Load the library

	HMODULE DSoundLibrary = LoadLibraryA("Dsound.dll");

	if (DSoundLibrary)
	{

		direct_sound_create* DirectSoundCreate = (direct_sound_create*)GetProcAddress(DSoundLibrary, "DirectSoundCreate");

		LPDIRECTSOUND DirectSoundPointer;

		if (DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0, &DirectSoundPointer, 0)))
		{

			WAVEFORMATEX  WaveFormat;
			WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
			WaveFormat.nChannels = 2;
			WaveFormat.nSamplesPerSec = SamplesPerSecond;
			WaveFormat.wBitsPerSample = 16;
			WaveFormat.nBlockAlign = (WaveFormat.nChannels * WaveFormat.wBitsPerSample) / 8;
			WaveFormat.nAvgBytesPerSec = WaveFormat.nSamplesPerSec * WaveFormat.nBlockAlign;
			WaveFormat.cbSize = 0;

			if (SUCCEEDED(DirectSoundPointer->SetCooperativeLevel(Window, DSSCL_PRIORITY)))
			{
				LPDIRECTSOUNDBUFFER PrimaryBuffer;
				DSBUFFERDESC BufferDescription = {};

				BufferDescription.dwSize = sizeof(BufferDescription);
				BufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;

				if (SUCCEEDED(DirectSoundPointer->CreateSoundBuffer(&BufferDescription, &PrimaryBuffer, 0)))
				{
					
						if (SUCCEEDED(PrimaryBuffer->SetFormat(&WaveFormat)))
						{
							// We have finally set the format!!
							OutputDebugStringA("Primary Buffer Format set with succes\n");
						}
						else
						{
							//Diagnostic
						}
				}
				else
				{
					//Diagnostic
				}
			

			}
			else
			{
				//Diagnostic
			}

			DSBUFFERDESC BufferDescription = {};
			BufferDescription.dwSize = sizeof(BufferDescription);
			BufferDescription.dwFlags = 0;
			BufferDescription.dwBufferBytes = BufferSize;
			BufferDescription.lpwfxFormat = &WaveFormat;
			if (SUCCEEDED(DirectSoundPointer->CreateSoundBuffer(&BufferDescription, &GlobalSecondaryBuffer, 0)))
			{
				OutputDebugStringA("Secondary Buffer created with succes\n");
			}


		}

		else
		{
			//Diagnostic
		}

		//Get a DirectSoun object



		//"Create" a primary buffer


		//"Create" a secondary buffer

	}
	else
	{
		//Diagnostic
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

	int BytesPerPixel = 4;

	Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader);
	Buffer->Info.bmiHeader.biWidth = Buffer->Width;
	Buffer->Info.bmiHeader.biHeight = -Buffer->Height;
	Buffer->Info.bmiHeader.biPlanes = 1;
	Buffer->Info.bmiHeader.biBitCount = 32;
	Buffer->Info.bmiHeader.biCompression = BI_RGB;	
	Buffer->Pitch = Width * BytesPerPixel;
	int BitmapMemorySize = (Buffer->Width * Buffer->Height) * BytesPerPixel;
	Buffer->Memory = VirtualAlloc(0, BitmapMemorySize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);


}

struct win32_window_dimension
{
	int Width;
	int Height;
};

struct win32_sound_output
{
	int SamplesPerSecond;
	int ToneHz;
	uint32 RunningSampleIndex;
	int WavePeriod;
	int BytesPerSample;
	int SecondaryBufferSize;
};

void Win32FillBuffer(win32_sound_output* SoundOutput, DWORD ByteToLock, DWORD BytesToWrite)
{
	LPVOID Region1;
	DWORD Region1Size;
	LPVOID Region2;
	DWORD Region2Size;

	HRESULT Error = GlobalSecondaryBuffer->Lock(ByteToLock, BytesToWrite, &Region1, &Region1Size, &Region2, &Region2Size, 0);
	if (SUCCEEDED(Error))
	{
		int16* SampleOut = (int16*)Region1;
		DWORD Region1SampleCount = Region1Size / SoundOutput->BytesPerSample;
		int16 ToneVolume = 3000;
		for (DWORD SampleIndex = 0; SampleIndex < Region1SampleCount; SampleIndex++)
		{
			real32 t = 2.0f * Pi32 * ((real32)SoundOutput->RunningSampleIndex / (real32)SoundOutput->WavePeriod);
			real32 SinValue = sinf(t);
			int16 SampleValue = (int16)(SinValue * ToneVolume);
			*SampleOut++ = SampleValue;
			*SampleOut++ = SampleValue;
			SoundOutput->RunningSampleIndex++;
		}
		SampleOut = (int16*)Region2;
		DWORD Region2SampleCount = Region2Size / SoundOutput->BytesPerSample;

		for (DWORD SampleIndex = 0; SampleIndex < Region2SampleCount; SampleIndex++)
		{
			real32 t = 2.0f * Pi32 * ((real32)SoundOutput->RunningSampleIndex / (real32)SoundOutput->WavePeriod);
			real32 SinValue = sinf(t);
			int16 SampleValue = (int16)(SinValue * ToneVolume);
			*SampleOut++ = SampleValue;
			*SampleOut++ = SampleValue;
			SoundOutput->RunningSampleIndex++;
		}
		GlobalSecondaryBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
	}
}

internal win32_window_dimension Win32GetWindowDimension(HWND Window)
{
	win32_window_dimension Result;

	RECT ClientRect;
	GetClientRect(Window, &ClientRect);

	Result.Width = ClientRect.right - ClientRect.left;
	Result.Height = ClientRect.bottom - ClientRect.top;

	return(Result);
}

internal void Win32DisplayBufferInWindow(win32_offscreen_buffer* Buffer, HDC DeviceContext, int WindowWidth, int WindowHeight)
{

	StretchDIBits(DeviceContext, 0, 0, WindowWidth, WindowHeight, 0, 0, Buffer->Width, Buffer->Height,
		Buffer->Memory, &Buffer->Info, DIB_RGB_COLORS, SRCCOPY);

}

internal LRESULT CALLBACK Win32MainWindowCallBack(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam)
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
		GlobalRunning = false;
	}break;

	case WM_CLOSE: 
	{
		GlobalRunning = false;
	}break;

	case WM_ACTIVATEAPP: 
	{
		OutputDebugString(L"WM_ACTIVATEAPP\n");
	}break;

	case WM_KEYDOWN:
	{

	}break;
	case WM_KEYUP:
	{
		WPARAM VKCode = WParam;
		bool32 WasDown = (LParam & (1 << 30));
		bool32 IsDown = (LParam & (1 << 31));
		
		if (VKCode == 'W')
		{
			OutputDebugStringA("W");
		}
		
		if (VKCode == 'A')
		{
		}
		
		if (VKCode == 'S')
		{
		}
		
		if (VKCode == 'D')
		{
		}
		
		if (VKCode == 'Q')
		{
		}
		
		if (VKCode == 'E')
		{
		}
		
		if (VKCode == VK_UP)
		{
		}
		
		if (VKCode == VK_DOWN)
		{
		}
		
		if (VKCode == VK_RIGHT)
		{
		}
		
		if (VKCode == VK_LEFT)
		{
		}

		if (VKCode == VK_ESCAPE)
		{
			OutputDebugStringA("ESCAPE: ");
			if (IsDown)
			{
				OutputDebugStringA("Is Down");
			}
			
			if (WasDown)
			{
				OutputDebugStringA("Was Down");
			}
			OutputDebugStringA("\n");
		}

		if (VKCode == VK_SPACE)
		{
		}
	}break;

	case WM_PAINT: 
	{
		PAINTSTRUCT Paint;
		HDC DeviceContext = BeginPaint(Window, &Paint);

		win32_window_dimension WindowDimension = Win32GetWindowDimension(Window);

		Win32DisplayBufferInWindow(&GlobalBackBuffer, DeviceContext, WindowDimension.Width, WindowDimension.Height);
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



int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) 
{
	Win32LoadXInput();

	WNDCLASSA WindowClass = {};

	WindowClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	WindowClass.lpfnWndProc = Win32MainWindowCallBack;
	WindowClass.hInstance = hInstance;
	//	HICON     hIcon;
	WindowClass.lpszClassName = "HandMadeHeroWindowClass";
	Win32ResizeDIBSection(&GlobalBackBuffer, 1280, 720);
	LARGE_INTEGER PerfCountFrequencyResult;
	QueryPerformanceFrequency(&PerfCountFrequencyResult);
	int64 PerfCountFrequency = PerfCountFrequencyResult.QuadPart;

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
			hInstance,
			0
		);

		if (Window) 
		{
			HDC DeviceContext = GetDC(Window);

			int XOffset = 0;
			int YOffset = 0;

			win32_sound_output SoundOutput = {};

			SoundOutput.SamplesPerSecond = 48000;
			SoundOutput.ToneHz = 356;
			SoundOutput.RunningSampleIndex = 0;
			SoundOutput.WavePeriod = SoundOutput.SamplesPerSecond / SoundOutput.ToneHz;
			SoundOutput.BytesPerSample = sizeof(int16) * 2;
			SoundOutput.SecondaryBufferSize = SoundOutput.SamplesPerSecond * SoundOutput.BytesPerSample;

			Win32InitDSound(Window, SoundOutput.SecondaryBufferSize, SoundOutput.SamplesPerSecond);
			Win32FillBuffer(&SoundOutput, 0, SoundOutput.SecondaryBufferSize);
			GlobalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);

			GlobalRunning = true;

			LARGE_INTEGER LastCounter;
			QueryPerformanceCounter(&LastCounter);


			int64 LastCycleCount = __rdtsc();

			while (GlobalRunning)
			{
				MSG Message;
				while (PeekMessageA(&Message, 0, 0, 0, PM_REMOVE)) 
				{
					if (Message.message == WM_QUIT) 
					{
						GlobalRunning = false;
					}

					TranslateMessage(&Message);
					DispatchMessage(&Message);
					
				}

				for (DWORD ControllerIndex = 0; ControllerIndex < XUSER_MAX_COUNT; ControllerIndex++)
				{
					XINPUT_STATE ControllerState;
					if (XInputGetState(ControllerIndex, &ControllerState))
					{
						XINPUT_GAMEPAD* Pad = &ControllerState.Gamepad;
						
						bool Up = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
						bool Down = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
						bool Left = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
						bool Right = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);
						bool Start = (Pad->wButtons & XINPUT_GAMEPAD_START);
						bool Back = (Pad->wButtons & XINPUT_GAMEPAD_BACK);
						bool LeftShoulder = (Pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
						bool RightShoulder = (Pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);
						bool AButton = (Pad->wButtons & XINPUT_GAMEPAD_A);
						bool BButton = (Pad->wButtons & XINPUT_GAMEPAD_B);
						bool XButton = (Pad->wButtons & XINPUT_GAMEPAD_X);
						bool YButton = (Pad->wButtons & XINPUT_GAMEPAD_Y);

						if (AButton)
						{
							YOffset++;
						}

						SHORT LeftThumbx = Pad->sThumbLX;
						SHORT LeftThumby = Pad->sThumbLY;

					}
					else
					{
						//The controller is not working
					}
				
				}

				XINPUT_VIBRATION Vibration;
				Vibration.wLeftMotorSpeed = 6000;
				Vibration.wRightMotorSpeed = 6000;

				XInputSetState(0, &Vibration);
				
				win32_window_dimension WindowDimension = Win32GetWindowDimension(Window);

				Win32DisplayBufferInWindow(&GlobalBackBuffer, DeviceContext, WindowDimension.Width, WindowDimension.Height);
				game_offscreen_buffer Buffer = {};
				Buffer.Memory = GlobalBackBuffer.Memory;
				Buffer.Height = GlobalBackBuffer.Height;
				Buffer.Width = GlobalBackBuffer.Width;
				Buffer.Pitch = GlobalBackBuffer.Pitch;
				GameUpdateAndRender(&Buffer);


				DWORD PlayCursor;
				DWORD WriteCursor;
				HRESULT Error = GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor);

				if (SUCCEEDED(Error))
				{
					DWORD ByteToLock = (SoundOutput.RunningSampleIndex * SoundOutput.BytesPerSample) % SoundOutput.SecondaryBufferSize;
					DWORD BytesToWrite;
					if (ByteToLock > PlayCursor)
					{
						BytesToWrite = SoundOutput.SecondaryBufferSize - ByteToLock;
						BytesToWrite += PlayCursor;
					}
					else
					{
						BytesToWrite = PlayCursor - ByteToLock;
					}

					Win32FillBuffer(&SoundOutput, ByteToLock, BytesToWrite);
				}



				XOffset++;

				int64 EndCycleCount = __rdtsc();

				LARGE_INTEGER EndCounter;
				QueryPerformanceCounter(&EndCounter);

				int64 CyclesElapsed = EndCycleCount - LastCycleCount;
				int32 MCPerFrame = (int32)CyclesElapsed / (1000 * 1000);

				int64 CounterElapsed = EndCounter.QuadPart - LastCounter.QuadPart;
				int32 MSPerFrame = (int32)((1000 * CounterElapsed) / PerfCountFrequency);

				int32 FPS = 1000 / MSPerFrame;

				char Bufferf[256];
				wsprintfA(Bufferf, "%dms, %dFPS, %dMCPF\n", MSPerFrame, FPS, MCPerFrame);
				OutputDebugStringA(Bufferf);

				LastCounter = EndCounter;
				LastCycleCount = EndCycleCount;
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
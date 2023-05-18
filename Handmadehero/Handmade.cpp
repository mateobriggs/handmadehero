#include "Handmade.h"

internal void GameOutputSound(game_sound_output_buffer* SoundBuffer)
{
	local_persist real32 tSine;
	int16* SampleOut = SoundBuffer->Samples;
	int16 ToneVolume = 3000;
	int ToneHz = 256;
	int WavePeriod = SoundBuffer->SamplesPerSecond / ToneHz;

	for (int SampleIndex = 0; SampleIndex < SoundBuffer->SampleCount; SampleIndex++)
	{
		real32 SinValue = sinf(tSine);
		int16 SampleValue = (int16)(SinValue * ToneVolume);
		*SampleOut++ = SampleValue;
		*SampleOut++ = SampleValue;

		tSine += 2.0f * Pi32 * 1.0f / (real32)WavePeriod;
	}

}

internal void RenderWeirdGradient(game_offscreen_buffer* Buffer, int BlueOffset, int GreenOffset)
{

	int Width = Buffer->Width;

	uint8* Row = (uint8*)Buffer->Memory;

	for (int Y = 0; Y < Buffer->Height; Y++)
	{
		uint32* Pixel = (uint32*)Row;

		for (int X = 0; X < Buffer->Width; X++)
		{
			uint8 Blue = (X + BlueOffset);
			uint8 Green = (Y + GreenOffset);
			*Pixel++ = ((Green << 8) | Blue); 
		}

		Row += Buffer->Pitch;
	}
}

void GameUpdateAndRender(game_offscreen_buffer* Buffer, int XOffset, int YOffset, game_sound_output_buffer* SoundBuffer)
{
	GameOutputSound(SoundBuffer);
	RenderWeirdGradient(Buffer, XOffset, YOffset);
}

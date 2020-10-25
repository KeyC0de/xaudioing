#include "com_initializer.h"
#include "sound_manager.h"

#pragma comment( lib, "xaudio2_8.lib" )

int main()
{
	COMInitializer comInitializer;

	/*
	// 1st.
	WAVEFORMATEXTENSIBLE waveFmt1 = {0};
	XAUDIO2_BUFFER xaudioBuffer1 = {0};
	BYTE* pDataBuffer1 = nullptr;
	const wchar_t* zsFilename1 = L"assets\\sfx\\ARNOLD_i'll_be_back.wav";
	IXAudio2SourceVoice* pSourceVoice1 = nullptr;
	
	pDataBuffer1 = setupSourceVoice( waveFmt1,
		xaudioBuffer1,
		zsFilename1,
		pXaudio2,
		pSourceVoice1 );
	
	/// 5. Optional - Add Submix Voices to the audio graph
	IXAudio2SubmixVoice* pSubmixVoice;
	pXaudio2->CreateSubmixVoice( &pSubmixVoice,
		1u,
		44100u,
		0u,
		0u,
		nullptr,
		nullptr );

	// create more sounds
	// 2nd.
	XAUDIO2_BUFFER xaudioBuffer2 = {0};
	WAVEFORMATEXTENSIBLE waveFmt2 = {0};
	BYTE* pDataBuffer2 = nullptr;
	const wchar_t* zsFilename2 = L"assets\\sfx\\AmbientCity_TypeB02.wav";
	IXAudio2SourceVoice* pSourceVoice2 = nullptr;
	
	pDataBuffer2 = setupSourceVoice( waveFmt2,
		xaudioBuffer2,
		zsFilename2,
		pXaudio2,
		pSourceVoice2,
		pSubmixVoice );

	// 3nd.
	XAUDIO2_BUFFER xaudioBuffer3 = {0};
	WAVEFORMATEXTENSIBLE waveFmt3 = {0};
	BYTE* pDataBuffer3 = nullptr;
	const wchar_t* zsFilename3 = L"assets\\sfx\\Anthrax - I am the Law.wav";
	IXAudio2SourceVoice* pSourceVoice3 = nullptr;
	
	pDataBuffer3 = setupSourceVoice( waveFmt3,
		xaudioBuffer3,
		zsFilename3,
		pXaudio2,
		pSourceVoice3,
		pSubmixVoice );
	
	// you can apply changes to all voices attached to an output (submix) voice
	//	by adjusting just the submix voice
	pSubmixVoice->SetVolume( 0.1 );
	*/
}
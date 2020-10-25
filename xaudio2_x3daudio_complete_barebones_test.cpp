//#include "sound_manager.h"

#include "winner.h"
#include "assertions.h"
#pragma comment( lib, "xaudio2_8.lib" )
//#define USING_XAUDIO2_8

#include <wrl/client.h>
#include "com_initializer.h"
#include <comdef.h>

#include <xaudio2.h>
//#include <xaudio2fx.h>
//#include <xapofx.h>

#include <x3daudio.h>
#include <DirectXMath.h>
namespace dx = DirectX;

#include <iostream>
#include <string>

namespace mwrl = Microsoft::WRL;


// error printing
std::wstring printHresultErrorDescription( HRESULT hres )
{
	_com_error error( hres );
	return error.ErrorMessage();
}

#define ASSERT_HRES_IF_FAILED( hres ) if ( FAILED ( hres ) )\
	{\
		std::wcout << __LINE__\
			<< L" "\
			<< printHresultErrorDescription( hres )\
			<< std::endl;\
		std::exit( hres );\
	}

static constexpr WORD nChannelsPerSound = 2u;
static constexpr DWORD nSamplesPerSec = 48000u;	// valid: 44100u, 48000u, 96000u
static constexpr WORD nBitsPerSample = 16u;
static constexpr size_t nMaxAudioChannels = 64u;


#ifdef _XBOX	// big endian
#define fourccRIFF	'RIFF'
#define fourccDATA	'data'
#define fourccFMT	'fmt '
#define fourccWAVE	'WAVE'
#define fourccXWMA	'XWMA'
#define fourccDPDS	'dpds'
#else	// little endian
#define fourccRIFF	'FFIR'
#define fourccDATA	'atad'
#define fourccFMT	' tmf'
#define fourccWAVE	'EVAW'
#define fourccXWMA	'AMWX'
#define fourccDPDS	'sdpd'
#endif

// ro find a chunk in a RIFF file
HRESULT findChunk( HANDLE file,
	DWORD fourcc,
	DWORD& chunkSize,
	DWORD& chunkDataPosition )
{
    HRESULT hr = S_OK;
    if ( INVALID_SET_FILE_POINTER == SetFilePointer( file,
		0,
		nullptr,
		FILE_BEGIN ) )
        return HRESULT_FROM_WIN32( GetLastError() );

    DWORD chunkType;
    DWORD chunkDataSize;
    DWORD RIFFDataSize = 0;
    DWORD fileType;
    DWORD bytesRead = 0;
    DWORD fileOffset = 0;

    while ( hr == S_OK )
    {
        DWORD bytesRead;
        if ( 0 == ReadFile( file,
			&chunkType,
			sizeof( DWORD ),
			&bytesRead,
			nullptr ) )
            hr = HRESULT_FROM_WIN32( GetLastError() );

        if ( 0 == ReadFile( file,
			&chunkDataSize,
			sizeof( DWORD ),
			&bytesRead,
			nullptr ) )
            hr = HRESULT_FROM_WIN32( GetLastError() );

        switch ( chunkType )
        {
        case fourccRIFF:
            RIFFDataSize = chunkDataSize;
            chunkDataSize = 4;
            if ( 0 == ReadFile( file,
				&fileType,
				sizeof( DWORD ),
				&bytesRead,
				nullptr ) )
                hr = HRESULT_FROM_WIN32( GetLastError() );
            break;

        default:
            if ( INVALID_SET_FILE_POINTER == SetFilePointer( file,
				chunkDataSize,
				nullptr,
				FILE_CURRENT ) )
            return HRESULT_FROM_WIN32( GetLastError() );            
        }

        fileOffset += sizeof( DWORD ) * 2;

        if ( chunkType == fourcc )
        {
            chunkSize = chunkDataSize;
            chunkDataPosition = fileOffset;
            return S_OK;
        }

        fileOffset += chunkDataSize;

        if ( bytesRead >= RIFFDataSize )
			return S_FALSE;
    }

    return S_OK;
}

//===================================================
//	\function	readChunkData
//	\brief  read chunk's data (after the chunk has been located)
//	\date	2020/10/21 17:37
HRESULT readChunkData( HANDLE file,
	void* buffer,
	DWORD buffersize,
	DWORD bufferoffset )
{
    HRESULT hr = S_OK;
    if ( INVALID_SET_FILE_POINTER == SetFilePointer( file,
		bufferoffset,
		nullptr,
		FILE_BEGIN ) )
        return HRESULT_FROM_WIN32( GetLastError() );
    DWORD bytesRead;
    if ( 0 == ReadFile( file,
		buffer,
		buffersize,
		&bytesRead,
		nullptr ) )
        hr = HRESULT_FROM_WIN32( GetLastError() );
    return hr;
}


//===================================================
//	\function	setupSourceVoice
//	\brief  loads the audio from disk and stores it in the
//			WAVEFORMATEXTENSIBLE format and XAUDIO2_BUFFER structures
//			Optionally sets a submix voice as output as well
//	\date	2020/10/23 15:35
BYTE* setupSourceVoice( WAVEFORMATEXTENSIBLE& waveFmt,
	XAUDIO2_BUFFER& xaudioBuffer,
	const wchar_t* zsFilename,
	mwrl::ComPtr<IXAudio2> pXaudio2,
	IXAudio2SourceVoice*& pSourceVoice,
	IXAudio2SubmixVoice* pSubmixVoice = nullptr,
	UINT32 sourceVoiceCreationFlags = XAUDIO2_VOICE_NOSRC | XAUDIO2_VOICE_NOPITCH )
{
	HANDLE file = CreateFileW( zsFilename,
		GENERIC_READ,
		FILE_SHARE_READ,
		nullptr,
		OPEN_EXISTING,
		0,
		nullptr );
	if ( INVALID_HANDLE_VALUE == file )
		ASSERT_HRES_IF_FAILED( HRESULT_FROM_WIN32( GetLastError() ) );

	HRESULT hres = SetFilePointer( file,
		0,
		nullptr,
		FILE_BEGIN );
	if ( INVALID_SET_FILE_POINTER == hres )
		ASSERT_HRES_IF_FAILED( HRESULT_FROM_WIN32( GetLastError() ) );

	// 1. locate the 'RIFF' chunk in the audio file and check the file type
	DWORD chunkSize;
	DWORD chunkPosition;

	findChunk( file,
		fourccRIFF,
		chunkSize,
		chunkPosition );

	// check file type should be fourccWAVE = 'XWMA'
	DWORD fileType;
	readChunkData( file,
		&fileType,
		sizeof( DWORD ),
		chunkPosition );
	if ( fileType != fourccWAVE )
	{
		std::wcout << L"Unsupported Filetype\n" << fileType << L" discovered:\n";
#ifdef _DEBUG
		__debugbreak();
#endif // _DEBUG
	}

	// 2. locate the 'fmt' chunk and copy its contents into a WAVEFORMATEXTENSIBLE structure
	hres = findChunk( file,
		fourccFMT,
		chunkSize,
		chunkPosition );
	ASSERT_HRES_IF_FAILED( hres );
	
	hres = readChunkData( file,
		&waveFmt,
		chunkSize,
		chunkPosition );
	ASSERT_HRES_IF_FAILED( hres );

	// 3. locate the 'data' of the chunk and copy its contents into a buffer
	hres = findChunk( file,
		fourccDATA,
		chunkSize,
		chunkPosition );
	ASSERT_HRES_IF_FAILED( hres );

	//std::wcout << chunkSize << L'\n';
	
	BYTE* pDataBuffer = new BYTE[chunkSize];
	hres = readChunkData( file,
		pDataBuffer,
		chunkSize,
		chunkPosition );
	ASSERT_HRES_IF_FAILED( hres );

	// 4. populate the XAUDIO2_BUFFER structure
	xaudioBuffer.AudioBytes = chunkSize;	// size of the audio buffer in Bytes
	xaudioBuffer.pAudioData = pDataBuffer;	// buffer containing audio data
	xaudioBuffer.Flags = XAUDIO2_END_OF_STREAM;

	//std::wcout << waveFmt.Format.wFormatTag << L'\n';
	//std::wcout << waveFmt.Format.nChannels << L'\n';
	//std::wcout << waveFmt.Format.nSamplesPerSec << L'\n';
	//std::wcout << waveFmt.Format.wBitsPerSample << L'\n';
	//std::wcout << waveFmt.Format.nBlockAlign << L'\n';
	//std::wcout << waveFmt.Format.cbSize << L'\n';
	//std::wcout << waveFmt.Format.nAvgBytesPerSec << L'\n';

	if ( waveFmt.Format.nSamplesPerSec != nSamplesPerSec )
	{
		sourceVoiceCreationFlags = 0u;
	}

	// 5. optional - specify an output (submix) voice for this source voice
	if ( pSubmixVoice )
	{
		//  create the voice sends structure to specify for the source voice
		XAUDIO2_SEND_DESCRIPTOR outputVoiceSendDesc = { 0, pSubmixVoice };
		XAUDIO2_VOICE_SENDS outputVoiceSends = { 1, &outputVoiceSendDesc };
		// 6. Create the source voice
		hres = pXaudio2->CreateSourceVoice( &pSourceVoice,
			(WAVEFORMATEX*)&waveFmt,
			sourceVoiceCreationFlags,
			XAUDIO2_DEFAULT_FREQ_RATIO,
			nullptr,
			&outputVoiceSends,
			nullptr );
	}
	else {
		// 6. Create the source voice
		hres = pXaudio2->CreateSourceVoice( &pSourceVoice,
			(WAVEFORMATEX*)&waveFmt,
			sourceVoiceCreationFlags,
			XAUDIO2_DEFAULT_FREQ_RATIO,
			nullptr,
			nullptr,
			nullptr );
	}
	ASSERT_HRES_IF_FAILED( hres );
	
	// set steady sample rate
	if ( waveFmt.Format.nSamplesPerSec != nSamplesPerSec )
	{
		waveFmt.Format.nSamplesPerSec = nSamplesPerSec;
		hres = pSourceVoice->SetSourceSampleRate( nSamplesPerSec );
		ASSERT_HRES_IF_FAILED( hres );
	}

	ASSERT( waveFmt.Format.wFormatTag == WAVE_FORMAT_PCM,
		L"Only XPCM technique allowed!" );
	ASSERT( waveFmt.Format.nSamplesPerSec == nSamplesPerSec,
		L"All sounds must be to 48kHz!" );
	ASSERT( waveFmt.Format.wBitsPerSample == nBitsPerSample,
		L"Only 16b/sample allowed!" );
	ASSERT( waveFmt.Format.cbSize == 0, L"No extra Format information allowed" );
	//ASSERT( waveFmt.Format.nChannels == nChannelsPerSound,
	//	L"All sounds must be stereo!" );
	//ASSERT( waveFmt.Format.nBlockAlign == ( nChannelsPerSound * nBitsPerSample ) / 8,
	//	L"Wrong block alignment" );
	//ASSERT( waveFmt.Format.nAvgBytesPerSec == waveFmt.Format.nBlockAlign * nSamplesPerSec,
	//	L"Average bytes per second error" );

	// 6. submit the XAUDIO2_BUFFER to the source voice
	hres = pSourceVoice->SubmitSourceBuffer( &xaudioBuffer, nullptr );
	ASSERT_HRES_IF_FAILED( hres );

	return pDataBuffer;
}


void play( IXAudio2SourceVoice* sourceVoice )
{
	BOOL isPlayingSound = TRUE;
	XAUDIO2_VOICE_STATE soundState = {0};
	HRESULT hres = sourceVoice->Start( 0u );
	while ( SUCCEEDED( hres ) && isPlayingSound )
	{// loop till sound completion
		sourceVoice->GetState( &soundState );
		isPlayingSound = ( soundState.BuffersQueued > 0 ) != 0;
		Sleep( 250 );
	}
	ASSERT_HRES_IF_FAILED( hres );
}

#include <thread>


int main()
{
	/// 1. Initialize COM
	COMInitializer comInitializer;

	/// 2. Create XAudio2 engine
	mwrl::ComPtr<IXAudio2> pXaudio2;
	HRESULT hres;
	hres = XAudio2Create( &pXaudio2,
		0u,
		XAUDIO2_DEFAULT_PROCESSOR );
	ASSERT_HRES_IF_FAILED( hres );

	/// 3. Create mastering voice
	IXAudio2MasteringVoice* pMasterVoice = nullptr;
	hres = pXaudio2->CreateMasteringVoice( &pMasterVoice );
	ASSERT_HRES_IF_FAILED( hres );

	/// 4. Setup audio files
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

	/// 6. Initialize X3DAudio
	DWORD channelMask;
	pMasterVoice->GetChannelMask( &channelMask );

	X3DAUDIO_HANDLE x3dAudioEngine;
	X3DAudioInitialize( channelMask,
		X3DAUDIO_SPEED_OF_SOUND,
		x3dAudioEngine );

	/// 7. Create listeners and emitters
	X3DAUDIO_LISTENER listener;
	ZeroMemory( &listener, sizeof( X3DAUDIO_LISTENER ) );
	X3DAUDIO_EMITTER emitter = {0};
	emitter.ChannelCount = 1;
	emitter.CurveDistanceScaler = FLT_MIN;

	// set their positions in 3d space
	listener.Position = { 0,0,0 };
	emitter.Position = { 0,0,10 };

	/// 8. create dsp settings structure
	XAUDIO2_VOICE_DETAILS audioDetails = {0};
	pMasterVoice->GetVoiceDetails( &audioDetails );

	X3DAUDIO_DSP_SETTINGS dspSettings = {0};
	float* dspMatrix = new float[sizeof( audioDetails.InputChannels )];
	dspSettings.SrcChannelCount = 1u;
	dspSettings.DstChannelCount = audioDetails.InputChannels;
	dspSettings.pMatrixCoefficients = dspMatrix;

	//////////////////////////////////////////////////////////////////////
	/// 9. Perform the x3daudio steps below in your game loop
	// a. update listeners and emitters position, velocity and orientation
	X3DAUDIO_VECTOR emitterOrientFront = { 0,0,0 }, listenerOrientFront = { 0,0,0 };
	dx::XMFLOAT3 emitterOrientTop = { 0,0,0 }, listenerOrientTop = { 0,0,0 };
	dx::XMFLOAT3 emitterPosition = { 0,0,0 }, listenerPosition = { 0,0,0 };
	dx::XMFLOAT3 emitterVelocity = { 0,0,0 }, listenerVelocity = { 0,0,0 };
	emitter.OrientFront = emitterOrientFront;
	emitter.OrientTop = emitterOrientTop;
	emitter.Position = emitterPosition;
	emitter.Velocity = emitterVelocity;
	listener.OrientFront = listenerOrientFront;
	listener.OrientTop = listenerOrientTop;
	listener.Position = listenerPosition;
	listener.Velocity = listenerVelocity;

	// b. calculate new dsp settings for the voices
	X3DAudioCalculate( x3dAudioEngine,
		&listener,
		&emitter,
		X3DAUDIO_CALCULATE_MATRIX | X3DAUDIO_CALCULATE_DOPPLER
		| X3DAUDIO_CALCULATE_LPF_DIRECT | X3DAUDIO_CALCULATE_REVERB,
		&dspSettings );

	// c. optional - apply the volume and pitch values to the source voices
	pSourceVoice2->SetOutputMatrix( pMasterVoice,
		1u,
		audioDetails.InputChannels,
		dspSettings.pMatrixCoefficients,
		0u );
	pSourceVoice2->SetFrequencyRatio( dspSettings.DopplerFactor );

	// d. optional - apply the calculated reverb level to the submix voice
	pSourceVoice2->SetOutputMatrix( pSubmixVoice,
		1u,
		1u,
		&dspSettings.ReverbLevel );
	
	// e. optional - apply the calculated low pass filter to the source voice
	XAUDIO2_FILTER_PARAMETERS filterParameters = {
		XAUDIO2_FILTER_TYPE::LowPassFilter,
		2.0f * sinf( X3DAUDIO_PI / 6.0f * dspSettings.LPFDirectCoefficient),
		1.0f
	};
	pSourceVoice2->SetFilterParameters( &filterParameters );
	//////////////////////////////////////////////////////////////////////

	/// 10. play sounds
	std::thread thr1{ play, pSourceVoice1 };
	std::thread thr2{ play, pSourceVoice2 };
	//std::thread thr3{ play, pSourceVoice3 };

	thr1.join();
	thr2.join();
	//thr3.join();

	/// 11. cleanup
	delete[] pDataBuffer1;
	delete[] pDataBuffer2;
	delete[] pDataBuffer3;
	delete[] dspMatrix;

	pSourceVoice1->FlushSourceBuffers();
	pSourceVoice2->FlushSourceBuffers();
	pSourceVoice3->FlushSourceBuffers();
	pSourceVoice1->DestroyVoice();
	pSourceVoice2->DestroyVoice();
	pSourceVoice3->DestroyVoice();
	
	//char* ca = new char[1024];
}

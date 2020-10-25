//#include "sound_manager.h"

#include "winner.h"
#pragma comment( lib, "xaudio2_8.lib" )
//#define USING_XAUDIO2_8

#include "com_initializer.h"
#include <wrl/client.h>
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

/**===================================================
//	\function	readChunkData
//	\brief  read chunk's data (after the chunk has been located)
//	\date	2020/10/21 17:37
*/
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


/**===================================================
//	\function	setupSourceVoice
//	\brief  loads the audio from disk and stores it in the
//			WAVEFORMATEXTENSIBLE format and XAUDIO2_BUFFER structures
//			Optionally sets a submix voice as output as well
//	\date	2020/10/23 15:35
*/
BYTE* setupSourceVoice( WAVEFORMATEXTENSIBLE& waveFmt,
	XAUDIO2_BUFFER& xaudioBuffer,
	const wchar_t* zsFilename,
	mwrl::ComPtr<IXAudio2> pXaudio2,
	IXAudio2SourceVoice*& pSourceVoice,
	IXAudio2SubmixVoice* pSubmixVoice = nullptr )
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

	// 5. optional - specify an output (submix) voice for this source voice
	if ( pSubmixVoice )
	{
		//  create the voice sends structure to specify for the source voice
		XAUDIO2_SEND_DESCRIPTOR outputVoiceSendDesc = { 0, pSubmixVoice };
		XAUDIO2_VOICE_SENDS outputVoiceSends = { 1, &outputVoiceSendDesc };
		// 6. Create the source voice
		hres = pXaudio2->CreateSourceVoice( &pSourceVoice,
			(WAVEFORMATEX*)&waveFmt,
			0u,
			XAUDIO2_DEFAULT_FREQ_RATIO,
			nullptr,
			&outputVoiceSends,
			nullptr );
	}
	else {
		// 6. Create the source voice
		hres = pXaudio2->CreateSourceVoice( &pSourceVoice,
			(WAVEFORMATEX*)&waveFmt,
			0u,
			XAUDIO2_DEFAULT_FREQ_RATIO,
			nullptr,
			nullptr,
			nullptr );
	}
	ASSERT_HRES_IF_FAILED( hres );

	// 6. submit the XAUDIO2_BUFFER to the source voice
	hres = pSourceVoice->SubmitSourceBuffer( &xaudioBuffer );
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
		Sleep( 100 );
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

	// create another sound
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
	
	// you can now apply changes to all sound effect voices just by adjusting the
	// output (submix) voice
	pSubmixVoice->SetVolume( 0.1 );

	/// 8. play sounds
	std::thread thr1{ play, pSourceVoice1 };
	std::thread thr2{ play, pSourceVoice2 };

	thr1.join();
	thr2.join();

	delete[] pDataBuffer1;
	delete[] pDataBuffer2;
	
	//char* ca = new char[1024];
}

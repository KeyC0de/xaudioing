#include "winner.h"
#include <xaudio2.h>
//#include <xaudio2fx.h>
//#include <xapofx.h>
#include <x3daudio.h>

#include "sound_manager.h"
#include "assertions.h"

#include <algorithm>
#include "util.h"

#ifdef _DEBUG
#	include <iostream>
#endif // _DEBUG

namespace mwrl = Microsoft::WRL;


SoundManager::Channel::Channel()
{
}

SoundManager::Channel::~Channel() noexcept
{
	ASSERT( m_pSound, L"Null sound!" );
	ASSERT( m_pSourceVoice, L"Null voice!" );
	m_pSourceVoice->DestroyVoice();
	m_pSourceVoice = nullptr;
}

void SoundManager::Channel::setupChannel( SoundManager& soundManager,
	Sound& sound)
{
	class VoiceCallback : public IXAudio2VoiceCallback
	{
	public:
		// Called when the voice is about to start processing a new audio buffer.
		void STDMETHODCALLTYPE OnBufferStart( void* pBufferContext ) override
		{
			pass
		}
		// Called when the voice finishes processing a buffer.
		void STDMETHODCALLTYPE OnBufferEnd( void* pBufferContext ) override
		{
			Channel& channel = *reinterpret_cast<Channel*>( pBufferContext );
			channel.stopSound();
			{
				std::lock_guard<std::mutex> lg{ channel.m_pSound->m_mu };
				removeByBackSwap( channel.m_pSound->m_busyChannels, channel );
				channel.m_pSound->m_busyChannels.erase(std::find(
					channel.m_pSound->m_busyChannels.begin(),channel.m_pSound->m_busyChannels.end(), &channel));
				// notify any thread that might be waiting for activeChannels
				// to become zero (i.e. thread calling destructor)
				channel.m_pSound->m_condVar.notify_all();
			}
			channel.m_pSound = nullptr;
			SoundSystem::getInstance().deactivateChannel( channel );
		}
		// Called when the voice reaches the end position of a loop.
		void STDMETHODCALLTYPE OnLoopEnd( void* pBufferContext ) override
		{
			pass
		}
		// Called when the voice has just finished playing a contiguous audio stream.
		void STDMETHODCALLTYPE OnStreamEnd() override
		{
			pass
		}
		// Called when a critical error occurs during voice processing.
		void STDMETHODCALLTYPE OnVoiceError( void* pBufferContext, HRESULT Error )
			override
		{
			pass
		}
		// Called just after the processing pass for the voice ends.
		void STDMETHODCALLTYPE OnVoiceProcessingPassEnd() override
		{
			pass
		}
		// Called during each processing pass for each voice, just 
		void STDMETHODCALLTYPE OnVoiceProcessingPassStart( UINT32 bytesRequired ) override
		{
			pass
		}
	};

	static VoiceCallback voiceCallback;
	ZeroMemory( &voiceCallback, sizeof( VoiceCallback ) );
	HRESULT hres;

	sound.m_pXaudioBuffer->pContext = this;

	// 5. optional - specify an output (submix) voice for this source voice
	UINT32 sourceVoiceCreationFlags = 0u;
	//if ( pSubmixVoice )
	//{
	//	//  create the voice sends structure to specify for the source voice
	//	XAUDIO2_SEND_DESCRIPTOR outputVoiceSendDesc = { 0, pSubmixVoice };
	//	XAUDIO2_VOICE_SENDS outputVoiceSends = { 1, &outputVoiceSendDesc };
	//	// 6. Create the source voice
	//	hres = pXaudio2->CreateSourceVoice( &pSourceVoice,
	//		(WAVEFORMATEX*)&waveFmt,
	//		sourceVoiceCreationFlags,
	//		XAUDIO2_DEFAULT_FREQ_RATIO,
	//		nullptr,
	//		&outputVoiceSends,
	//		nullptr );
	//}
	//else {
		// 6. Create the source voice
		hres = soundManager.m_pXAudio2->CreateSourceVoice( &m_pSourceVoice,
			(WAVEFORMATEX*)&m_pSound->m_pWaveFormat,
			sourceVoiceCreationFlags,
			XAUDIO2_DEFAULT_FREQ_RATIO,
			&voiceCallback,
			nullptr,
			nullptr );
	//}
	ASSERT_HRES_IF_FAILED;

	// set steady sample rate
	if ( m_pSound->m_pWaveFormat->Format.nSamplesPerSec 
		!= sound_wave_properties::nSamplesPerSec )
	{
		m_pSound->m_pWaveFormat->Format.nSamplesPerSec 
			= sound_wave_properties::nSamplesPerSec;
		hres = m_pSourceVoice->SetSourceSampleRate(
			sound_wave_properties::nSamplesPerSec );
		ASSERT_HRES_IF_FAILED;
	}

	ASSERT( m_pSound->m_pWaveFormat->Format.wFormatTag == WAVE_FORMAT_PCM,
				L"Only XPCM technique allowed!" );
	ASSERT( m_pSound->m_pWaveFormat->Format.wBitsPerSample
			== sound_wave_properties::nBitsPerSample,
				L"Wrong bits per sample!" );
	ASSERT( m_pSound->m_pWaveFormat->Format.nChannels
			== sound_wave_properties::nChannelsPerSound,
				L"Wrong amount of channels per sound!" );
	ASSERT( m_pSound->m_pWaveFormat->Format.nSamplesPerSec
			== sound_wave_properties::nSamplesPerSec,
				L"Wrong number of samples per second!" );
	ASSERT( m_pSound->m_pWaveFormat->Format.cbSize == 0,
				L"No extra Format information allowed" );

	// 6. submit the XAUDIO2_BUFFER to the source voice
	hres = m_pSourceVoice->SubmitSourceBuffer( m_pSound->m_pXaudioBuffer.get() );
	ASSERT_HRES_IF_FAILED( hres );
}

void SoundManager::Channel::playSound( Sound* sound,
	float volume,
	float freqRatio )
{
	ASSERT( m_pSound, L"Null Sound!" );
	ASSERT( m_pSourceVoice, L"Null Voice!" );
	std::unique_lock ul{ sound->m_mu };
	sound->m_busyChannels.emplace_back( this );

	HRESULT hres = m_pSourceVoice->SetVolume( volume );
	ASSERT_HRES_IF_FAILED;
	hres = m_pSourceVoice->SetSourceSampleRate( freqRatio );
	ASSERT_HRES_IF_FAILED;
	hres = m_pSourceVoice->Start();
	ASSERT_HRES_IF_FAILED;
}

void SoundManager::Channel::stopSound()
{
	ASSERT( m_pSound, L"Sound was not initialized!" );
	ASSERT( m_pSourceVoice, L"Voice was not set!" );
	m_pSourceVoice->Stop();
	m_pSourceVoice->FlushSourceBuffers();
}

bool SoundManager::Channel::rechannel( const Sound* pOldSound, Sound* pNewSound )
{
	ASSERT( pOldSound == pNewSound, L"Channel mismatch!" );
	m_pSound.reset( pNewSound );
}

Sound* SoundManager::Channel::getSound() const
{
	ASSERT( m_pSound, L"Sound is good to go!" );
	return m_pSound.get();
}

SoundManager& SoundManager::getInstance( WAVEFORMATEXTENSIBLE* format )
{
	static SoundManager soundManager{ format };
	return soundManager;
}

SoundManager::~SoundManager() noexcept
{
	{
		std::unique_lock ul{ m_mu };
		m_occupiedChannels.clear();
		m_idleChannels.clear();
	}
	m_pMasterVoice->DestroyVoice();
	m_pMasterVoice = nullptr;
}

void SoundManager::setMasterVolume( float volume )
{
	m_pMasterVoice->SetVolume( volume );
}

void SoundManager::playChannelSound( Sound* sound,
	float volume,
	float freqRatio )
{
	std::unique_lock ul{ m_mu };
	if ( !m_idleChannels.empty() )
	{
		for ( int i = 0; i < m_idleChannels.size(); ++i )
		{
			// go through channels to find the one we want to play
			auto& channel = m_idleChannels[i];
			if ( channel->getSound() == sound )
			{
				channel->setupChannel( *this, *sound );
				m_occupiedChannels.emplace_back( std::move( channel ) );
				removeByBackSwap( m_idleChannels, i );
				m_occupiedChannels.back()->playSound( sound,
					volume,
					freqRatio );
				break;
			}
		}
	}
}

bool SoundManager::disableChannel( Channel& channel )
{
	auto ref = std::find_if( m_occupiedChannels.begin(),
		m_occupiedChannels.end(),
		[&channel] ( const std::unique_ptr<Channel>& cha ) {
			&channel == cha.get();
		} );
	if ( &ref == nullptr )
		return false;

	return true;
}

SoundManager::SoundManager( WAVEFORMATEXTENSIBLE* format )
{
	// keep a pointer to the format
	m_pFormat = format;

	HRESULT hres;
	hres = XAudio2Create( &m_pXAudio2,
		0u,
		XAUDIO2_DEFAULT_PROCESSOR );
	ASSERT_HRES_IF_FAILED;

	hres = m_pXAudio2->CreateMasteringVoice( &m_pMasterVoice );
	ASSERT_HRES_IF_FAILED;

	// create the channels
	m_idleChannels.reserve( nMaxAudioChannels );
	for ( size_t i = 0; i < nMaxAudioChannels; ++i )
	{
		m_idleChannels.emplace_back( std::make_unique<Channel>() );
	}
}

#pragma region LibrarySoundReading

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

HRESULT Sound::findChunk( HANDLE file,
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

HRESULT Sound::readChunkData( HANDLE file,
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

#pragma endregion

Sound::Sound( const wchar_t* zsFilename,
	const std::wstring& defaultName,
	const std::wstring& defaultSubmixName )
	:
	// Initialize wave format and audio buffer
	m_pWaveFormat{ std::make_unique<WAVEFORMATEXTENSIBLE>() },
	m_pXaudioBuffer{ std::make_unique<XAUDIO2_BUFFER>() }
{
	HANDLE file = CreateFileW( zsFilename,
		GENERIC_READ,
		FILE_SHARE_READ,
		nullptr,
		OPEN_EXISTING,
		0,
		nullptr );
	if ( INVALID_HANDLE_VALUE == file )
	{
		ASSERT_HRES_IF_FAILED_( HRESULT_FROM_WIN32( GetLastError() ) );
	}

	HRESULT hres = SetFilePointer( file,
		0,
		nullptr,
		FILE_BEGIN );
	if ( INVALID_SET_FILE_POINTER == hres )
	{
		ASSERT_HRES_IF_FAILED( HRESULT_FROM_WIN32( GetLastError() ) );
	}

	// 1. locate the 'RIFF' chunk in the audio file and check the file type
	DWORD chunkSize;
	DWORD chunkPosition;

	hres = findChunk( file,
		fourccRIFF,
		chunkSize,
		chunkPosition );
	ASSERT_HRES_IF_FAILED;

	// check file type should be fourccWAVE = 'XWMA'
	DWORD fileType;
	hres = readChunkData( file,
		&fileType,
		sizeof( DWORD ),
		chunkPosition );
	ASSERT_HRES_IF_FAILED;
	if ( fileType != fourccWAVE )
	{
		std::wcout << L"Unsupported Filetype\n" << fileType << L" discovered:\n";
#ifdef _DEBUG	// experiment
		__debugbreak();
#endif // _DEBUG
	}

	// 2. locate the 'fmt' chunk and copy its contents into a WAVEFORMATEXTENSIBLE structure
	hres = findChunk( file,
		fourccFMT,
		chunkSize,
		chunkPosition );
	ASSERT_HRES_IF_FAILED;
	
	hres = readChunkData( file,
		&m_pWaveFormat,
		chunkSize,
		chunkPosition );
	ASSERT_HRES_IF_FAILED;

	// 3. locate the 'data' of the chunk and copy its contents into a buffer
	hres = findChunk( file,
		fourccDATA,
		chunkSize,
		chunkPosition );
	ASSERT_HRES_IF_FAILED;

	//std::wcout << chunkSize << L'\n';
	m_audioData = std::make_unique<BYTE[]>( chunkSize );
	hres = readChunkData( file,
		&m_audioData,
		chunkSize,
		chunkPosition );
	ASSERT_HRES_IF_FAILED;

	// 4. populate the XAUDIO2_BUFFER structure
	m_pXaudioBuffer->AudioBytes = chunkSize;	// size of the audio buffer in Bytes
	m_pXaudioBuffer->pAudioData = m_audioData.get();	// buffer containing audio data
	m_pXaudioBuffer->Flags = XAUDIO2_END_OF_STREAM;
}

Sound::Sound( Sound&& rhs )
	:
	m_name{ std::move( rhs.m_name ) }
{
	std::unique_lock ul{ rhs.m_mu };
	m_audioData = std::move( rhs.m_audioData );
	m_busyChannels = std::move( rhs.m_busyChannels );
	for ( auto& channel : m_busyChannels )
	{
		channel->rechannel( &rhs, this );
	}
	rhs.m_condVar.notify_all();
}

Sound& Sound::operator=( Sound&& rhs )
{
	if ( this != &rhs )
	{
		std::swap( *this, rhs );
	}
	return *this;
}

Sound::~Sound() noexcept
{
	if ( !m_busyChannels.empty() )
	{
		std::lock_guard lg{ m_mu };
		for ( const auto& i : m_busyChannels )
		{
			i->stopSound();
		}
		// wait for those channels to finish playing
		m_condVar.wait( m_mu,
			[this] () { return m_busyChannels.size() == 0; } );
	}
}

std::wstring Sound::getName() const noexcept
{
	return m_name;
}

std::wstring Sound::getTypeName() const noexcept
{
	return std::wstring();
}

void Sound::play( float volume = 1.0f,
	float freqRatio = 1.0f )
{
	SoundManager::getInstance( m_pWaveFormat.get() )
		.playChannelSound( this,
			volume,
			freqRatio );
}

void Sound::stop()
{
}

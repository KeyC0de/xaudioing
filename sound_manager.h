#pragma once

#include <memory>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <string>

#include <wrl\client.h>

#define cond_noex noexcept( !_IS_DEBUG )

namespace sound_wave_properties
{
	// wav properties - all sounds must have the same format
	static constexpr WORD nChannelsPerSound = 2u;
	static constexpr DWORD nSamplesPerSec = 48000u;	// valid: 44100u, 48000u, 96000u
	static constexpr WORD nBitsPerSample = 16u;
}

//============================================================
//	\class	SoundManager
//
//	\author	KeyC0de
//	\date	2020/10/23 21:33
//
//	\brief	singleton class
//			back-end
//			stores the Sounds in vector of Channel<Sound>>s
//			encapsulates a Mastering voice which works with a single wave format
//			thus all Sounds contained in a SoundManager object must have the same format
//=============================================================
class SoundManager final
{
public:
	//============================================================
	//	\class	Channel
	//
	//	\author	KeyC0de
	//	\date	2020/10/25 14:01
	//
	//	\brief	back-end
	//			each Sound sticks to a single Channel
	//			at most nMaxAudioChannels can play at a certain time
	//=============================================================
	class Channel final
	{
		friend class Sound;
	public:
		Channel();
		Channel( const Channel& rhs ) = delete;
		Channel& operator=( const Channel& rhs ) = delete;
		~Channel() noexcept;
		
		void setupChannel( SoundManager& soundManager, class Sound& sound );
		void playSound( class Sound* sound, float volume, float freqRatio );
		void stopSound();
		//===================================================
		//	\function	rechannel
		//	\brief  finds new channel for the existing Sound
		//	\date	2020/10/25 19:18
		bool rechannel( const class Sound* pOldSound, class Sound* pNewSound );
		class Sound* getSound() const cond_noex;
	private:
		class IXAudio2SourceVoice* m_pSourceVoice = nullptr;
		class std::unique_ptr<class Sound> m_pSound;
	};

	//class SubmixType final
	//{
	//public:
	//	SubmixType();
	//	~SubmixType() noexcept;
	//
	//	std::wstring getName() const cond_noex;
	//private:
	//	std::wstring name;
	//	class IXAudio2SubmixVoice* m_pSubmixVoice = nullptr;
	//	//  create the voice sends structure to specify for the source voice
	//	XAUDIO2_SEND_DESCRIPTOR m_outputVoiceSendDesc;
	//	XAUDIO2_VOICE_SENDS m_outputVoiceSends;
	//};
public:
	//===================================================
	//	\function	getInstance
	//	\brief  return the single instance of the class
	//	\date	2020/10/24 1:48
	static SoundManager& getInstance( WAVEFORMATEXTENSIBLE* format );
public:
	SoundManager( const SoundManager& rhs ) = delete;
	SoundManager& operator=( const SoundManager& rhs ) = delete;
	//SoundManager( SoundManager&& rhs );
	//SoundManager& operator=( SoundManager&& rhs );
	~SoundManager() noexcept;

	void setMasterVolume( float volume );
	//void setSubmixVolume( float volume, const SubmixType& submix );
	void playChannelSound( class Sound* sound, float volume, float freqRatio );
	//===================================================
	//	\function	disableChannel
	//	\brief  places specified channel in the list of idle ones
	//				and removes it from the list of active ones
	//			returns bool whether if found the channel in the occupiedChannels list
	//	\date	2020/10/25 19:45
	bool disableChannel( Channel& channel );
	//void disableSubmixVoice( const SubmixType& submix );
private:
	SoundManager( WAVEFORMATEXTENSIBLE* format );
private:
	WAVEFORMATEXTENSIBLE* m_pFormat;

	Microsoft::WRL::ComPtr<class IXAudio2> m_pXAudio2;
	class IXAudio2MasteringVoice* m_pMasterVoice = nullptr;
	std::mutex m_mu;
	std::vector<std::unique_ptr<Channel>> m_occupiedChannels;
	std::vector<std::unique_ptr<Channel>> m_idleChannels;
	//std::vector<std::unique_ptr<SubmixType>> m_submixTypes;

	static constexpr size_t nMaxAudioChannels = 64u;
};


//============================================================
//	\class	Sound
//
//	\author	KeyC0de
//	\date	2020/10/24 1:51
//
//	\brief	move only
//			front-end
//			encapsulates a sound
//			ctor creates the sound properties
//			::play() instructs the sound manager to play the sound on free channel(s)
//=============================================================
class Sound final
{
	friend class SoundManager::Channel;
public:
	//===================================================
	//	\function	findChunk
	//	\brief  locates chunks in RIFF files
	//	\date	2020/10/25 15:09
	static HRESULT findChunk( HANDLE file, DWORD fourcc, DWORD& chunkSize,
		DWORD& chunkDataPosition );
	//===================================================
	//	\function	readChunkData
	//	\brief  read chunk's data (after the chunk has been located)
	//	\date	2020/10/21 17:37
	static HRESULT readChunkData( HANDLE file, void* buffer, DWORD buffersize,
		DWORD bufferoffset );
public:
	// TODO: Sound Looping
	//===================================================
	//	\function	Sound
	//	\brief  constructor loads sound file and configures all its properties
	//	\date	2020/10/25 15:04
	Sound( const wchar_t* zsFilename, const std::wstring& defaultName = L"",
		const std::wstring& defaultSubmixName = L"" );
	Sound( const Sound& rhs ) = delete;
	Sound& operator=( const Sound& rhs ) = delete;

	Sound( Sound&& rhs ) cond_noex;
	Sound& operator=( Sound&& rhs ) cond_noex;
	~Sound() noexcept;

	std::wstring getName() const cond_noex;
	//===================================================
	//	\function	getTypeName
	//	\brief  get sound type eg effects, music, dialogue etc
	//			each sound type corresponds to a Submix voice
	//	\date	2020/10/25 14:05
	std::wstring getTypeName() const cond_noex;
	void play( float volume = 1.0f, float freqRatio = 1.0f );
	void stop();
private:
	std::wstring m_name;
	std::unique_ptr<BYTE[]> m_audioData;
	std::unique_ptr<WAVEFORMATEXTENSIBLE> m_pWaveFormat;
	std::unique_ptr<XAUDIO2_BUFFER> m_pXaudioBuffer;
	std::mutex m_mu;
	std::condition_variable m_condVar;
	std::vector<SoundManager::Channel*> m_busyChannels;	// those are currently playing
};

/*
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
	ASSERT_HRES_IF_FAILED;
}*/
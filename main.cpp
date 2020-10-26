#pragma comment( lib, "xaudio2_8.lib" )

#include "com_initializer.h"
#include "sound_manager.h"


int main()
{
	COMInitializer comInitializer;
	// 1.
	const wchar_t* zsFilename1 = L"assets\\sfx\\ARNOLD_i'll_be_back.wav";
	const wchar_t* zsFilename2 = L"assets\\sfx\\AmbientCity_TypeB02.wav";
	const wchar_t* zsFilename3 = L"assets\\sfx\\Anthrax - I am the Law.wav";
	Sound arnie{ zsFilename1, L"Arnie" };

	arnie.play();
	
	// 2.
	Sound ambience{ zsFilename2, L"City Ambience" };
	//
	// 3.
	Sound anthraxImTheLaw{ zsFilename3, L"Anthrax - I am the Law" };
	anthraxImTheLaw.play();

	ambience.play(.5f);
	//ambience.stop();
	//ambience.play(.25f);

	std::system( "pause" );

	return 0;
}

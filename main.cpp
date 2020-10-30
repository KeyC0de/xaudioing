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
	Sound arnie{ zsFilename1, L"Arnie", L"sfx" };

	arnie.play();
	
	// 2.
	Sound ambience{ zsFilename2, L"City Ambience", L"sfx" };
	ambience.play( .1f );
	// 3.
	Sound anthraxImTheLaw{ zsFilename3, L"Anthrax - I am the Law", L"music" };
	anthraxImTheLaw.play();
	
	ambience.stop();
	// playing after stop doesn't work though:
	//ambience.play(.25f);


	// 4,5
	// bring it to its knees!
	Sound arnieAgain{ zsFilename1, L"ArnieAgainWtf?", L"sfx2" };
	arnieAgain.play( 2.0f );
	Sound arnieThrice{ zsFilename1, L"ArnieThrice?" };
	arnieThrice.play( .4f );

	std::system( "pause" );

	return 0;
}

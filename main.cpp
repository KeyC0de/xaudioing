#include "com_initializer.h"
#include "sound_manager.h"


int main()
{
	COMInitializer comInitializer;
	//using std::string_literals;
	// 1.
	const char* zsFilename1 = "assets\\sfx\\ARNOLD_i'll_be_back.wav";
	const char* zsFilename2 = "assets\\sfx\\AmbientCity_TypeB02.wav";
	const char* zsFilename3 = "assets\\sfx\\CARTOON_BOING_3.wav";
	Sound arnie{ zsFilename1, "Arnie", "sfx" };

	arnie.play();
	
	// 2.
	Sound ambience{ zsFilename2, "City Ambience", "sfx" };
	ambience.play( .1f );
	// 3.
	Sound anthraxImTheLaw{ zsFilename3, "CARTOON_BOING_3", "music" };
	anthraxImTheLaw.play();
	
	ambience.stop();
	// playing after stop doesn't work though:
	//ambience.play(.25f);


	// 4,5
	// bring it to its knees!
	Sound arnieAgain{ zsFilename1, "ArnieAgainWtf?", "sfx2" };
	arnieAgain.play( 2.0f );
	Sound arnieThrice{ zsFilename1, "ArnieThrice?" };
	arnieThrice.play( .4f );

	std::system( "pause" );

	return 0;
}

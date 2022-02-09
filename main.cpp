#include "com_initializer.h"
#include "sound_manager.h"


int main()
{
	COMInitializer comInitializer;
	//using std::string_literals;
	// 1.
	const char* zsFilename1 = "assets/sfx/ARNOLD_i'll_be_back.wav";
	const char* zsFilename2 = "assets/sfx/AmbientCity_TypeB02.wav";
	const char* zsFilename3 = "assets/sfx/CARTOON_BOING_3.wav";
	const char* zsFilename4 = "assets/sfx/arkanoid_brick.wav";
	const char* zsFilename5 = "assets/sfx/arkanoid_pad.wav";
	Sound arnie{zsFilename1, "Arnie", "sfx"};

	arnie.play();
	
	// 2.
	Sound ambience{zsFilename2, "City Ambience", "sfx"};
	ambience.play( .5f );
	// 3.
	Sound anthraxImTheLaw{zsFilename3, "CARTOON_BOING_3", "music"};
	//anthraxImTheLaw.play();
	
	// playing after stop doesn't work though:
	Sleep( 2000 );
	ambience.stop();
	Sleep( 5000 );
	ambience.play(.25f);


	// 4, 5
	// bring it to its knees!
	Sound arnieAgain{zsFilename1, "ArnieAgainWtf?", "sfx2"};
	arnieAgain.play( 2.0f );
	Sound arnieThrice{zsFilename1, "ArnieThrice?"};
	arnieThrice.play( .4f );

	Sleep( 3000 );
	// 6, 7
	Sound brick{zsFilename4, "Brick"};
	brick.play();
	Sleep( 500 );
	brick.stop();
	brick.play();
	Sleep( 1000 );
	Sound pad{zsFilename5, "Pad"};
	pad.play();

#if defined _DEBUG && !defined NDEBUG
	while ( !getchar() );
#endif
	return EXIT_SUCCESS;
}
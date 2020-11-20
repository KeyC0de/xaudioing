<h1 align="center">
	<a href="https://github.com/KeyC0de/MathHandbook_Latex">XAudioing</a>
</h1>
<hr>

A high performance minimal XAudio 2.8 based library. It is a low level component as is the nature of XAudio2. I made it to use it in a Direct3D game engine but it doesn't require Direct3d and can be used for whatever purpose on Windows. XAudio2 delegates sounds to the WASAPI backend, you don't need multliple threads for each playing sound; these are managed by WASAPI which mixes them appropriately.

I used:

* Windows 8.1 x86_64, Microsoft Visual Studio 2017
* XAudio v2.8
* X3DAudio

If you're on Windows 10 you can use XAudio v2.9 without much, if any, change.


# Usage

`SoundManager` is the singleton audio instance.

There is a maximum number of sound channels, which I have set to 64. But make sure that your hardware sound card can also support this. Note that I use UNICODE UTF-16.

You can find examples in main.cpp of how to create and play sounds.

1. Create sound object: `Sound mySound{filePath, soundName, soundCategory = L""};`
2. Play it: `mySound.play( volume = 1.0f );`
3. Stop it: `mySound.stop()`

You can optionally provide a sound category (known as submix voice) as the third argument in the constructor, for example "sfx", "music", "ambience" etc. You can have up to 8 submixes (experimental - it should be possible to use more).

Only .wav sounds are supported. I do not intend at the moment of supporting other audio formats. Note that your .wav file should have 2 or more channels (Stereo) with 16 bits per sample. It is recommended that the samples per second be 48KHz. If it isn't the audio might accelerate or decelerate - you won't like it.


# Known bugs

After you stop a sound, I can't play it again. I'll have to look more into why this is happening.


# Future

Implement:

* Sound looping
* Sound pausing & continuing
* Support more sample rates and configurations


# Contribute

Please submit any bugs you find through GitHub repository 'Issues' page with details describing how to replicate the problem. If you liked it or you learned something new give it a star, clone it, contribute to it whatever. Enjoy.


# License

Distributed under the GNU GPL V3 License. See "GNU GPL license.txt" for more information.


# Contact

email: *nik.lazkey@gmail.com*
website: *www.keyc0de.net*


# Acknowledgements

All demo sounds are in the public domain from https://freesound.org/

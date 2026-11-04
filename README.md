Gish Vita: ReVibed
====

Gish is a side-scrolling platformer video game with some physics puzzle elements developed by Cryptic Sea (pseudonym of Alex Austin), Edmund McMillen, Josiah Pisciotta and published by Chronic Logic in 2004. The game was featured in the first Humble Indie Bundle in May 2010. Following the success of the Humble Bundle promotion, Cryptic Sea pledged to go open source with the game which eventually happened under the GPLv2 on May 29, 2010. - [Wikipedia](https://en.wikipedia.org/wiki/Gish_(video_game)).

This is a fork of [isage's port of Gish to PS Vita](https://github.com/isage/Gish-vita), with the purpose of polishing it further into a more playable experience using AI. The details on what was done so far are mentioned in [Releases](https://github.com/AGTS10k/Gish-vita-revibed/releases) for each release.

The port uses SDL2, PVR_PSP2, OpenAL, and Ogg Vorbis libraries.

**_Full disclaimer_**: all changes made here **were done by AI** (Qwen3.8-27B local model running on an RTX 3090 in my personal PC, then DeepSeek V4.1 Flash Free courtesy of OpenCode). While each change was directed and reviewed by me, I'm not able to code in C myself - only understand what the model did, to some extent. I tested the game after each change, but haven't completed it fully. If you encounter any bugs or other issues, please report it, and I will try to fix or workaround it.

## Download

You can download an installable VPK from the [Releases](https://github.com/AGTS10k/Gish-vita-revibed/releases) section. This VPK will install over the original port.
You need data files from Gish 1.6 (non-steam, MS Windows), Gish 1.53 (non-steam, Linux) or Gish 1.7 (steam version).
Steam 1.7 can be obtained by selecting `version170` in "betas".

Copy data to `ux0:/data/gish/`

## Screenshots
<img width="960" height="544" alt="image" src="https://github.com/user-attachments/assets/b7f9051c-8672-4e66-ab92-4b27e4a9b29f" /><img width="960" height="544" alt="image" src="https://github.com/user-attachments/assets/e4a6fa61-e4c7-4141-a911-e6ff7288594a" /><img width="960" height="544" alt="image" src="https://github.com/user-attachments/assets/c214f013-4eb4-4a7b-ae88-ba029744d805" /><img width="960" height="544" alt="image" src="https://github.com/user-attachments/assets/0eff0f9e-b6fe-41ad-be83-1d0a18772021" />

## Credits
* [isage](https://github.com/isage) for the original port.
* [Pickle](https://github.com/Pickle/GishGLES) for OpenGL ES renderer.
* [EXL](https://github.com/EXL/Gish) for improvements.
* [Graphene](https://github.com/GrapheneCt/PVR_PSP2) for PVR_PSP2.

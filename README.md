Gish Vita: ReVibed
====

Gish is a side-scrolling platformer video game with some physics puzzle elements developed by Cryptic Sea (pseudonym of Alex Austin), Edmund McMillen, Josiah Pisciotta and published by Chronic Logic in 2004. The game was featured in the first Humble Indie Bundle in May 2010. Following the success of the Humble Bundle promotion, Cryptic Sea pledged to go open source with the game which eventually happened under the GPLv2 on May 29, 2010. - [Wikipedia](https://en.wikipedia.org/wiki/Gish_(video_game)).

This is a fork of [isage's port of Gish to PS Vita](https://github.com/isage/Gish-vita), with the purpose of polishing it further into a more playable experience.

The port uses SDL2, PVR_PSP2, OpenAL, and Ogg Vorbis libraries.

**_Full disclaimer_**: all changes made here **were done by AI** (Qwen3.8-27B local model running on an RTX 3090 in my personal PC - no datacenters involved!). Each change was directed and reviewed by me, but I'm not able to code in C myself - only (somewhat) understand what the model did. The changes are small and unlikely to break anything. I tested the game after each change, but haven't completed it fully - the performance in certain sections still needs a lot of optimization work.

## Download

You can download an installable VPK from the [Releases](https://github.com/AGTS10k/Gish-vita-revibed/releases) section. This VPK will install over the original port.
You need data files from Gish 1.6 (non-steam, MS Windows), Gish 1.53 (non-steam, Linux) or Gish 1.7 (steam version).
Steam 1.7 can be obtained by selecting `version170` in "betas".

Copy data to `ux0:/data/gish/`

## Credits
* [isage](https://github.com/isage) for the original port.
* [Pickle](https://github.com/Pickle/GishGLES) for OpenGL ES renderer.
* [EXL](https://github.com/EXL/Gish) for improvements.
* [Graphene](https://github.com/GrapheneCt/PVR_PSP2) for PVR_PSP2.

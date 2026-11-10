/*
Copyright (C) 2005, 2010 - Cryptic Sea

This file is part of Gish.

Gish is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include "../game/mainmenu.h"

#include "../audio/audio.h"
#include "../config.h"
#include "../game/animation.h"
#include "../game/config.h"
#include "../game/credits.h"
#include "../game/debug.h"
#include "../game/editor.h"
#include "../game/english.h"
#include "../game/game.h"
#include "../game/gameaudio.h"
#include "../game/gameobject.h"
#include "../game/lighting.h"
#include "../game/music.h"
#include "../game/options.h"
#include "../game/physics.h"
#include "../game/player.h"
#include "../game/socket.h"
#include "../game/vsmode.h"
#include "../input/joystick.h"
#include "../input/keyboard.h"
#include "../input/mouse.h"
#include "../math/vector.h"
#include "../menu/menu.h"
#include "../sdl/event.h"
#include "../sdl/video.h"
#include "../video/opengl.h"
#include "../video/text.h"
#include "../video/texture.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>

void mainmenu(void)
{
  int count, temp;
  int prevjoymenunum;
  // int registered;
  char loadfilename[32] = "loading00.tga";
  char texfilename[32]  = "text000.tga";
  // unsigned int x,y;

  srand(time(NULL));
  count = (rand() % 11) + 1;

  loadfilename[7] = 48 + count / 10;
  loadfilename[8] = 48 + count % 10;
  loadbackground(800, loadfilename);

  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
  glClear(GL_COLOR_BUFFER_BIT);

  setuptextdisplay();

  glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
  displaybackground(800);

  drawtext(TXT_LOADING, (320 | TEXT_CENTER), 448, 16, 1.0f, 1.0f, 1.0f, 1.0f);

#if defined(PC_GLES)
  eglSwapBuffers(eglDisplay, eglSurface);
#else
  SDL_GL_SwapWindow(globalwindow);
#endif

  for (count = 0; count < 64; count++)
  {
    texfilename[4] = 48 + (count / 100) % 10;
    texfilename[5] = 48 + (count / 10) % 10;
    texfilename[6] = 48 + count % 10;
    loadtexturetga(count + 256, texfilename, 0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
  }

  loadtexturetga(256 + 64, "cage1.tga", 0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
  loadtexturetga(256 + 65, "cage2.tga", 0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
  loadtexturetga(256 + 66, "cage3.tga", 0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
  loadtexturetga(256 + 67, "cage4.tga", 0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
  loadtexturetga(256 + 68, "x.tga", 0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);

  setuplightingtextures();

  loadbackground(470, "gishtitle.tga");

  loadtexturetga(462, "complete.tga", 0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
  loadtexturetga(465, "breatalk.tga", 0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);

  loadtexturetga(468, "lvlcomplete.tga", 0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
  loadtexturetga(469, "gishy.tga", 0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);

  loadtexturetga(512, "gishtalk.tga", 0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
  loadtexturetga(513, "honeytalk.tga", 0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
  loadtexturetga(514, "paunchytalk.tga", 0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
  loadtexturetga(515, "vistalk.tga", 0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
  loadtexturetga(516, "bastalk.tga", 0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
  loadtexturetga(517, "sistalk.tga", 0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
  loadtexturetga(518, "heratalk.tga", 0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
  loadtexturetga(519, "badending.tga", 0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
  loadtexturetga(520, "goodending.tga", 0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
  loadtexturetga(521, "sewerimg.tga", 0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
  loadtexturetga(522, "caveimg.tga", 0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
  loadtexturetga(523, "hellimg.tga", 0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
  loadtexturetga(524, "egyptimg.tga", 0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
  loadtexturetga(525, "churchimg.tga", 0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
  loadtexturetga(526, "warpimg.tga", 0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
  loadtexturetga(527, "satantalk.tga", 0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);

  int crypticseatex
      = loadtexturetga(528, "crypticsea.tga", 0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
  loadtexturetga(529, "turbo.tga", 0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
  loadtexturetga(530, "sumo.tga", 0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
  loadtexturetga(531, "football.tga", 0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);

  loadtexturetga(532, "greed.tga", 0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
  if (cache_fix)
    loadtexturetga(533, "duel.tga", 0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
  else
    loadtexturetga(533, "pitfight.tga", 0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
  loadtexturetga(534, "dragster.tga", 0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
  loadtexturetga(535, "collect.tga", 0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);

  loadbackground(540, "level.tga");

  loadbackground(560, "congrats.tga");
  loadbackground(580, "blank.tga");

  loadbackground(600, "vsmode.tga");
  loadbackground(620, "storymode.tga");

  /* The 4:3 border is also the source for expand mode: the 16:9 frame is
     composed from it at draw time in game.c (see the expand branch there), so
     there is no second border texture.  6 tiles of 256x256: columns x=0/256/512,
     rows y=0/256.  The art is 640x480, so the third column and the second row
     only have 128/224 valid texels; the rest is never sampled. */
  loadtexturetgapartial(640, "border.tga", 0, 0, 256, 256);
  loadtexturetgapartial(641, "border.tga", 256, 0, 256, 256);
  loadtexturetgapartial(642, "border.tga", 512, 0, 256, 256);
  loadtexturetgapartial(643, "border.tga", 0, 256, 256, 256);
  loadtexturetgapartial(644, "border.tga", 256, 256, 256, 256);
  loadtexturetgapartial(645, "border.tga", 512, 256, 256, 256);

  loadtexturetga(820, "gishwin.tga", 0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
  loadtexturetga(821, "graywin.tga", 0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
  loadtexturetga(822, "fight.tga", 0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
  loadtexturetga(823, "ready.tga", 0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);

  loadtexturetga(700, "0.tga", 0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
  loadtexturetga(701, "1.tga", 0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
  loadtexturetga(702, "2.tga", 0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
  loadtexturetga(703, "3.tga", 0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
  loadtexturetga(704, "4.tga", 0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
  loadtexturetga(705, "5.tga", 0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
  loadtexturetga(706, "6.tga", 0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
  loadtexturetga(707, "7.tga", 0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
  loadtexturetga(708, "8.tga", 0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
  loadtexturetga(709, "9.tga", 0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
  loadtexturetga(710, "10.tga", 0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
  loadtexturetga(711, "11.tga", 0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
  loadtexturetga(712, "12.tga", 0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);

  loadtexturetga(720, "gishhud.tga", 0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
  loadtexturetga(721, "greyhud.tga", 0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
  loadtexturetga(722, "tarballs.tga", 0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
  loadtexturetga(723, "key.tga", 0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);

  loadtexturetga(360, "rope.tga", 0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
  loadtexturetga(361, "chain.tga", 0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
  loadtexturetga(362, "piston1.tga", 0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);

  loadtexturetga(363, "tar2.tga", 0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
  loadtexturetga(364, "poop.tga", 0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
  loadtexturetga(365, "dust.tga", 0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
  loadtexturetga(366, "bubble.tga", 0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
  loadtexturetga(367, "blood.tga", 0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
  loadtexturetga(368, "tar.tga", 0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
  loadtexturetga(369, "amber.tga", 0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);

  loadtexturetga(370, "face.tga", 0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
  loadtexturetga(371, "facem1.tga", 0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
  loadtexturetga(372, "facem2.tga", 0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
  loadtexturetga(373, "facem3.tga", 0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
  loadtexturetga(374, "facem4.tga", 0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
  loadtexturetga(375, "faceb1.tga", 0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
  loadtexturetga(376, "faceb2.tga", 0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
  loadtexturetga(377, "faceb1.tga", 0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
  loadtexturetga(378, "faceh1.tga", 0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
  loadtexturetga(379, "faceh2.tga", 0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
  loadtexturetga(380, "faceh3.tga", 0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
  loadtexturetga(381, "faceh4.tga", 0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
  loadtexturetga(382, "gishhurt.tga", 0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);

  loadtexturetga(370 + 20, "gface.tga", 0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
  loadtexturetga(371 + 20, "gfacem1.tga", 0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
  loadtexturetga(372 + 20, "gfacem2.tga", 0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
  loadtexturetga(373 + 20, "gfacem3.tga", 0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
  loadtexturetga(374 + 20, "gfacem4.tga", 0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
  loadtexturetga(375 + 20, "gfaceb1.tga", 0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
  loadtexturetga(376 + 20, "gfaceb2.tga", 0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
  loadtexturetga(377 + 20, "gfaceb1.tga", 0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
  loadtexturetga(378 + 20, "gfaceh01.tga", 0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
  loadtexturetga(379 + 20, "gfaceh02.tga", 0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
  loadtexturetga(380 + 20, "gfaceh03.tga", 0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
  loadtexturetga(381 + 20, "gfaceh03.tga", 0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
  loadtexturetga(382 + 20, "grayhurt.tga", 0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);

  loadtexturetga(370 + 40, "bface.tga", 0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
  loadtexturetga(371 + 40, "bfacem1.tga", 0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
  loadtexturetga(372 + 40, "bfacem2.tga", 0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
  loadtexturetga(373 + 40, "bfacem3.tga", 0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
  loadtexturetga(374 + 40, "bfacem4.tga", 0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
  loadtexturetga(375 + 40, "bfaceb1.tga", 0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
  loadtexturetga(376 + 40, "bfaceb2.tga", 0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
  loadtexturetga(377 + 40, "bfaceb1.tga", 0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
  loadtexturetga(378 + 40, "bfaceh1.tga", 0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
  loadtexturetga(379 + 40, "bfaceh2.tga", 0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
  loadtexturetga(380 + 40, "bfaceh3.tga", 0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
  loadtexturetga(381 + 40, "bfaceh3.tga", 0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
  loadtexturetga(382 + 40, "bhurt.tga", 0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
  loadtexturetga(383 + 40, "btar.tga", 0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);

  loadtexturetga(370 + 60, "rface.tga", 0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
  loadtexturetga(371 + 60, "rfacem1.tga", 0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
  loadtexturetga(372 + 60, "rfacem2.tga", 0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
  loadtexturetga(373 + 60, "rfacem3.tga", 0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
  loadtexturetga(374 + 60, "rfacem4.tga", 0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
  loadtexturetga(375 + 60, "rfaceb1.tga", 0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
  loadtexturetga(376 + 60, "rfaceb2.tga", 0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
  loadtexturetga(377 + 60, "rfaceb1.tga", 0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
  loadtexturetga(378 + 60, "rfaceh1.tga", 0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
  loadtexturetga(379 + 60, "rfaceh2.tga", 0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
  loadtexturetga(380 + 60, "rfaceh3.tga", 0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
  loadtexturetga(381 + 60, "rfaceh3.tga", 0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
  loadtexturetga(382 + 60, "rhurt.tga", 0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
  loadtexturetga(383 + 60, "rtar.tga", 0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);

  numofanimations = 1024;

  // loadanimations();

  if (config.sound)
    loadoggs();

  setupphysics();

  setupobjecttypes();

  editor.mode = 1;

  game.songnum        = -1;
  game.currentsongnum = -1;
  resetorientation(view.orientation);
  soundsimulation(view.position, view.orientation);

  introscreen();

  resetmenuitems();

  joymenunum     = 1;
  game.turbomode = config.turbomode;

  while (!menuitem[0].active && !windowinfo.shutdown)
  {
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    numofmenuitems = 0;
    temp = (sizeof(TXT_EXIT) + sizeof(TXT_CREDITS) + sizeof(TXT_OPTIONS) + sizeof(TXT_VERSUS) + sizeof(TXT_START) - 1)
           * 8;

    createmenuitem(TXT_EXIT, (320 + temp) | TEXT_END, 380, 16, 1.0f, 1.0f, 1.0f, 1.0f);
    setmenuitem(MO_HOTKEY, SCAN_ESC);
    createmenuitem(TXT_START, 320 - temp, 380, 16, 1.0f, 1.0f, 1.0f, 1.0f);
    setmenuitem(MO_HOTKEY, SCAN_S);
    temp -= (sizeof(TXT_START)) * 16;
    //    createmenuitem(TXT_VERSUS,320-temp,380,16,1.0f,1.0f,1.0f,1.0f);
    //    setmenuitem(MO_HOTKEY,SCAN_V);
    temp -= (sizeof(TXT_VERSUS)) * 16;
    //    createmenuitem(TXT_OPTIONS,320-temp,380,16,1.0f,1.0f,1.0f,1.0f);
    //    setmenuitem(MO_HOTKEY,SCAN_O);
    temp -= (sizeof(TXT_OPTIONS)) * 16;
    createmenuitem(TXT_CREDITS, 320 - temp, 380, 16, 1.0f, 1.0f, 1.0f, 1.0f);
    setmenuitem(MO_HOTKEY, SCAN_C);
    //    createmenuitem("Cryptic Sea",0,0,1,1.0f,1.0f,1.0f,1.0f);
    //    if (!(crypticseatex == -1)) {
    //        setmenuitem(MO_IMAGE,528);
    //        setmenuitem(MO_RESIZE,16,0,96,48);
    //    }

    checksystemmessages();
    checkkeyboard();
    checkmouse();
    checkjoystick();
    checkmenuitems();

    if (keyboard[SCAN_T] && !prevkeyboard[SCAN_T])
      game.turbomode ^= 1;

    updateogg();
    checkmusic();
    game.songnum = 8;
    soundsimulation(view.position, view.orientation);

    setuptextdisplay();

    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    displaybackground(470);

    if (game.turbomode)
      drawtext(TXT_TURBO_EDITION, (640 | TEXT_END), 470, 10, 1.0f, 1.0f, 1.0f, 1.0f);
    drawtext("Version 1.53V", 0, 470, 10, 1.0f, 1.0f, 1.0f, 1.0f);

    if (joymenunum == 5)
    {
      drawtext(TXT_COPYRIGHT, (320 | TEXT_CENTER), 470, 10, 1.0f, 1.0f, 1.0f, 1.0f);
    }
    else
    {
      drawtext(TXT_COPYRIGHT, (320 | TEXT_CENTER), 470, 10, 0.75f, 0.75f, 0.75f, 1.0f);
    }

    drawmenuitems();

    drawmousecursor(768 + font.cursornum, mouse.x, mouse.y, 16, 1.0f, 1.0f, 1.0f, 1.0f);

#if defined(PC_GLES)
    eglSwapBuffers(eglDisplay, eglSurface);
#else
    SDL_GL_SwapWindow(globalwindow);
#endif

    if (menuitem[5].active)
    {
      launchwebpage("www.crypticsea.com");
      menuitem[5].active = 0;
    }
    if (menuitem[4].active)
    {
      // game.songnum=-1;
      // checkmusic();

      prevjoymenunum = 4;
      creditsmenu();
      joymenunum = prevjoymenunum;
    }
    if (menuitem[3].active)
    {
      game.songnum = -1;
      checkmusic();

      prevjoymenunum = 3;
      optionsmenu();
      joymenunum = prevjoymenunum;
    }
    if (menuitem[2].active)
    {
      // game.songnum=-1;
      // checkmusic();

      prevjoymenunum = 2;
      //      versusmenu();
      creditsmenu();
      joymenunum = prevjoymenunum;
    }
    if (menuitem[1].active)
    {
      // game.songnum=-1;
      // checkmusic();

      prevjoymenunum = 1;
      playermenu();
      joymenunum = prevjoymenunum;
    }
  }

  game.songnum = -1;
  checkmusic();
}

void loadbackground(int texturenum, char *filename)
{
  loadtexturetgapartial(texturenum, filename, 0, 0, 256, 256);
  loadtexturetgapartial(texturenum + 1, filename, 256, 0, 256, 256);
  loadtexturetgapartial(texturenum + 2, filename, 512, 0, 256, 256);
  loadtexturetgapartial(texturenum + 3, filename, 768, 0, 256, 256);
  loadtexturetgapartial(texturenum + 4, filename, 0, 256, 256, 256);
  loadtexturetgapartial(texturenum + 5, filename, 256, 256, 256, 256);
  loadtexturetgapartial(texturenum + 6, filename, 512, 256, 256, 256);
  loadtexturetgapartial(texturenum + 7, filename, 768, 256, 256, 256);
  loadtexturetgapartial(texturenum + 8, filename, 0, 512, 256, 256);
  loadtexturetgapartial(texturenum + 9, filename, 256, 512, 256, 256);
  loadtexturetgapartial(texturenum + 10, filename, 512, 512, 256, 256);
  loadtexturetgapartial(texturenum + 11, filename, 768, 512, 256, 256);
}

void displaybackground(int texturenum)
{
  if (hudfrustumscale > 1.0f)
  {
    /* Expand mode: the widened HUD frustum would let these quads spill into
       the side strips (their eye coords reach x = +1.56).  Clip the menu
       background back to the central 4:3 strip so menus look exactly as they
       do in 4:3 mode. */
    int stripw = (windowinfo.resolutiony * 4) / 3;

    glEnable(GL_SCISSOR_TEST);
    glScissor((windowinfo.resolutionx - stripw) / 2, 0, stripw, windowinfo.resolutiony);
  }

  drawbackground(texturenum, 0, 0, 256, 256, 800, 600);
  drawbackground(texturenum + 1, 256, 0, 256, 256, 800, 600);
  drawbackground(texturenum + 2, 512, 0, 256, 256, 800, 600);
  drawbackground(texturenum + 3, 768, 0, 256, 256, 800, 600);
  drawbackground(texturenum + 4, 0, 256, 256, 256, 800, 600);
  drawbackground(texturenum + 5, 256, 256, 256, 256, 800, 600);
  drawbackground(texturenum + 6, 512, 256, 256, 256, 800, 600);
  drawbackground(texturenum + 7, 768, 256, 256, 256, 800, 600);
  drawbackground(texturenum + 8, 0, 512, 256, 256, 800, 600);
  drawbackground(texturenum + 9, 256, 512, 256, 256, 800, 600);
  drawbackground(texturenum + 10, 512, 512, 256, 256, 800, 600);
  drawbackground(texturenum + 11, 768, 512, 256, 256, 800, 600);

  if (hudfrustumscale > 1.0f)
    glDisable(GL_SCISSOR_TEST);
}

/* ---------------------------------------------------------------------------
   EXPAND-MODE LEVEL BACKDROP (config.txt: keepaspect=2)

   A level's backdrop (level.background) is an 800x600 image, loaded as twelve
   256x256 tiles by loadbackground() and drawn by displaybackground() over the
   whole 800x600 design space - which is exactly the screen in 4:3.  In expand
   mode the HUD frustum is widened by hudfrustumscale (960/725 = 1.3241), so
   the same 800x600 space is 1059.3 design units wide and the image alone would
   leave 129.65 units of black down each side.  displaybackground() covers that
   up by scissoring the backdrop to the central 4:3 strip, which is right for
   menus but leaves the level itself with two black bars.

   The three pattern backdrops are widened here the same way the 16:9 border is
   built from border.tga: extra columns of the same image are drawn 1:1 beside
   it, so the composition is wider than the screen and gets centred, and no
   pixel is ever resampled.  Which columns to repeat was chosen by eye for each
   image; the comment by each row says what the composition adds up to.

   church2.tga is a photograph - a repeated band of sky and mountains would be
   obvious - so it is zoomed by 1059.3/800 = 1.3241 instead and the top and
   bottom are cropped away.  The crop starts at row 60 and is as tall as the
   zoom leaves room for (453.1 rows), which keeps the aspect ratio exact.
   --------------------------------------------------------------------------- */
typedef struct
{
  const char *filename;
  int pieces;          /* how many source ranges are laid out, left to right */
  int srcx0[4];        /* first source column of each piece, 1-based */
  int srcx1[4];        /* last source column of each piece, 1-based */
} _backdroplayout;

static const _backdroplayout backdroplayout[] = {
    /* 671..800 | 1..800 | 1..130 = 130 + 800 + 130 = 1060 design units.  Both
       joins are column 800 followed by column 1, which is how the brickwork
       wraps, so they land on matching content. */
    {"egypt.tga", 3, {671, 1, 1}, {800, 800, 130}},
    /* 270..402 | 5..402 | 5..532 = 133 + 398 + 528 = 1059 design units.  Both
       joins are column 402 followed by column 5.  The cave art is inset from
       the sides of the image, so columns 1..4 and 533..800 (the light border)
       are never sampled. */
    {"cave.tga", 3, {269, 5, 5}, {402, 402, 532}},
    /* 273..401 | 1..800 | 400..529 = 129 + 800 + 130 = 1059 design units.  The
       joins are 401->1 and 800->400. */
    {"sewer.tga", 3, {272, 1, 400}, {401, 800, 529}},
};

/* Draws the source rectangle (sx0,sy0)-(sx1,sy1) of an 800x600 backdrop onto
   the design rectangle (dx0,dy0)-(dx1,dy1), splitting the quad at the 256x256
   tile boundaries the image was loaded in (loadbackground()).  Source
   coordinates are image pixels, origin top left, x1/y1 exclusive; the
   destination is in 800x600 design units.  Both rectangles must be positive:
   drawbackgroundsub() masks negative x with 0xFFFF, so callers shift the
   modelview instead of passing negative coordinates. */
static void drawbackdropmap(int texturenum, float sx0, float sy0, float sx1, float sy1, float dx0, float dy0,
                            float dx1, float dy1)
{
  int tilex, tiley;
  float rx0, rx1, ry0, ry1, ix0, ix1, iy0, iy1;
  int px0, px1, py0, py1;

  for (tiley = 0; tiley < 3; tiley++)
  {
    ry0 = (float)(tiley * 256);
    ry1 = ry0 + 256.0f;
    iy0 = (sy0 > ry0 ? sy0 : ry0);
    iy1 = (sy1 < ry1 ? sy1 : ry1);

    if (iy1 <= iy0)
      continue;

    py0 = (int)(dy0 + (iy0 - sy0) * (dy1 - dy0) / (sy1 - sy0) + 0.5f);
    py1 = (int)(dy0 + (iy1 - sy0) * (dy1 - dy0) / (sy1 - sy0) + 0.5f);

    if (py1 <= py0)
      continue;

    for (tilex = 0; tilex < 4; tilex++)
    {
      rx0 = (float)(tilex * 256);
      rx1 = rx0 + 256.0f;
      ix0 = (sx0 > rx0 ? sx0 : rx0);
      ix1 = (sx1 < rx1 ? sx1 : rx1);

      if (ix1 <= ix0)
        continue;

      /* Both edges of a tile are rounded the same way as the matching edge of
         its neighbour, so the quads stay edge to edge instead of leaving a
         one-pixel gap. */
      px0 = (int)(dx0 + (ix0 - sx0) * (dx1 - dx0) / (sx1 - sx0) + 0.5f);
      px1 = (int)(dx0 + (ix1 - sx0) * (dx1 - dx0) / (sx1 - sx0) + 0.5f);

      if (px1 <= px0)
        continue;

      drawbackgroundsub(texturenum + tiley * 4 + tilex, px0, py0, px1 - px0, py1 - py0, (ix0 - rx0) / 256.0f,
                        (iy0 - ry0) / 256.0f, (ix1 - rx0) / 256.0f, (iy1 - ry0) / 256.0f, 800, 600);
    }
  }
}

/* The backdrop for the level that is being played.  In 4:3 (and for any image
   without an expand layout) this is displaybackground(), untouched. */
void displaylevelbackground(int texturenum, const char *filename)
{
  const _backdroplayout *layout = NULL;
  int count, total = 0, dstx = 0;
  float extra, shift, width, srcheight;

  if (hudfrustumscale <= 1.0f)
  {
    displaybackground(texturenum);

    return;
  }

  /* Half the added width, in 800x600 design units.  1 NDC unit is 400 units
     wide here (convertscreenvertex() divides by resolutionx/2), hence /400. */
  extra = (hudfrustumscale - 1.0f) * 400.0f;

  if (strcmp(filename, "church2.tga") == 0)
  {
    width     = 800.0f + extra * 2.0f;
    srcheight = 600.0f * 800.0f / width; /* rows that fit the 1.324x zoom */

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(-extra / 400.0f, 0.0f, 0.0f);
    drawbackdropmap(texturenum, 0.0f, 60.0f, 800.0f, 60.0f + srcheight, 0.0f, 0.0f, width, 600.0f);
    glLoadIdentity();
    glMatrixMode(GL_TEXTURE);

    return;
  }

  for (count = 0; count < (int)(sizeof(backdroplayout) / sizeof(backdroplayout[0])); count++)
    if (strcmp(filename, backdroplayout[count].filename) == 0)
      layout = &backdroplayout[count];

  if (layout == NULL)
  {
    /* No expand layout for this backdrop: leave it in the 4:3 strip. */
    displaybackground(texturenum);

    return;
  }

  for (count = 0; count < layout->pieces; count++)
    total += layout->srcx1[count] - layout->srcx0[count] + 1;

  /* Centre the composition on the screen: it is 1059-1060 units wide against
     the 1059.3 the frustum covers, so the outer half-unit is clipped. */
  shift = (float)(total - 800) * 0.5f;

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glTranslatef(-shift / 400.0f, 0.0f, 0.0f);

  dstx = 0;
  for (count = 0; count < layout->pieces; count++)
  {
    width = (float)(layout->srcx1[count] - layout->srcx0[count] + 1);

    drawbackdropmap(texturenum, (float)(layout->srcx0[count] - 1), 0.0f, (float)layout->srcx1[count], 600.0f,
                    (float)dstx, 0.0f, (float)dstx + width, 600.0f);
    dstx += (int)width;
  }

  glLoadIdentity();
  glMatrixMode(GL_TEXTURE);
}

void versusmenu(void)
{
  int count;
  int unlocked;

  unlocked = 0;
  for (count = 0; count < 6; count++)
    if (player[count].unlock[0])
      unlocked = 1;

  resetmenuitems();

  joymenunum = 1;

  while (!menuitem[0].active && !windowinfo.shutdown)
  {
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    numofmenuitems = 0;
    createmenuitem(TXT_BACK, 0, 0, 16, 1.0f, 1.0f, 1.0f, 1.0f);
    setmenuitem(MO_HOTKEY, SCAN_ESC);

    count = 64;
    createmenuitem(TXT_SUMO, (144 | TEXT_CENTER), 304, 16, 1.0f, 1.0f, 1.0f, 1.0f);
    setmenuitem(MO_HOTKEY, SCAN_S);
    createmenuitem(TXT_FOOTBALL, (320 | TEXT_CENTER), 304, 16, 1.0f, 1.0f, 1.0f, 1.0f);
    setmenuitem(MO_HOTKEY, SCAN_F);
    createmenuitem(TXT_GREED, (500 | TEXT_CENTER), 304, 16, 1.0f, 1.0f, 1.0f, 1.0f);
    setmenuitem(MO_HOTKEY, SCAN_G);
    if (cache_fix)
    {
      createmenuitem(TXT_DUEL, (152 | TEXT_CENTER), 444, 16, 1.0f, 1.0f, 1.0f, 1.0f);
      setmenuitem(MO_HOTKEY, SCAN_U);
    }
    else
    {
      createmenuitem(TXT_PITFIGHT, (152 | TEXT_CENTER), 444, 16, 1.0f, 1.0f, 1.0f, 1.0f);
      setmenuitem(MO_HOTKEY, SCAN_P);
    }
    createmenuitem(TXT_DRAGSTER, (320 | TEXT_CENTER), 444, 16, 1.0f, 1.0f, 1.0f, 1.0f);
    setmenuitem(MO_HOTKEY, SCAN_D);
    if (unlocked)
    {
      createmenuitem(TXT_COLLECTION, (500 | TEXT_CENTER), 444, 16, 1.0f, 1.0f, 1.0f, 1.0f);
      setmenuitem(MO_HOTKEY, SCAN_C);
    }

    checksystemmessages();
    checkkeyboard();
    checkmouse();
    checkjoystick();
    checkmenuitems();

    updateogg();
    checkmusic();
    game.songnum = 8;
    soundsimulation(view.position, view.orientation);

    setuptextdisplay();

    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    displaybackground(600);

    if (!unlocked)
    {
      drawbackground(256 + 68, 448, 328, 96, 96, 640, 480);
      drawtext("?????", (500 | TEXT_CENTER), 444, 16, 1.0f, 1.0f, 1.0f, 1.0f);
    }

    drawmenuitems();

    drawmousecursor(768 + font.cursornum, mouse.x, mouse.y, 16, 1.0f, 1.0f, 1.0f, 1.0f);

#if defined(PC_GLES)
    eglSwapBuffers(eglDisplay, eglSurface);
#else
    SDL_GL_SwapWindow(globalwindow);
#endif

    for (count = 1; count <= 7; count++)
      if (menuitem[count].active)
        versusmodemenu(count - 1);
  }

  resetmenuitems();
}

void storyscreen(void)
{
  int count;
  int simtimer;
  int simcount;
  int loaddelay;
  int screennum;

  loaddelay = 0;
  screennum = 0;

  loadbackground(824, "story1.tga");

  simtimer = SDL_GetTicks();

  resetmenuitems();

  joymenunum = 1;

  while (!menuitem[0].active && screennum < 8 && !windowinfo.shutdown)
  {
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    numofmenuitems = 0;
    createmenuitem(TXT_BACK, 0, 0, 16, 1.0f, 1.0f, 1.0f, 1.0f);
    setmenuitem(MO_HOTKEY, SCAN_ESC);
    createmenuitem(TXT_NEXT, (640 | TEXT_END), 0, 16, 1.0f, 1.0f, 1.0f, 1.0f);
    setmenuitem(MO_HOTKEY, SCAN_N);

    checksystemmessages();
    checkkeyboard();
    checkmouse();
    checkjoystick();
    checkmenuitems();

// Disable buggy music in story on Android
#ifndef ANDROID_NDK
    updateogg();
    checkmusic();
    game.songnum = 8;
    soundsimulation(view.position, view.orientation);
#endif

    setuptextdisplay();

    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    displaybackground(824);

    drawmenuitems();

    drawmousecursor(768 + font.cursornum, mouse.x, mouse.y, 16, 1.0f, 1.0f, 1.0f, 1.0f);

#if defined(PC_GLES)
    eglSwapBuffers(eglDisplay, eglSurface);
#else
    SDL_GL_SwapWindow(globalwindow);
#endif

    simcount = 0;
    while (SDL_GetTicks() - simtimer > 20 && simcount < 5)
    {
      simcount++;
      count    = SDL_GetTicks() - simtimer - 20;
      simtimer = SDL_GetTicks() - count;

      loaddelay++;
      if (loaddelay >= 400)
      {
        screennum++;
        if (screennum == 1)
          loadbackground(824, "story2.tga");
        if (screennum == 2)
          loadbackground(824, "story3.tga");
        if (screennum == 3)
          loadbackground(824, "story4.tga");
        if (screennum == 4)
          loadbackground(824, "story5.tga");
        if (screennum == 5)
          loadbackground(824, "story6.tga");
        if (screennum == 6)
          loadbackground(824, "story7.tga");
        if (screennum == 7)
          loadbackground(824, "story8.tga");
        loaddelay = 0;
      }
    }
    if (menuitem[1].active)
    {
      screennum++;
      if (screennum == 1)
        loadbackground(824, "story2.tga");
      if (screennum == 2)
        loadbackground(824, "story3.tga");
      if (screennum == 3)
        loadbackground(824, "story4.tga");
      if (screennum == 4)
        loadbackground(824, "story5.tga");
      if (screennum == 5)
        loadbackground(824, "story6.tga");
      if (screennum == 6)
        loadbackground(824, "story7.tga");
      if (screennum == 7)
        loadbackground(824, "story8.tga");
      loaddelay          = 0;
      menuitem[1].active = 0;
    }
  }

  resetmenuitems();
}

void introscreen(void)
{
  int count, count2 = 0;
  int simtimer;
  int simcount;
  int loaddelay;
  float alpha;
  int secretsequence;
  float vec[3];

  srand(time(NULL));

  loaddelay = 0;

  secretsequence = 0;

  simtimer = SDL_GetTicks();

  resetmenuitems();

  while (!menuitem[0].active && loaddelay < 200 && !windowinfo.shutdown)
  {
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    numofmenuitems = 0;
    createmenuitem(" ", 0, 0, 640, 1.0f, 1.0f, 1.0f, 1.0f);
    setmenuitem(MO_HOTKEY, SCAN_ESC);

    checksystemmessages();
    checkkeyboard();
    checkmouse();
    checkjoystick();
    checkmenuitems();

    if (secretsequence == 0)
      count2 = 3;
    if (secretsequence == 1)
      count2 = 3;
    if (secretsequence == 2)
      count2 = 2;
    if (secretsequence == 3)
      count2 = 2;
    if (secretsequence == 4)
      count2 = 0;
    if (secretsequence == 5)
      count2 = 1;
    if (secretsequence == 6)
      count2 = 0;
    if (secretsequence == 7)
      count2 = 1;
    if (secretsequence == 8)
      count2 = 4;
    if (secretsequence == 9)
      count2 = 5;
    if (secretsequence == 10)
    {
      if (game.turbomode == 0)
      {
        vec[0] = 0.0f;
        vec[1] = 0.0f;
        vec[2] = 0.0f;
        playsound(10, vec, NULL, 0.4f, 0, 1.0f, -1, 0);
      }
      game.turbomode = 1;
    }

    for (count = 0; count < KEYALIAS_LENGTH; count++)
    {
      if (keyboard[control[0].key[count]] && !prevkeyboard[control[0].key[count]])
      {
        if (count == count2)
          secretsequence++;
        else
          secretsequence = 0;
      }

      if (control[0].joysticknum != -1)
      {
        if (count == 0)
          if (joystick[control[0].joysticknum].axis[0] <= -0.5f && prevjoystick[control[0].joysticknum].axis[0] > -0.5f)
          {
            if (count == count2)
              secretsequence++;
            else
              secretsequence = 0;
          }
        if (count == 1)
          if (joystick[control[0].joysticknum].axis[0] >= 0.5f && prevjoystick[control[0].joysticknum].axis[0] < 0.5f)
          {
            if (count == count2)
              secretsequence++;
            else
              secretsequence = 0;
          }
        if (count == 2)
          if (joystick[control[0].joysticknum].axis[1] <= -0.5f && prevjoystick[control[0].joysticknum].axis[1] > -0.5f)
          {
            if (count == count2)
              secretsequence++;
            else
              secretsequence = 0;
          }
        if (count == 3)
          if (joystick[control[0].joysticknum].axis[1] >= 0.5f && prevjoystick[control[0].joysticknum].axis[1] < 0.5f)
          {
            if (count == count2)
              secretsequence++;
            else
              secretsequence = 0;
          }
        if (control[0].button[count] != -1)
          if (joystick[control[0].joysticknum].button[control[0].button[count]]
              && !prevjoystick[control[0].joysticknum].button[control[0].button[count]])
          {
            if (count == count2)
              secretsequence++;
            else
              secretsequence = 0;
          }
      }
    }

    updateogg();
    checkmusic();
    game.songnum = 8;
    soundsimulation(view.position, view.orientation);

    setuptextdisplay();

    if (loaddelay < 100)
    {
      alpha = (float)(100 - loaddelay) / 100.0f;
      glColor4f(1.0f, 1.0f, 1.0f, alpha);
      displaybackground(800);
    }
    else
    {
      alpha = (float)(loaddelay - 100) / 100.0f;
      glColor4f(1.0f, 1.0f, 1.0f, alpha);
      displaybackground(470);

      drawtext(TXT_EXIT, (320 + 72 + 128), 380, 16, 0.5f, 0.5f, 0.5f, alpha);
      drawtext(TXT_START, (320 - 72 - 112) | TEXT_END, 380, 16, 0.5f, 0.5f, 0.5f, alpha);
      //      drawtext(TXT_VERSUS,(320-72)|TEXT_END,380,16,0.5f,0.5f,0.5f,alpha);
      //      drawtext(TXT_OPTIONS,(320)|TEXT_CENTER,380,16,0.5f,0.5f,0.5f,alpha);
      drawtext(TXT_CREDITS, (320 + 72), 380, 16, 0.5f, 0.5f, 0.5f, alpha);

      if (game.turbomode)
        drawtext(TXT_TURBO_EDITION, (640 | TEXT_END), 470, 10, 1.0f, 1.0f, 1.0f, 1.0f);
      drawtext("Version 1.53V", 0, 470, 10, 1.0f, 1.0f, 1.0f, 1.0f);

      drawtext(TXT_COPYRIGHT, (320 | TEXT_CENTER), 470, 10, 0.75f, 0.75f, 0.75f, alpha);
    }
    drawmenuitems();

    drawmousecursor(768 + font.cursornum, mouse.x, mouse.y, 16, 1.0f, 1.0f, 1.0f, 1.0f);

#if defined(PC_GLES)
    eglSwapBuffers(eglDisplay, eglSurface);
#else
    SDL_GL_SwapWindow(globalwindow);
#endif

    simcount = 0;
    while (SDL_GetTicks() - simtimer > 20 && simcount < 5)
    {
      simcount++;
      count    = SDL_GetTicks() - simtimer - 20;
      simtimer = SDL_GetTicks() - count;

      loaddelay++;
    }
  }

  resetmenuitems();
}

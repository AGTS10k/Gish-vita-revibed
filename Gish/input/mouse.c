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

#include "../input/mouse.h"

#include "../config.h"
#include "../sdl/event.h"
#include "../sdl/sdl.h"

#include <string.h>

_mouse mouse, prevmouse;

#ifdef __vita__
/* The front touch panel stands in for the mouse (see updatetouch() below).
   The pointer only exists while a finger owns it: until then nothing is
   highlighted, and after the finger lifts the pointer goes back off-screen. */

int touchactive = 0; /* video/text.c draws the cursor only while this is set */

static SDL_FingerID touchowner = -1; /* finger driving the pointer, -1 = none */
static int touchblocked        = 0;  /* owner left with others down: wait for a clean panel */
static float touchpos[2];            /* last position of the owner, 0..1 window coords */
static int touchclick = 0;           /* tap released this frame: one frame of mouse.lmb */

/* The front panel is the only touch device SDL registers as DIRECT on Vita: the
   driver adds "Front" as SDL_TOUCH_DEVICE_DIRECT and "Back" as
   SDL_TOUCH_DEVICE_INDIRECT_ABSOLUTE.  Looking for a DIRECT device therefore
   finds the screen and keeps the rear pad out of the picture by construction. */
static SDL_TouchID touchpanel(void)
{
  int count, numdevices = SDL_GetNumTouchDevices();

  for (count = 0; count < numdevices; count++)
  {
    SDL_TouchID id = SDL_GetTouchDevice(count);

    if (id != 0 && SDL_GetTouchDeviceType(id) == SDL_TOUCH_DEVICE_DIRECT)
      return id;
  }

  return 0;
}

/* The finger with this id, or NULL once it has been lifted. */
static SDL_Finger *touchfinger(SDL_TouchID panel, SDL_FingerID id)
{
  int count, numfingers = SDL_GetNumTouchFingers(panel);

  for (count = 0; count < numfingers; count++)
  {
    SDL_Finger *finger = SDL_GetTouchFinger(panel, count);

    if (finger && finger->id == id)
      return finger;
  }

  return NULL;
}

/* SDL reports touches as fractions of the window.  Those have to be mapped back
   into the 640x480 design space that the menus and their hit tests use, i.e.
   the inverse of setuptextdisplay()'s projection: the HUD viewport is
   (hudareax, hudareay, hudareaw, hudareah) and the frustum is widened by
   hudfrustumscale, so 640 design units are 640*hudfrustumscale wide on screen
   (that is how expand mode centres the 4:3 strip).  Both are identity cases
   when the HUD covers a 4:3 window, and both account for the black bars of
   keep-aspect mode. */
static void touchdesignpos(const float *pos, int *x, int *y)
{
  float winx = pos[0] * (float)windowinfo.resolutionx;
  float winy = pos[1] * (float)windowinfo.resolutiony;

  if (hudareaw <= 0 || hudareah <= 0)
  {
    *x = -1;
    *y = -1;
    return;
  }

  *x = (int)(320.0f
             + 320.0f * hudfrustumscale
                   * ((winx - (float)hudareax) * 2.0f / (float)hudareaw - 1.0f));
  *y = (int)((winy - (float)hudareay) * 480.0f / (float)hudareah);
}

/* Reads the front panel once per frame and updates the pointer state.  Called
   from checkmouse() after the regular mouse read, so it can simply override the
   phantom position SDL reports when nothing is touched.

   Rules (all in one place, since polling means no event can get lost):
   - The first finger down owns the pointer; every other finger is ignored
     while it stays down.
   - The click happens when the owner is released, at its last known position,
     for exactly one frame.
   - If the owner lifts while another finger is still down, the gesture ends
     without a click and no new gesture starts until the panel is empty.
   - With several fingers landing in the same frame SDL gives no arrival
     order, so the lowest index in the finger list wins. */
static void updatetouch(void)
{
  SDL_TouchID panel;
  SDL_Finger *finger;
  int numfingers;

  touchclick  = 0;
  touchactive = 0;

  if (windowinfo.minimized)
  {
    /* The window lost focus (PS button, LiveArea, the web browser): whatever was
       down is gone as far as we know, so drop the gesture without turning it
       into a click.  The pause menu is up when focus returns; a fresh tap starts
       a fresh gesture. */
    touchowner   = -1;
    touchblocked = 0;
    return;
  }

  panel = touchpanel();
  if (panel == 0)
  {
    touchowner   = -1;
    touchblocked = 0;
    return;
  }

  numfingers = SDL_GetNumTouchFingers(panel);

  if (touchowner >= 0)
  {
    finger = touchfinger(panel, touchowner);
    if (finger)
    {
      touchpos[0] = finger->x;
      touchpos[1] = finger->y;
      touchactive = 1;
      return;
    }

    /* The owner lifted; its last position was recorded on the previous frame,
       and that is where the click lands. */
    touchowner = -1;

    if (numfingers > 0)
    {
      /* Somebody else is still holding the screen: no click, and nothing else
         may take over the pointer until every finger has been lifted. */
      touchblocked = 1;
      return;
    }

    if (!touchblocked)
      touchclick = 1;
    touchblocked = 0;
    return;
  }

  if (touchblocked)
  {
    if (numfingers == 0)
      touchblocked = 0;
    return;
  }

  if (numfingers == 0)
    return;

  /* A new gesture starts. */
  finger = SDL_GetTouchFinger(panel, 0);
  if (!finger)
    return;

  touchowner   = finger->id;
  touchpos[0]  = finger->x;
  touchpos[1]  = finger->y;
  touchactive  = 1;
}
#endif /* __vita__ */

void checkmouse(void)
{
  Uint8 buttons;

  memcpy(&prevmouse, &mouse, sizeof(mouse));
  // buttons=SDL_GetRelativeMouseState(&mouse.xdif,&mouse.ydif);
  // SDL_PumpEvents();
  buttons = SDL_GetMouseState(&mouse.x, &mouse.y);
  mouse.x = mouse.x * 640 / windowinfo.resolutionx;
  mouse.y = mouse.y * 480 / windowinfo.resolutiony;
#ifdef __vita__
  updatetouch();

  if (touchactive || touchclick)
    touchdesignpos(touchpos, &mouse.x, &mouse.y);
  else if (buttons == 0)
  {
    /* No physical mouse on Vita: with nothing touched, SDL_GetMouseState leaves the phantom
       cursor parked at (0,0) - exactly inside the "Back" item (top-left of every menu).
       That made Back permanently render as hovered/selected. Park it off-screen instead.
       (A real mouse - PS TV with USB - keeps working, because holding a button pins the
       position regardless of the touch panel.) */
    mouse.x = -1;
    mouse.y = -1;
  }
#endif

  mouse.xdif = mouse.x - prevmouse.x;
  mouse.ydif = mouse.y - prevmouse.y;

  if (buttons & SDL_BUTTON(1))
    mouse.lmb = 1;
  else
    mouse.lmb = 0;

#ifdef __vita__
  /* A released finger is the only left-click the Vita has. */
  if (touchclick)
    mouse.lmb = 1;
#endif

  if (buttons & SDL_BUTTON(3))
    mouse.rmb = 1;
  else
    mouse.rmb = 0;

  if (mouse.lmb == 0 && prevmouse.lmb == 1)
    mouse.lmbr = 1;
  else
    mouse.lmbr = 0;

  if (mouse.rmb == 0 && prevmouse.rmb == 1)
    mouse.rmbr = 1;
  else
    mouse.rmbr = 0;
}

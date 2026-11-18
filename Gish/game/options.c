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

#include "../game/options.h"

#include "../config.h"
#include "../game/config.h"
#include "../game/english.h"
#include "../game/game.h"
#include "../input/joystick.h"
#include "../input/keyboard.h"
#include "../input/mouse.h"
#include "../menu/menu.h"
#include "../sdl/event.h"
#include "../sdl/video.h"
#include "../video/opengl.h"
#include "../video/text.h"
#include "../video/texture.h"

_option option;
_control control[CONTROLS_LENGTH];

/* Vita port: draw one bindable row's value.  The row the pad (or the mouse)
   has selected is wrapped in square brackets, and a row waiting for a button
   shows a bracketed "?" - the same value slot, so nothing shifts when the
   selection moves or a rebind starts.  This is the only cue these rows can
   have: checkmenuitems() cannot brighten them, because their menu label is
   blank and their value is drawn here, outside drawmenuitems().

   item is the menuitem index that owns this row's highlight/active state.
   Brackets are drawn at the fixed brightness of the value itself and grow
   outwards: "[" one cell before the value, "]" one cell past its end, so the
   two-cell L1/R1 glyphs and the "Button 12" fallback stay enclosed.  "none"
   is what to show when nothing is bound (TXT_AXIS or TXT_NONE).

   highlight 0 marks a row that cannot be used at all - its player has no pad
   assigned.  drawmenuitems() never touches this value, so it is dimmed here to
   the same 25% a disabled menu item gets. */
static void drawbindlabel(int button, int item, int x, int y, int textsize, char *none)
{
  char text[64];
  int len;
  float brightness;

  /* snprintf returns the written length, which is what the right bracket
     needs; strlen() is not used in this file. */
  if (menuitem[item].active)
    len = snprintf(text, sizeof(text), "?");
  else if (button >= 0 && button <= 11)
    len = snprintf(text, sizeof(text), "%s", joylabel(button));
  else if (button > 11)
    len = snprintf(text, sizeof(text), TXT_BUTTON " %d", button + 1);
  else
    len = snprintf(text, sizeof(text), "%s", none);

  if (menuitem[item].active || menuitem[item].highlight >= 2)
  {
    drawtext("[", x - textsize, y, textsize, 1.0f, 1.0f, 1.0f, 1.0f);
    drawtext("]", x + len * textsize, y, textsize, 1.0f, 1.0f, 1.0f, 1.0f);
  }

  brightness = menuitem[item].highlight == 0 ? 0.25f : 1.0f;
  drawtext(text, x, y, textsize, brightness, brightness, brightness, 1.0f);
}

/* Vita port: the volume rows.  The original Options screen had a drag-only bar
   under each Sound/Music toggle, which a pad cannot operate, so each row now
   reads "- N% +": two ordinary menu items either side of a plain text readout.
   The buttons are MO_FUNCTION items, so checkmenuitems() calls them and clears
   `active` for us (see the loop at the bottom of that function); the number is
   only ever drawn, never an item, so it cannot be selected, clicked or armed.

   Stepping works from the rounded percentage instead of the float: 0.05f is
   not representable exactly, so adding it repeatedly would drift away from the
   value config.txt stores (that is (int)(volume * 100)).  Reading the
   percentage, stepping it as an int and converting back keeps every readout an
   exact multiple of 5; a value that started elsewhere (config.txt can say 37)
   lands on the neighbouring multiple on the first press. */
static void adjustvolume(float *volume, int step)
{
  int percent = (int)(*volume * 100.0f + 0.5f) + step;

  if (percent < 0)
    percent = 0;
  if (percent > 100)
    percent = 100;

  *volume = (float)percent / 100.0f;
}

static void soundvolumedown(void)
{
  adjustvolume(&option.soundvolume, -5);
}

static void soundvolumeup(void)
{
  adjustvolume(&option.soundvolume, 5);
}

static void musicvolumedown(void)
{
  adjustvolume(&option.musicvolume, -5);
}

static void musicvolumeup(void)
{
  adjustvolume(&option.musicvolume, 5);
}

/* The readout between the two buttons, centred in the gap they leave.  x is
   the column's left edge (0 for Sound, 320 for Music), so the readout sits at
   the same offset the items around it use: "-" ends at x + 176, the centre is
   x + 216 and "+" starts at x + 256.  Centring rather than left-aligning keeps
   the two gaps equal whatever the digit count ("100%" is four glyphs, "5%" is
   two).  brightness matches what drawmenuitems() does with an item's colours -
   (highlight + 1) / 4 - so a disabled row's readout dims with its buttons:
   0.25 for disabled, 1.0 for normal.  It is a plain string, so there is
   nothing to dim automatically. */
static void drawvolume(float volume, int x, int y, float brightness)
{
  char text[8];

  sprintf(text, "%d%%", (int)(volume * 100.0f + 0.5f));
  drawtext(text, (x + 216) | TEXT_CENTER, y, 16, brightness, brightness, brightness, 1.0f);
}

/* Vita port: the pad buttons that may never be bound from inside the game -
   d-pad down/left/up/right (6-9), Select (10) and Start (11).  The d-pad and
   Start drive menu navigation (checkmenuitems() moves with buttons 2/3 and
   activates with button 8), so binding them could leave the console with no
   way back into Options; Select is the rebind cancel key.  While a row is
   waiting for a button these presses are ignored by the rebind scans, but
   config.txt can still assign them - the game keeps reading whatever is
   stored there (player 1's Start, for example, is fixed at 11). */
static int reservedbutton(int button)
{
  return button >= 6 && button <= 11;
}

/* Vita port: label for a pad slot (control[].joysticknum).  Player 1's slot is
   drawn straight to the screen instead of being a menu item (see optionsmenu),
   so the text has to be selectable without going through createmenuitem(). */
static char *joynumlabel(int joysticknum)
{
  switch (joysticknum)
  {
  case 0:
    return TXT_JOY1;
  case 1:
    return TXT_JOY2;
  case 2:
    return TXT_JOY3;
  case 3:
    return TXT_JOY4;
  }

  return TXT_NONE;
}

/* Vita port: the port-only settings at the bottom of the hub are plain labels
   with one value item each.  The label is drawn by drawtext() and is not a menu
   item, so the row is a single navigation stop and the only thing a click or
   Start can hit is the value slot on the right - the same shape as the 1.53
   videooptionsmenu() rows.  The item's MO_FUNCTION handler cycles the setting
   and the label it shows is rebuilt from the new value on the next frame. */

static char *aspectlabel(void)
{
  switch (config.keepaspect)
  {
  case 0:
    return TXT_ASPECT_STRETCHED;
  case 2:
    return TXT_ASPECT_FULL;
  }

  return TXT_ASPECT_ORIGINAL;
}

/* Aspect Ratio cycles keepaspect 0 -> 1 -> 2 (Stretched, Original 4:3, Full).
   setupviewport() recomputes the render and HUD areas, so the change is visible
   without leaving the screen - main.c calls the same function at startup. */
static void aspectratiochange(void)
{
  config.keepaspect++;

  if (config.keepaspect > 2)
    config.keepaspect = 0;

  setupviewport();
}

/* Lighting and Shadows are copied into these two globals at boot (main.c), and
   the renderer reads the globals rather than config.txt, so a toggle has to
   update both.  Shadows keep their value while Lighting is Off - the row is
   merely disabled (see the MO_HIGHLIGHT 0 in optionsmenu), so switching
   Lighting back on restores the previous Shadows setting. */
static void lightingchange(void)
{
  config.lighting ^= 1;
  lighting_enabled = config.lighting;
}

static void shadowschange(void)
{
  config.shadows ^= 1;
  shadow_enabled = config.shadows;
}

/* MSAA is baked into the EGL config before the window exists, so this only
   changes what the next launch asks for - hence the hint under the row. */
static void msaachange(void)
{
  config.msaa ^= 1;
}

static void interpolationchange(void)
{
  config.frameinterpolation ^= 1;
}

static void limiterchange(void)
{
  config.fpslimiter ^= 1;
}

static void warpzonepixelationchange(void)
{
  config.warpzonepixelation ^= 1;
}

/* game.turbomode is the live copy mainmenu() normally sets from config; set it
   here too so the change takes effect without a trip through the main menu. */
static void turbomodechange(void)
{
  config.turbomode ^= 1;
  game.turbomode = config.turbomode;
}

/* Vita port: the Controls item cannot open the submenu directly - controlsmenu()
   runs a loop of its own, and calling it from inside checkmenuitems() would nest
   two input and draw passes in one frame.  Its handler only raises a flag;
   optionsmenu() opens the screen after the frame has been presented. */
static int requestcontrols = 0;

static void controlsmenuopen(void)
{
  requestcontrols = 1;
}

void optionsmenu(void)
{
  if (touchcontrols)
  {
    pleasedisabletouchcontrols();
  }
  else
  {
    int prevjoynum;

    /* Vita port: this screen is pad-navigable, and it is a hub now.  The top
       row is Save + Controls, the Sound/Music pair follows, and the rest of the
       page holds the port-only settings that used to be config.txt-only
       (aspect ratio, lighting, shadows, MSAA, interpolation, limiter, Warp Zone
       pixelation, turbo mode).  The keyboard/pad bindings moved to the Controls
       screen.  Down/Up move, Start activates - the same path every other menu
       uses.  Touch still works, because the hit test in checkmenuitems() runs
       either way. */
    joystickmenu = 1;

    resetmenuitems();

    while (!menuitem[0].active && !windowinfo.shutdown)
    {
      glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
      glClear(GL_COLOR_BUFFER_BIT);

      numofmenuitems = 0;
      /* Vita port: "Back" is called "Save" now, because leaving this screen
         writes config.txt (see the saveconfig() call after the loop) - the
         old Exit-item-only save was never reached when the app was closed
         from the LiveArea.  ESC still closes the screen; the label has no
         hotkey letter because ESC has no letter to highlight. */
      createmenuitem(TXT_SAVE, 0, 0, 16, 1.0f, 1.0f, 1.0f, 1.0f);
      setmenuitem(MO_HOTKEY, SCAN_ESC);

      /* Bindings and pad assignment live on their own screen now. */
      createmenuitem(TXT_CONTROLS, 160, 0, 16, 1.0f, 1.0f, 1.0f, 1.0f);
      setmenuitem(MO_FUNCTION, controlsmenuopen);

      /* Vita port: the "- N% +" volume rows (see adjustvolume()).  Both groups
         are laid out identically and exactly 320 apart: the toggle at x, "-"
         at x + 160, the readout centred at x + 216 and "+" at x + 256.  Sound
         is the left column (x = 0) and Music the right one (x = 320) on the
         same line, so the two read as a pair.

         All three items of a group are created every frame; only their state
         changes.  A group whose toggle is off stays on screen as a dimmed,
         unusable row, so the item order is fixed.  Disabling is MO_HIGHLIGHT
         0: checkmenuitems() then skips the item entirely (no hit test, no
         hotkey, no activation) and drawmenuitems() draws it at 25% brightness
         instead of the usual 50%. */
      if (option.sound)
      {
        createmenuitem(TXT_SOUND_ON, 0, 32, 16, 1.0f, 1.0f, 1.0f, 1.0f);
        setmenuitem(MO_SET, &option.sound, 0);
      }
      else
      {
        createmenuitem(TXT_SOUND_OFF, 0, 32, 16, 1.0f, 1.0f, 1.0f, 1.0f);
        setmenuitem(MO_SET, &option.sound, 1);
      }
      /* Sound Off is a master mute (stock 1.53), so its own volume has nothing
         to act on: both buttons are disabled.  The toggle above stays live -
         it is the only way back on. */
      createmenuitem("-", 160, 32, 16, 1.0f, 1.0f, 1.0f, 1.0f);
      setmenuitem(MO_FUNCTION, soundvolumedown);
      if (!option.sound)
        setmenuitem(MO_HIGHLIGHT, 0);
      createmenuitem("+", 256, 32, 16, 1.0f, 1.0f, 1.0f, 1.0f);
      setmenuitem(MO_FUNCTION, soundvolumeup);
      if (!option.sound)
        setmenuitem(MO_HIGHLIGHT, 0);

      if (option.music)
      {
        createmenuitem(TXT_MUSIC_ON, 320, 32, 16, 1.0f, 1.0f, 1.0f, 1.0f);
        setmenuitem(MO_SET, &option.music, 0);
      }
      else
      {
        createmenuitem(TXT_MUSIC_OFF, 320, 32, 16, 1.0f, 1.0f, 1.0f, 1.0f);
        setmenuitem(MO_SET, &option.music, 1);
      }
      /* The Music toggle is disabled only by Sound Off (master mute), never by
         Music being off: that toggle is the way back on.  The Music group is
         disabled by either - Music off means there is nothing to adjust, Sound
         off means Music does nothing at all. */
      if (!option.sound)
        setmenuitem(MO_HIGHLIGHT, 0);
      createmenuitem("-", 480, 32, 16, 1.0f, 1.0f, 1.0f, 1.0f);
      setmenuitem(MO_FUNCTION, musicvolumedown);
      if (!option.music || !option.sound)
        setmenuitem(MO_HIGHLIGHT, 0);
      createmenuitem("+", 576, 32, 16, 1.0f, 1.0f, 1.0f, 1.0f);
      setmenuitem(MO_FUNCTION, musicvolumeup);
      if (!option.music || !option.sound)
        setmenuitem(MO_HIGHLIGHT, 0);

      /* The port options.  The mockup's vertical rhythm uses 16 units for a menu
         row and 13 for a hint row, and every run of hint lines is followed by
         an empty row of the same size, so the hint blocks are not flush against
         the next label.  That is why the y values below are neither multiples
         of 16 nor evenly spaced. */
      createmenuitem(aspectlabel(), 320, 80, 16, 1.0f, 1.0f, 1.0f, 1.0f);
      setmenuitem(MO_FUNCTION, aspectratiochange);

      createmenuitem(config.lighting ? TXT_ON : TXT_OFF, 160, 148, 16, 1.0f, 1.0f, 1.0f, 1.0f);
      setmenuitem(MO_FUNCTION, lightingchange);

      /* Shadows keep their value while Lighting is off, but cannot be changed:
         MO_HIGHLIGHT 0 makes checkmenuitems() skip the item and
         drawmenuitems() draws it at 25%, matching the dimmed label below. */
      createmenuitem(config.shadows ? TXT_ON : TXT_OFF, 480, 148, 16, 1.0f, 1.0f, 1.0f, 1.0f);
      setmenuitem(MO_FUNCTION, shadowschange);
      if (!config.lighting)
        setmenuitem(MO_HIGHLIGHT, 0);

      createmenuitem(config.msaa ? TXT_ON : TXT_OFF, 320, 190, 16, 1.0f, 1.0f, 1.0f, 1.0f);
      setmenuitem(MO_FUNCTION, msaachange);

      createmenuitem(config.frameinterpolation ? TXT_ON : TXT_OFF, 320, 232, 16, 1.0f, 1.0f, 1.0f, 1.0f);
      setmenuitem(MO_FUNCTION, interpolationchange);

      createmenuitem(config.fpslimiter ? TXT_ON : TXT_OFF, 320, 248, 16, 1.0f, 1.0f, 1.0f, 1.0f);
      setmenuitem(MO_FUNCTION, limiterchange);

      createmenuitem(config.warpzonepixelation ? TXT_ON : TXT_OFF, 320, 329, 16, 1.0f, 1.0f, 1.0f, 1.0f);
      setmenuitem(MO_FUNCTION, warpzonepixelationchange);

      createmenuitem(config.turbomode ? TXT_ON : TXT_OFF, 320, 403, 16, 1.0f, 1.0f, 1.0f, 1.0f);
      setmenuitem(MO_FUNCTION, turbomodechange);

      checksystemmessages();
      checkkeyboard();
      checkmouse();
      checkjoystick();
      checkmenuitems();

      setuptextdisplay();

      /* Row labels and hints.  The labels are not items, so they are drawn at
         the same 75% the bind-row names use; the hints use the 50% grey of the
         pad name on the Controls screen. */
      drawtext(TXT_ASPECT, 0, 80, 16, 0.75f, 0.75f, 0.75f, 1.0f);
      drawtext(TXT_HINT_ASPECT1, 0, 96, 10, 0.5f, 0.5f, 0.5f, 1.0f);
      drawtext(TXT_HINT_ASPECT2, 0, 109, 10, 0.5f, 0.5f, 0.5f, 1.0f);
      drawtext(TXT_HINT_ASPECT3, 0, 122, 10, 0.5f, 0.5f, 0.5f, 1.0f);

      drawtext(TXT_LIGHTING, 0, 148, 16, 0.75f, 0.75f, 0.75f, 1.0f);
      if (config.lighting)
        drawtext(TXT_SHADOWS, 320, 148, 16, 0.75f, 0.75f, 0.75f, 1.0f);
      else
        drawtext(TXT_SHADOWS, 320, 148, 16, 0.25f, 0.25f, 0.25f, 1.0f);
      drawtext(TXT_HINT_LIGHTING, 0, 164, 10, 0.5f, 0.5f, 0.5f, 1.0f);

      drawtext(TXT_ANTIALIASING, 0, 190, 16, 0.75f, 0.75f, 0.75f, 1.0f);
      drawtext(TXT_HINT_MSAA, 0, 206, 10, 0.5f, 0.5f, 0.5f, 1.0f);

      drawtext(TXT_FRAMEINTERPOLATION, 0, 232, 16, 0.75f, 0.75f, 0.75f, 1.0f);
      drawtext(TXT_FPSLIMITER, 0, 248, 16, 0.75f, 0.75f, 0.75f, 1.0f);
      drawtext(TXT_HINT_INTERPOLATION1, 0, 264, 10, 0.5f, 0.5f, 0.5f, 1.0f);
      drawtext(TXT_HINT_INTERPOLATION2, 0, 277, 10, 0.5f, 0.5f, 0.5f, 1.0f);
      drawtext(TXT_HINT_LIMITER1, 0, 290, 10, 0.5f, 0.5f, 0.5f, 1.0f);
      drawtext(TXT_HINT_LIMITER2, 0, 303, 10, 0.5f, 0.5f, 0.5f, 1.0f);

      drawtext(TXT_WARPZONE_PIXELATION, 0, 329, 16, 0.75f, 0.75f, 0.75f, 1.0f);
      drawtext(TXT_HINT_WARP1, 0, 345, 10, 0.5f, 0.5f, 0.5f, 1.0f);
      drawtext(TXT_HINT_WARP2, 0, 358, 10, 0.5f, 0.5f, 0.5f, 1.0f);

      drawtext(TXT_TURBO_EDITION, 0, 403, 16, 0.75f, 0.75f, 0.75f, 1.0f);
      drawtext(TXT_HINT_TURBO1, 0, 419, 10, 0.5f, 0.5f, 0.5f, 1.0f);
      drawtext(TXT_HINT_TURBO2, 0, 432, 10, 0.5f, 0.5f, 0.5f, 1.0f);

      /* Vita port: the percent readouts of the two volume rows.  Drawn here,
         not as menu items, so the number cannot be selected or clicked - only
         the "-" and "+" either side of it can.  Brightness mirrors
         drawmenuitems()'s dimming - 1.0 for a usable row, 0.25 for a disabled
         one - under exactly the same conditions the items use. */
      drawvolume(option.soundvolume, 0, 32, option.sound ? 1.0f : 0.25f);
      drawvolume(option.musicvolume, 320, 32, (option.music && option.sound) ? 1.0f : 0.25f);

      drawmenuitems();

      drawmousecursor(768 + font.cursornum, mouse.x, mouse.y, 16, 1.0f, 1.0f, 1.0f, 1.0f);

#if defined(PC_GLES)
      eglSwapBuffers(eglDisplay, eglSurface);
#else
      SDL_GL_SwapWindow(globalwindow);
#endif

      /* Opened after the frame is presented, so the press that activated the
         Controls item is already an edge in the past and cannot be read as
         input by the new screen.  The hub's selection is restored afterwards
         so Back lands on Controls, not on Save. */
      if (requestcontrols)
      {
        requestcontrols = 0;

        prevjoynum = joymenunum;
        controlsmenu();
        joymenunum = prevjoynum;
      }
    }

    resetmenuitems();

    joystickmenu = 1;

    /* Vita port: write config.txt as soon as the player backs out of Options.
       The only other saves are the one at boot (main.c, after loadconfig) and
       the one after mainmenu() returns, i.e. after the Exit item - and the
       Vita has no Exit item worth trusting: closing the application from the
       LiveArea kills the process without reaching that code, so every volume
       change, pad assignment, rebind and option change made here was silently
       lost.  The file is small and saveconfig() rewrites it from memory, so a
       write per Save is cheap.  This is also reached when windowinfo.shutdown
       ends the loop.  The Controls screen needs nothing of its own: it returns
       here, and the Save that leaves this screen writes its assignments and
       bindings as well. */
    saveconfig();
  }
}

/* The Vita port does not offer this screen (optionsmenu() no longer creates the
   menu item), because its colour depth switch and Apply button can leave the
   GLES surface in a state the game cannot recover from.  Kept so the screen is
   still here if a safe subset is ever wanted on this console. */
void videooptionsmenu(void)
{
  int count, count2;
  int prevvideomodenum;
  int videomodenum;
  char restext[64];
  int fullscreen;
  int bitsperpixel;
  char *glvendor;
  char *glrenderer;
  char *glversion;

  glvendor   = (char *)glGetString(GL_VENDOR);
  glrenderer = (char *)glGetString(GL_RENDERER);
  glversion  = (char *)glGetString(GL_VERSION);

  videomodenum = -1;
  for (count = 0; count < numofsdlvideomodes; count++)
    if (sdlvideomode[count].bitsperpixel == 32)
    {
      if (windowinfo.resolutionx == sdlvideomode[count].resolutionx)
        if (windowinfo.resolutiony == sdlvideomode[count].resolutiony)
          videomodenum = count;
    }
  prevvideomodenum = videomodenum;
  fullscreen       = windowinfo.fullscreen;
  bitsperpixel     = windowinfo.bitsperpixel;

  resetmenuitems();

  while (!menuitem[0].active && !menuitem[1].active && !windowinfo.shutdown)
  {
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    numofmenuitems = 0;
    createmenuitem(TXT_BACK, 0, 0, 16, 1.0f, 1.0f, 1.0f, 1.0f);
    setmenuitem(MO_HOTKEY, SCAN_ESC);
    createmenuitem(TXT_APPLY, (640 | TEXT_END), 0, 16, 1.0f, 1.0f, 1.0f, 1.0f);
    setmenuitem(MO_HOTKEY, SCAN_A);

    count2 = 64;
    for (count = numofsdlvideomodes - 1; count >= 0; count--)
      if (sdlvideomode[count].resolutionx >= 640)
        if (sdlvideomode[count].bitsperpixel == 32)
        {
          sprintf(restext, "%dx%d", sdlvideomode[count].resolutionx, sdlvideomode[count].resolutiony);
          createmenuitem(restext, 0, count2, 16, 1.0f, 1.0f, 1.0f, 1.0f);
          setmenuitem(MO_SET, &videomodenum, count);

          count2 += 16;
        }

    count = 48;
    createmenuitem(TXT_FULLSCREEN, 320, count, 16, 1.0f, 1.0f, 1.0f, 1.0f);
    setmenuitem(MO_TOGGLE, &fullscreen);
    setmenuitem(MO_HOTKEY, SCAN_F);
    count += 16;
    count += 16;
    count += 16;
    createmenuitem(TXT_16BIT, 320, count, 16, 1.0f, 1.0f, 1.0f, 1.0f);
    setmenuitem(MO_SET, &bitsperpixel, 16);
    count += 16;
    createmenuitem(TXT_32BIT, 320, count, 16, 1.0f, 1.0f, 1.0f, 1.0f);
    setmenuitem(MO_SET, &bitsperpixel, 32);
    count += 16;

    checksystemmessages();
    checkkeyboard();
    checkmouse();
    checkjoystick();
    checkmenuitems();

    setuptextdisplay();

    drawtext(TXT_RESOLUTION, 0, 48, 16, 1.0f, 1.0f, 1.0f, 1.0f);
    drawtext(TXT_COLOR, 320, 80, 16, 1.0f, 1.0f, 1.0f, 1.0f);

    count = 400;
    drawtext(TXT_OPENGLINFO, 0, count, 16, 1.0f, 1.0f, 1.0f, 1.0f);
    count += 16;
    if (glext.multitexture)
      drawtext("GL_ARB_multitexture", 0, count, 12, 0.0f, 1.0f, 0.0f, 1.0f);
    else
      drawtext("GL_ARB_multitexture", 0, count, 12, 0.25f, 0.25f, 0.25f, 1.0f);
    count += 12;
    if (glext.texture_env_dot3)
      drawtext("GL_ARB_texture_env_dot3", 0, count, 12, 0.0f, 1.0f, 0.0f, 1.0f);
    else
      drawtext("GL_ARB_texture_env_dot3", 0, count, 12, 0.25f, 0.25f, 0.25f, 1.0f);
    count += 12;
    drawtext("GL_VERSION: /s", 0, count, 12, 1.0f, 1.0f, 1.0f, 1.0f, glversion);
    count += 12;
    drawtext("GL_VENDOR: /s", 0, count, 12, 1.0f, 1.0f, 1.0f, 1.0f, glvendor);
    count += 12;
    drawtext("GL_RENDERER: /s", 0, count, 12, 1.0f, 1.0f, 1.0f, 1.0f, glrenderer);
    count += 12;

    drawmenuitems();

    drawmousecursor(768 + font.cursornum, mouse.x, mouse.y, 16, 1.0f, 1.0f, 1.0f, 1.0f);

#if defined(PC_GLES)
    eglSwapBuffers(eglDisplay, eglSurface);
#else
    SDL_GL_SwapWindow(globalwindow);
#endif
  }

  if (menuitem[1].active)
  {
    if (prevvideomodenum == videomodenum)
      if (windowinfo.fullscreen == fullscreen)
        if (windowinfo.bitsperpixel == bitsperpixel)
          return;

    if (videomodenum == -1)
      return;

    windowinfo.resolutionx  = sdlvideomode[videomodenum].resolutionx;
    windowinfo.resolutiony  = sdlvideomode[videomodenum].resolutiony;
    windowinfo.fullscreen   = fullscreen;
    windowinfo.bitsperpixel = bitsperpixel;

#if !defined(GLES)
    if (windowinfo.bitsperpixel == 16)
    {
      SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 5);
      SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 6);
      SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 5);
      SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 0);
    }
    if (windowinfo.bitsperpixel == 32)
    {
      SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
      SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
      SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
      SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
    }
#endif

    TO_DEBUG_LOG("options.c Opening screen %dx%dx%d\n", windowinfo.resolutionx, windowinfo.resolutiony,
                 windowinfo.bitsperpixel);

    SDL_DisplayMode mode = {(windowinfo.bitsperpixel == 32) ? SDL_PIXELFORMAT_RGB888 : SDL_PIXELFORMAT_RGB565,
                            windowinfo.resolutionx, windowinfo.resolutiony, 0, 0};
    SDL_SetWindowDisplayMode(globalwindow, &mode);
    SDL_SetWindowSize(globalwindow, windowinfo.resolutionx, windowinfo.resolutiony);
    (windowinfo.fullscreen) ? SDL_SetWindowFullscreen(globalwindow, SDL_WINDOW_FULLSCREEN)
                            : SDL_SetWindowFullscreen(globalwindow, 0);

    for (count = 0; count < 2048; count++)
      if (texture[count].sizex != 0)
        setuptexture(count);
  }

  resetmenuitems();
}

/* Vita port: dead code since the volume bars became "- N% +" rows (see
   adjustvolume() in this file).  Nothing calls this any more - the call site
   in optionsmenu() is commented out.  Kept because it is the only remaining
   description of the original bar geometry, and it costs nothing to build. */
/* Vita port: the Controls screen.  All four players are on one page - the old
   separate 4 Player screen is gone, because a second screen for the same job
   was more confusing than four blocks on one page.  Per player: the pad slot,
   the pad's product name (size 10), and the four rebindable bindings (Stick,
   Jump, Slide, Heavy).  The d-pad and Start rows are not shown at all: they are
   fixed by the port and cannot be rebound, and a row nobody can use is worse
   than an absent one.

   Player 1's slot is display-only (the built-in controller); players 2-4 cycle
   None/Joy1..Joy4.  A player without a pad has its bind rows disabled (menu
   items with highlight 0), so they cannot be armed and left showing "?" with no
   way to bind anything.

   Layout: header at y=32, then four blocks 106 units apart starting at y=61.
   Inside a block the player line is at +0, the pad name at +16, and the four
   bind rows at +29/+45/+61/+77.  The last row ends at y=472, eight units above
   the bottom edge. */

void controlsmenu(void)
{
  int count, count2, count3;
  int player, row, item;
  int block, nextslot;
  float brightness;
  unsigned char rowactive[MAXMENUITEMS];
  char *playerlabel[4] = {TXT_PLAYER1, TXT_PLAYER2, TXT_PLAYER3, TXT_PLAYER4};
  char *bindlabel[4]   = {TXT_STICK, TXT_JUMP, TXT_SLIDE, TXT_HEAVY};

  resetmenuitems();

  while (!menuitem[0].active && !windowinfo.shutdown)
  {
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    numofmenuitems = 0;
    createmenuitem(TXT_BACK, 0, 0, 16, 1.0f, 1.0f, 1.0f, 1.0f);
    setmenuitem(MO_HOTKEY, SCAN_ESC);

    /* Item order, and therefore every index below: Back 0, then per player the
       slot (players 2-4 only) followed by the four bind rows - so a player's
       first bind row is at 1 + player * 5. */
    for (player = 0; player < 4; player++)
    {
      block = 61 + player * 106;

      /* A stored slot that does not name a pad we know of is reset to None, so
         the label and the cycle below always agree with the value. */
      if (control[player].joysticknum < -1 || control[player].joysticknum > 3)
        control[player].joysticknum = -1;

      if (player > 0)
      {
        nextslot = control[player].joysticknum + 1;
        if (nextslot > 3)
          nextslot = -1;

        createmenuitem(joynumlabel(control[player].joysticknum), 320, block, 16, 1.0f, 1.0f, 1.0f, 1.0f);
        setmenuitem(MO_SET, &control[player].joysticknum, nextslot);
      }

      for (row = 0; row < 4; row++)
      {
        createmenuitem("        ", 320, block + 29 + row * 16, 16, 1.0f, 1.0f, 1.0f, 1.0f);
        if (control[player].joysticknum == -1)
          setmenuitem(MO_HIGHLIGHT, 0);
      }
    }

    checksystemmessages();

    /* Snapshot which rows were already waiting for a button.  checkmenuitems()
       arms the selected row on this very Start press, and the handler after the
       frame would otherwise see the same press as the new binding: Start on
       "Stick" would bind Start and close the row again, which looks exactly
       like nothing happened.  One frame later the press edge is gone, and a
       second, separate press binds normally - including Start. */
    for (count = 0; count < numofmenuitems; count++)
      rowactive[count] = (unsigned char)menuitem[count].active;

    /* While any row is waiting for a button, switch pad navigation off for this
       frame.  Otherwise a row could be armed, the d-pad could move to a second
       row and Start could arm that one too: two rows would sit active, and the
       next bindable press would bind one of them while the duplicate rule left
       the other empty.  Only navigation is gated: the mouse hit test, the
       hotkeys and Select (read straight from joystick[] below) keep working,
       which is how a row is cancelled or resolved.  Re-armed every frame from
       the snapshot, so it turns itself back on the moment the last row is
       resolved. */
    joystickmenu = 1;
    for (count = 0; count < numofmenuitems; count++)
      if (rowactive[count])
        joystickmenu = 0;

    checkkeyboard();
    checkmouse();
    checkjoystick();
    checkmenuitems();

    for (player = 0; player < 4; player++)
      if (control[player].joysticknum >= numofjoysticks)
        control[player].joysticknum = -1;

    setuptextdisplay();

    drawtext(TXT_PLAYER, 0, 32, 16, 1.0f, 1.0f, 1.0f, 1.0f);
    drawtext(TXT_CONTROLLER, 320, 32, 16, 1.0f, 1.0f, 1.0f, 1.0f);

    for (player = 0; player < 4; player++)
    {
      block = 61 + player * 106;

      drawtext(playerlabel[player], 0, block, 16, 1.0f, 1.0f, 1.0f, 1.0f);

      /* Player 1's slot stays display-only and is drawn here (see joynumlabel):
         it used to be a cycling item, which let a stray tap set it to "None"
         and leave the console with no pad at all.  Players 2-4 draw their slot
         through drawmenuitems() as a normal value item. */
      if (player == 0)
        drawtext(joynumlabel(control[player].joysticknum), 320, block, 16, 0.5f, 0.5f, 0.5f, 1.0f);

      if (control[player].joysticknum != -1)
        drawtext(joystick[control[player].joysticknum].name, 320, block + 16, 10, 0.5f, 0.5f, 0.5f, 1.0f);
      else
        drawtext(TXT_NO_CONTROLLER, 320, block + 16, 10, 0.5f, 0.5f, 0.5f, 1.0f);

      brightness = control[player].joysticknum == -1 ? 0.25f : 0.75f;

      for (row = 0; row < 4; row++)
      {
        drawtext(bindlabel[row], 0, block + 29 + row * 16, 16, brightness, brightness, brightness, 1.0f);
        drawbindlabel(control[player].button[4 + row], 1 + player * 5 + row,
                      320, block + 29 + row * 16, 16, TXT_NONE);
      }
    }

    drawmenuitems();

    drawmousecursor(768 + font.cursornum, mouse.x, mouse.y, 16, 1.0f, 1.0f, 1.0f, 1.0f);

#if defined(PC_GLES)
    eglSwapBuffers(eglDisplay, eglSurface);
#else
    SDL_GL_SwapWindow(globalwindow);
#endif

    /* Select cancels an armed rebind row, exactly like a left click clears
       every item in checkmenuitems().  It runs before the rebind handlers, so
       the press that cancels is never written into a row, and outside the
       per-player blocks, so it also works for a player whose pad is not
       assigned yet (that handler is skipped entirely).  This exists only on the
       Options screens - Select is an ordinary button everywhere else. */
    if (control[0].joysticknum != -1)
      if (joystick[control[0].joysticknum].numofbuttons > 10)
        if (joystick[control[0].joysticknum].button[10]
            && !prevjoystick[control[0].joysticknum].button[10])
          for (count = 0; count < numofmenuitems; count++)
            menuitem[count].active = 0;

    /* One rebind scan per player.  Only buttons 4-7 (Stick, Jump, Slide, Heavy)
       are rebindable - the d-pad rows and Start are fixed and not shown - so
       count stays the row offset within those four. */
    for (player = 0; player < 4; player++)
    {
      if (control[player].joysticknum == -1)
        continue;

      for (row = 0; row < 4; row++)
      {
        item = 1 + player * 5 + row;

        if (!menuitem[item].active || !rowactive[item])
          continue;

        for (count2 = 0; count2 < joystick[control[player].joysticknum].numofbuttons; count2++)
          if (!reservedbutton(count2)
              && joystick[control[player].joysticknum].button[count2]
              && !prevjoystick[control[player].joysticknum].button[count2])
          {
            control[player].button[4 + row] = count2;

            /* One button, one action: clear it from this player's other three
               rebindable rows.  The hidden d-pad and Start rows can never
               collide with a bindable button - they hold 6-9 and 11, all of
               which reservedbutton() refuses - so they are left alone. */
            for (count3 = 4; count3 < 8; count3++)
              if (count3 != 4 + row)
                if (control[player].button[count3] == count2)
                  control[player].button[count3] = -1;

            menuitem[item].active = 0;
          }

        if (keyboard[SCAN_DELETE] && !prevkeyboard[SCAN_DELETE])
        {
          control[player].button[4 + row] = -1;
          menuitem[item].active = 0;
        }
      }
    }
  }

  /* Never leave the hub with navigation switched off: on the last frame of a
     cancelled screen a row could still have been armed. */
  joystickmenu = 1;

  resetmenuitems();
}

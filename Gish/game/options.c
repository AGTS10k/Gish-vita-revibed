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

/* Vita port: draw a pad binding as the button's own glyph instead of
   "Button N".  joylabel() knows the twelve SDL Vita buttons (face, d-pad,
   L1/R1, Select/Start); anything else - a non-Vita pad with more buttons -
   falls through to the old plain number so the binding stays readable. */
static void drawjoylabel(int button, int x, int y, int textsize)
{
  if (button >= 0 && button <= 11)
    drawtext(joylabel(button), x, y, textsize, 1.0f, 1.0f, 1.0f, 1.0f);
  else
    drawtext(TXT_BUTTON " /i", x, y, textsize, 1.0f, 1.0f, 1.0f, 1.0f, button + 1);
}

/* Vita port: the same glyph, for a row that cannot be rebound - the d-pad,
   Start, and player 1's controller slot.  There is no menu item behind it, so
   it can be neither clicked nor reached by pad navigation, but it is drawn at
   full brightness like an editable cell: the user asked to drop the grey cue
   for now.  "none" is what to show when nothing is bound: TXT_AXIS on the
   movement rows (stick only), TXT_NONE on the action rows. */
static void drawfixedlabel(int button, int x, int y, int textsize, char *none)
{
  if (button >= 0 && button <= 11)
    drawtext(joylabel(button), x, y, textsize, 1.0f, 1.0f, 1.0f, 1.0f);
  else if (button > 11)
    drawtext(TXT_BUTTON " /i", x, y, textsize, 1.0f, 1.0f, 1.0f, 1.0f, button + 1);
  else
    drawtext(none, x, y, textsize, 1.0f, 1.0f, 1.0f, 1.0f);
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

void optionsmenu(void)
{
  if (touchcontrols)
  {
    pleasedisabletouchcontrols();
  }
  else
  {
    int count, count2, count3;
    /* Was each item already waiting for a button before this frame's input?
       The rebind handlers further down use this so the Start press that opens
       a row is not also read as the button to bind. */
    unsigned char rowactive[MAXMENUITEMS];

    count = 0;

    /* Vita port: this screen is pad-navigable now.  It used to force
       joystickmenu = 0 because it was mouse-only (no way in, no way out with a
       pad) and because the old two-column layout had no sensible up/down order.
       With the KEY column gone the items run in one visual column: Back, the
       four rebindable player-1 rows, the four rebindable player-2 rows,
       4 Player, the player-2 pad slot, then Sound and Music.  Down/Up move,
       Start activates - the same path every other menu uses.  Touch still
       works, because the hit test in checkmenuitems() runs either way. */
    joystickmenu = 1;

    resetmenuitems();

    while (!menuitem[0].active && !windowinfo.shutdown)
    {
      glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
      glClear(GL_COLOR_BUFFER_BIT);

      numofmenuitems = 0;
      createmenuitem(TXT_BACK, 0, 0, 16, 1.0f, 1.0f, 1.0f, 1.0f);
      setmenuitem(MO_HOTKEY, SCAN_ESC);
      /* Vita port: only the four rebindable rows (Stick, Jump, Slide, Heavy -
         buttons 4-7) get menu items here and at y=400 below.  The d-pad and
         Start rows are fixed and drawn directly by drawfixedlabel(), so they
         have no item, no hit box and no navigation stop. */
      count = 208;
      createmenuitem("        ", 320, count, 16, 1.0f, 1.0f, 1.0f, 1.0f);
      count += 16;
      createmenuitem("        ", 320, count, 16, 1.0f, 1.0f, 1.0f, 1.0f);
      count += 16;
      createmenuitem("        ", 320, count, 16, 1.0f, 1.0f, 1.0f, 1.0f);
      count += 16;
      createmenuitem("        ", 320, count, 16, 1.0f, 1.0f, 1.0f, 1.0f);
      count += 16;

      count = 400;
      createmenuitem("        ", 320, count, 16, 1.0f, 1.0f, 1.0f, 1.0f);
      count += 16;
      createmenuitem("        ", 320, count, 16, 1.0f, 1.0f, 1.0f, 1.0f);
      count += 16;
      createmenuitem("        ", 320, count, 16, 1.0f, 1.0f, 1.0f, 1.0f);
      count += 16;
      createmenuitem("        ", 320, count, 16, 1.0f, 1.0f, 1.0f, 1.0f);
      count += 16;

      /* Vita port: the Video Options screen is not offered on this console, so
         only TXT_4_PLAYER is created here and it lands at index 9 (Back plus
         the four rebindable rows of each of the two players come first) - which
         is what the handler at the bottom of this loop expects.
         The video screen switched the colour depth at runtime (16 bit) and had
         an Apply button; on the Vita that can leave the GLES surface in a state
         the game cannot recover from, and there is no video mode to choose. */
      createmenuitem(TXT_4_PLAYER, 160, 0, 16, 1.0f, 1.0f, 1.0f, 1.0f);
      setmenuitem(MO_HOTKEY, SCAN_4);

      /* Vita port: player 1 is always the built-in controller, so its pad slot
         is display-only.  It used to be a cycling MO_SET item, which let a
         stray tap (or a future pad-navigation stop) set it to "None" and leave
         the console with no pad at all - unfixable without touch.  The label
         is now drawn directly (see the draw section below), so there is no
         menu item, no hit box, and nothing to land on.  Player 2's slot stays
         a real item: assigning a second pad there is how Versus starts. */
      count = 304;

      if (control[1].joysticknum == -1)
      {
        createmenuitem(TXT_NONE, 320, count, 16, 1.0f, 1.0f, 1.0f, 1.0f);
        setmenuitem(MO_SET, &control[1].joysticknum, 0);
      }
      if (control[1].joysticknum == 0)
      {
        createmenuitem(TXT_JOY1, 320, count, 16, 1.0f, 1.0f, 1.0f, 1.0f);
        setmenuitem(MO_SET, &control[1].joysticknum, 1);
      }
      if (control[1].joysticknum == 1)
      {
        createmenuitem(TXT_JOY2, 320, count, 16, 1.0f, 1.0f, 1.0f, 1.0f);
        setmenuitem(MO_SET, &control[1].joysticknum, 2);
      }
      if (control[1].joysticknum == 2)
      {
        createmenuitem(TXT_JOY3, 320, count, 16, 1.0f, 1.0f, 1.0f, 1.0f);
        setmenuitem(MO_SET, &control[1].joysticknum, 3);
      }
      if (control[1].joysticknum == 3)
      {
        createmenuitem(TXT_JOY4, 320, count, 16, 1.0f, 1.0f, 1.0f, 1.0f);
        setmenuitem(MO_SET, &control[1].joysticknum, -1);
      }

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
      if (option.music)
      {
        createmenuitem(TXT_MUSIC_ON, 0, 48, 16, 1.0f, 1.0f, 1.0f, 1.0f);
        setmenuitem(MO_SET, &option.music, 0);
      }
      else
      {
        createmenuitem(TXT_MUSIC_OFF, 0, 48, 16, 1.0f, 1.0f, 1.0f, 1.0f);
        setmenuitem(MO_SET, &option.music, 1);
      }

      checksystemmessages();

      /* Snapshot which rows were already waiting for a button.  checkmenuitems()
         arms the selected row on this very Start press, and the handler after
         the frame would otherwise see the same press as the new binding: Start
         on "Move Left" would bind Start and close the row again, which looks
         exactly like nothing happened.  One frame later the press edge is gone,
         and a second, separate press binds normally - including Start. */
      for (count = 0; count < numofmenuitems; count++)
        rowactive[count] = (unsigned char)menuitem[count].active;

      /* Vita port: while any row is waiting for a button, switch pad
         navigation off for this frame.  Otherwise a row could be armed, the
         d-pad could move to a second row and Start could arm that one too:
         two rows would sit active, and the next bindable press would bind one
         of them while the duplicate rule left the other empty.  With
         joystickmenu at 0 checkmenuitems() skips both the d-pad movement and
         the Start activation, so no second row can be armed.  Only navigation
         is gated: the mouse hit test, the hotkeys and Select (read straight
         from joystick[] below) all keep working, which is how a row is
         cancelled or resolved.  Re-armed every frame from the snapshot, so it
         turns itself back on the moment the last row is resolved. */
      joystickmenu = 1;
      for (count = 0; count < numofmenuitems; count++)
        if (rowactive[count])
          joystickmenu = 0;

      checkkeyboard();
      checkmouse();
      checkjoystick();
      checkmenuitems();

      if (control[0].joysticknum >= numofjoysticks)
        control[0].joysticknum = -1;
      if (control[1].joysticknum >= numofjoysticks)
        control[1].joysticknum = -1;

      setuptextdisplay();

      drawtext(TXT_PLAYER, 0, 80, 16, 1.0f, 1.0f, 1.0f, 1.0f);
      drawtext(TXT_JOYSTICK, 320, 80, 16, 1.0f, 1.0f, 1.0f, 1.0f);

      drawtext(TXT_PLAYER1, 0, 112, 16, 1.0f, 1.0f, 1.0f, 1.0f);
      /* Player 1's pad slot: display-only, drawn where the old item sat and in
         the same 50% grey an unhighlighted menuitem used. */
      drawtext(joynumlabel(control[0].joysticknum), 320, 112, 16, 0.5f, 0.5f, 0.5f, 1.0f);

      count2 = 0;
      count  = 0;
      drawtext(TXT_MOVE_LEFT, 0, 144 + count * 16, 16, 0.75f, 0.75f, 0.75f, 1.0f);
      drawfixedlabel(control[count2].button[count], 320, 144 + count * 16, 16, TXT_AXIS);
      count++;

      drawtext(TXT_MOVE_RIGHT, 0, 144 + count * 16, 16, 0.75f, 0.75f, 0.75f, 1.0f);
      drawfixedlabel(control[count2].button[count], 320, 144 + count * 16, 16, TXT_AXIS);
      count++;

      drawtext(TXT_MOVE_DOWN, 0, 144 + count * 16, 16, 0.75f, 0.75f, 0.75f, 1.0f);
      drawfixedlabel(control[count2].button[count], 320, 144 + count * 16, 16, TXT_AXIS);
      count++;

      drawtext(TXT_MOVE_UP, 0, 144 + count * 16, 16, 0.75f, 0.75f, 0.75f, 1.0f);
      drawfixedlabel(control[count2].button[count], 320, 144 + count * 16, 16, TXT_AXIS);
      count++;

      drawtext(TXT_STICK, 0, 144 + count * 16, 16, 0.75f, 0.75f, 0.75f, 1.0f);
      if (!menuitem[count - 3].active)
      {
        if (control[count2].button[count] != -1)
          drawjoylabel(control[count2].button[count], 320, 144 + count * 16, 16);
        else
          drawtext(TXT_NONE, 320, 144 + count * 16, 16, 1.0f, 1.0f, 1.0f, 1.0f);
      }
      else
        drawtext("?", 320, 144 + count * 16, 16, 1.0f, 1.0f, 1.0f, 1.0f);
      count++;

      drawtext(TXT_JUMP, 0, 144 + count * 16, 16, 0.75f, 0.75f, 0.75f, 1.0f);
      if (!menuitem[count - 3].active)
      {
        if (control[count2].button[count] != -1)
          drawjoylabel(control[count2].button[count], 320, 144 + count * 16, 16);
        else
          drawtext(TXT_NONE, 320, 144 + count * 16, 16, 1.0f, 1.0f, 1.0f, 1.0f);
      }
      else
        drawtext("?", 320, 144 + count * 16, 16, 1.0f, 1.0f, 1.0f, 1.0f);
      count++;

      drawtext(TXT_SLIDE, 0, 144 + count * 16, 16, 0.75f, 0.75f, 0.75f, 1.0f);
      if (!menuitem[count - 3].active)
      {
        if (control[count2].button[count] != -1)
          drawjoylabel(control[count2].button[count], 320, 144 + count * 16, 16);
        else
          drawtext(TXT_NONE, 320, 144 + count * 16, 16, 1.0f, 1.0f, 1.0f, 1.0f);
      }
      else
        drawtext("?", 320, 144 + count * 16, 16, 1.0f, 1.0f, 1.0f, 1.0f);
      count++;

      drawtext(TXT_HEAVY, 0, 144 + count * 16, 16, 0.75f, 0.75f, 0.75f, 1.0f);
      if (!menuitem[count - 3].active)
      {
        if (control[count2].button[count] != -1)
          drawjoylabel(control[count2].button[count], 320, 144 + count * 16, 16);
        else
          drawtext(TXT_NONE, 320, 144 + count * 16, 16, 1.0f, 1.0f, 1.0f, 1.0f);
      }
      else
        drawtext("?", 320, 144 + count * 16, 16, 1.0f, 1.0f, 1.0f, 1.0f);
      count++;

      drawtext(TXT_START_PAUSE, 0, 144 + count * 16, 16, 0.75f, 0.75f, 0.75f, 1.0f);
      drawfixedlabel(control[count2].button[count], 320, 144 + count * 16, 16, TXT_NONE);
      count++;

      drawtext(TXT_PLAYER2, 0, 304, 16, 1.0f, 1.0f, 1.0f, 1.0f);

      count2 = 1;
      count  = 0;
      drawtext(TXT_MOVE_LEFT, 0, 336 + count * 16, 16, 0.75f, 0.75f, 0.75f, 1.0f);
      drawfixedlabel(control[count2].button[count], 320, 336 + count * 16, 16, TXT_AXIS);
      count++;

      drawtext(TXT_MOVE_RIGHT, 0, 336 + count * 16, 16, 0.75f, 0.75f, 0.75f, 1.0f);
      drawfixedlabel(control[count2].button[count], 320, 336 + count * 16, 16, TXT_AXIS);
      count++;

      drawtext(TXT_MOVE_DOWN, 0, 336 + count * 16, 16, 0.75f, 0.75f, 0.75f, 1.0f);
      drawfixedlabel(control[count2].button[count], 320, 336 + count * 16, 16, TXT_AXIS);
      count++;

      drawtext(TXT_MOVE_UP, 0, 336 + count * 16, 16, 0.75f, 0.75f, 0.75f, 1.0f);
      drawfixedlabel(control[count2].button[count], 320, 336 + count * 16, 16, TXT_AXIS);
      count++;

      drawtext(TXT_STICK, 0, 336 + count * 16, 16, 0.75f, 0.75f, 0.75f, 1.0f);
      if (!menuitem[count + 1].active)
      {
        if (control[count2].button[count] != -1)
          drawjoylabel(control[count2].button[count], 320, 336 + count * 16, 16);
        else
          drawtext(TXT_NONE, 320, 336 + count * 16, 16, 1.0f, 1.0f, 1.0f, 1.0f);
      }
      else
        drawtext("?", 320, 336 + count * 16, 16, 1.0f, 1.0f, 1.0f, 1.0f);
      count++;

      drawtext(TXT_JUMP, 0, 336 + count * 16, 16, 0.75f, 0.75f, 0.75f, 1.0f);
      if (!menuitem[count + 1].active)
      {
        if (control[count2].button[count] != -1)
          drawjoylabel(control[count2].button[count], 320, 336 + count * 16, 16);
        else
          drawtext(TXT_NONE, 320, 336 + count * 16, 16, 1.0f, 1.0f, 1.0f, 1.0f);
      }
      else
        drawtext("?", 320, 336 + count * 16, 16, 1.0f, 1.0f, 1.0f, 1.0f);
      count++;

      drawtext(TXT_SLIDE, 0, 336 + count * 16, 16, 0.75f, 0.75f, 0.75f, 1.0f);
      if (!menuitem[count + 1].active)
      {
        if (control[count2].button[count] != -1)
          drawjoylabel(control[count2].button[count], 320, 336 + count * 16, 16);
        else
          drawtext(TXT_NONE, 320, 336 + count * 16, 16, 1.0f, 1.0f, 1.0f, 1.0f);
      }
      else
        drawtext("?", 320, 336 + count * 16, 16, 1.0f, 1.0f, 1.0f, 1.0f);
      count++;

      drawtext(TXT_HEAVY, 0, 336 + count * 16, 16, 0.75f, 0.75f, 0.75f, 1.0f);
      if (!menuitem[count + 1].active)
      {
        if (control[count2].button[count] != -1)
          drawjoylabel(control[count2].button[count], 320, 336 + count * 16, 16);
        else
          drawtext(TXT_NONE, 320, 336 + count * 16, 16, 1.0f, 1.0f, 1.0f, 1.0f);
      }
      else
        drawtext("?", 320, 336 + count * 16, 16, 1.0f, 1.0f, 1.0f, 1.0f);
      count++;

      if (control[0].joysticknum != -1)
        drawtext(joystick[control[0].joysticknum].name, 320, 128, 10, 0.5f, 0.5f, 0.5f, 1.0f);
      if (control[1].joysticknum != -1)
        drawtext(joystick[control[1].joysticknum].name, 320, 320, 10, 0.5f, 0.5f, 0.5f, 1.0f);

      drawmenuitems();

      drawsliderbars();

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
         assigned yet (that handler is skipped entirely).  This exists only on
         the Options screen - Select is an ordinary button everywhere else. */
      if (control[0].joysticknum != -1)
        if (joystick[control[0].joysticknum].numofbuttons > 10)
          if (joystick[control[0].joysticknum].button[10]
              && !prevjoystick[control[0].joysticknum].button[10])
            for (count = 0; count < numofmenuitems; count++)
              menuitem[count].active = 0;

        /* Vita port: only buttons 4-7 (Stick, Jump, Slide, Heavy) are
           rebindable - the d-pad rows and Start are fixed and drawn by
           drawfixedlabel(), so they have no item to arm.  count stays the
           button index, which is why the item index is offset by three. */
        for (count = 4; count < 8; count++)
          if (menuitem[count - 3].active && rowactive[count - 3])
          {
            for (count2 = 0; count2 < joystick[control[0].joysticknum].numofbuttons; count2++)
              if (!reservedbutton(count2)
                  && joystick[control[0].joysticknum].button[count2]
                  && !prevjoystick[control[0].joysticknum].button[count2])
              {
                control[0].button[count] = count2;

                if (control[0].joysticknum != -1)
                  for (count3 = 0; count3 < joystick[control[0].joysticknum].numofbuttons; count3++)
                    if (count3 != count)
                      if (control[0].button[count3] == count2)
                        control[0].button[count3] = -1;
                /*
                if (control[1].joysticknum!=-1)
                for (count3=0;count3<joystick[control[1].joysticknum].numofbuttons;count3++)
                if (count3!=count)
                if (control[1].button[count3]==count2)
                  control[1].button[count3]=-1;
                */
                menuitem[count - 3].active = 0;
              }
            if (keyboard[SCAN_DELETE] && !prevkeyboard[SCAN_DELETE])
            {
              control[0].button[count]    = -1;
              menuitem[count - 3].active = 0;
            }
          }
      if (control[1].joysticknum != -1)
        for (count = 4; count < 8; count++)
          if (menuitem[count + 1].active && rowactive[count + 1])
          {
            for (count2 = 0; count2 < joystick[control[1].joysticknum].numofbuttons; count2++)
              if (!reservedbutton(count2)
                  && joystick[control[1].joysticknum].button[count2]
                  && !prevjoystick[control[1].joysticknum].button[count2])
              {
                control[1].button[count] = count2;
                /*
                if (control[0].joysticknum!=-1)
                for (count3=0;count3<8;count3++)
                if (count3!=count)
                if (control[0].button[count3]==count2)
                  control[0].button[count3]=-1;
                */
                if (control[1].joysticknum != -1)
                  for (count3 = 0; count3 < 8; count3++)
                    if (count3 != count)
                      if (control[1].button[count3] == count2)
                        control[1].button[count3] = -1;

                menuitem[count + 1].active = 0;
              }
            if (keyboard[SCAN_DELETE] && !prevkeyboard[SCAN_DELETE])
            {
              control[1].button[count]    = -1;
              menuitem[count + 1].active = 0;
            }
          }
      if (menuitem[9].active)
        optionsmenu2();

      if (mouse.lmb)
        if (mouse.x >= 160 && mouse.x < 288)
        {
          if (mouse.y >= 32 && mouse.y < 48)
            option.soundvolume = (float)(mouse.x - 160) / 128.0f;
          if (mouse.y >= 48 && mouse.y < 64)
            option.musicvolume = (float)(mouse.x - 160) / 128.0f;
        }
    }

    resetmenuitems();

    joystickmenu = 1;
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

void drawsliderbars(void)
{
  float vec[3];

  glDisable(GL_TEXTURE_2D);

#if defined(GLES)
  GLfloat quad[12];
  glEnableClientState(GL_VERTEX_ARRAY);
#else
  glBegin(GL_QUADS);
#endif
  if (option.sound)
  {
    glColor4f(0.25f, 0.25f, 0.25f, 1.0f);

    vec[0] = 160.0f;
    vec[1] = 40.0f - 1.0f;
    convertscreenvertex(vec, font.sizex, font.sizey);
#if defined(GLES)
    quad[0] = vec[0];
    quad[1] = vec[1];
    quad[2] = vec[2];
#else
    glVertex3fv(vec);
#endif

    vec[0] = 160.0f + 128.0f;
    vec[1] = 40.0f - 5.0f;
    convertscreenvertex(vec, font.sizex, font.sizey);
#if defined(GLES)
    quad[3] = vec[0];
    quad[4] = vec[1];
    quad[5] = vec[2];
#else
    glVertex3fv(vec);
#endif

    vec[0] = 160.0f + 128.0f;
    vec[1] = 40.0f + 5.0f;
    convertscreenvertex(vec, font.sizex, font.sizey);
#if defined(GLES)
    quad[6] = vec[0];
    quad[7] = vec[1];
    quad[8] = vec[2];
#else
    glVertex3fv(vec);
#endif

    vec[0] = 160.0f;
    vec[1] = 40.0f + 1.0f;
    convertscreenvertex(vec, font.sizex, font.sizey);
#if defined(GLES)
    quad[9] = vec[0];
    quad[10] = vec[1];
    quad[11] = vec[2];

    glVertexPointer(3, GL_FLOAT, 0, quad);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glDisableClientState(GL_VERTEX_ARRAY);
#else
    glVertex3fv(vec);
#endif

    glColor4f(0.75f, 0.75f, 0.75f, 1.0f);

    vec[0] = 160.0f + option.soundvolume * 128.0f - 2.0f;
    vec[1] = 40.0f - 7.0f;
    convertscreenvertex(vec, font.sizex, font.sizey);
#if defined(GLES)
    glEnableClientState(GL_VERTEX_ARRAY);

    quad[0] = vec[0];
    quad[1] = vec[1];
    quad[2] = vec[2];
#else
    glVertex3fv(vec);
#endif

    vec[0] = 160.0f + option.soundvolume * 128.0f + 2.0f;
    vec[1] = 40.0f - 7.0f;
    convertscreenvertex(vec, font.sizex, font.sizey);
#if defined(GLES)
    quad[3] = vec[0];
    quad[4] = vec[1];
    quad[5] = vec[2];
#else
    glVertex3fv(vec);
#endif

    vec[0] = 160.0f + option.soundvolume * 128.0f + 2.0f;
    vec[1] = 40.0f + 7.0f;
    convertscreenvertex(vec, font.sizex, font.sizey);
#if defined(GLES)
    quad[6] = vec[0];
    quad[7] = vec[1];
    quad[8] = vec[2];
#else
    glVertex3fv(vec);
#endif

    vec[0] = 160.0f + option.soundvolume * 128.0f - 2.0f;
    vec[1] = 40.0f + 7.0f;
    convertscreenvertex(vec, font.sizex, font.sizey);
#if defined(GLES)
    quad[9] = vec[0];
    quad[10] = vec[1];
    quad[11] = vec[2];

    glVertexPointer(3, GL_FLOAT, 0, quad);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glDisableClientState(GL_VERTEX_ARRAY);
#else
    glVertex3fv(vec);
#endif
  }
  if (option.music)
  {
    glColor4f(0.25f, 0.25f, 0.25f, 1.0f);

    vec[0] = 160.0f;
    vec[1] = 56.0f - 1.0f;
    convertscreenvertex(vec, font.sizex, font.sizey);
#if defined(GLES)
    glEnableClientState(GL_VERTEX_ARRAY);

    quad[0] = vec[0];
    quad[1] = vec[1];
    quad[2] = vec[2];
#else
    glVertex3fv(vec);
#endif

    vec[0] = 160.0f + 128.0f;
    vec[1] = 56.0f - 5.0f;
    convertscreenvertex(vec, font.sizex, font.sizey);
#if defined(GLES)
    quad[3] = vec[0];
    quad[4] = vec[1];
    quad[5] = vec[2];
#else
    glVertex3fv(vec);
#endif

    vec[0] = 160.0f + 128.0f;
    vec[1] = 56.0f + 5.0f;
    convertscreenvertex(vec, font.sizex, font.sizey);
#if defined(GLES)
    quad[6] = vec[0];
    quad[7] = vec[1];
    quad[8] = vec[2];
#else
    glVertex3fv(vec);
#endif

    vec[0] = 160.0f;
    vec[1] = 56.0f + 1.0f;
    convertscreenvertex(vec, font.sizex, font.sizey);
#if defined(GLES)
    quad[9] = vec[0];
    quad[10] = vec[1];
    quad[11] = vec[2];

    glVertexPointer(3, GL_FLOAT, 0, quad);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glDisableClientState(GL_VERTEX_ARRAY);
#else
    glVertex3fv(vec);
#endif

    glColor4f(0.75f, 0.75f, 0.75f, 1.0f);

    vec[0] = 160.0f + option.musicvolume * 128.0f - 2.0f;
    vec[1] = 56.0f - 7.0f;
    convertscreenvertex(vec, font.sizex, font.sizey);
#if defined(GLES)
    glEnableClientState(GL_VERTEX_ARRAY);

    quad[0] = vec[0];
    quad[1] = vec[1];
    quad[2] = vec[2];
#else
    glVertex3fv(vec);
#endif

    vec[0] = 160.0f + option.musicvolume * 128.0f + 2.0f;
    vec[1] = 56.0f - 7.0f;
    convertscreenvertex(vec, font.sizex, font.sizey);
#if defined(GLES)
    quad[3] = vec[0];
    quad[4] = vec[1];
    quad[5] = vec[2];
#else
    glVertex3fv(vec);
#endif

    vec[0] = 160.0f + option.musicvolume * 128.0f + 2.0f;
    vec[1] = 56.0f + 7.0f;
    convertscreenvertex(vec, font.sizex, font.sizey);
#if defined(GLES)
    quad[6] = vec[0];
    quad[7] = vec[1];
    quad[8] = vec[2];
#else
    glVertex3fv(vec);
#endif
    vec[0] = 160.0f + option.musicvolume * 128.0f - 2.0f;
    vec[1] = 56.0f + 7.0f;
    convertscreenvertex(vec, font.sizex, font.sizey);

#if defined(GLES)
    quad[9] = vec[0];
    quad[10] = vec[1];
    quad[11] = vec[2];

    glVertexPointer(3, GL_FLOAT, 0, quad);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glDisableClientState(GL_VERTEX_ARRAY);
#else
    glVertex3fv(vec);
#endif
  }

#if !defined(GLES)
  glEnd();
#endif
  glEnable(GL_TEXTURE_2D);
}

void optionsmenu2(void)
{
  int count, count2, count3;
  /* Same activation-press guard as optionsmenu(): was each item already
     waiting for a button before this frame's input? */
  unsigned char rowactive[MAXMENUITEMS];

  count = 0;

  resetmenuitems();

  while (!menuitem[0].active && !windowinfo.shutdown)
  {
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    numofmenuitems = 0;
    createmenuitem(TXT_BACK, 0, 0, 16, 1.0f, 1.0f, 1.0f, 1.0f);
    setmenuitem(MO_HOTKEY, SCAN_ESC);
    /* Vita port: only the four rebindable rows of each player (buttons 4-7)
       get menu items - the d-pad rows are fixed and drawn by drawfixedlabel(). */
    count = 208;
    createmenuitem("        ", 320, count, 16, 1.0f, 1.0f, 1.0f, 1.0f);
    count += 16;
    createmenuitem("        ", 320, count, 16, 1.0f, 1.0f, 1.0f, 1.0f);
    count += 16;
    createmenuitem("        ", 320, count, 16, 1.0f, 1.0f, 1.0f, 1.0f);
    count += 16;
    createmenuitem("        ", 320, count, 16, 1.0f, 1.0f, 1.0f, 1.0f);
    count += 16;

    count = 400;
    createmenuitem("        ", 320, count, 16, 1.0f, 1.0f, 1.0f, 1.0f);
    count += 16;
    createmenuitem("        ", 320, count, 16, 1.0f, 1.0f, 1.0f, 1.0f);
    count += 16;
    createmenuitem("        ", 320, count, 16, 1.0f, 1.0f, 1.0f, 1.0f);
    count += 16;
    createmenuitem("        ", 320, count, 16, 1.0f, 1.0f, 1.0f, 1.0f);
    count += 16;

    count = 112;
    if (control[2].joysticknum == -1)
    {
      createmenuitem(TXT_NONE, 320, count, 16, 1.0f, 1.0f, 1.0f, 1.0f);
      setmenuitem(MO_SET, &control[2].joysticknum, 0);
    }
    if (control[2].joysticknum == 0)
    {
      createmenuitem(TXT_JOY1, 320, count, 16, 1.0f, 1.0f, 1.0f, 1.0f);
      setmenuitem(MO_SET, &control[2].joysticknum, 1);
    }
    if (control[2].joysticknum == 1)
    {
      createmenuitem(TXT_JOY2, 320, count, 16, 1.0f, 1.0f, 1.0f, 1.0f);
      setmenuitem(MO_SET, &control[2].joysticknum, 2);
    }
    if (control[2].joysticknum == 2)
    {
      createmenuitem(TXT_JOY3, 320, count, 16, 1.0f, 1.0f, 1.0f, 1.0f);
      setmenuitem(MO_SET, &control[2].joysticknum, 3);
    }
    if (control[2].joysticknum == 3)
    {
      createmenuitem(TXT_JOY4, 320, count, 16, 1.0f, 1.0f, 1.0f, 1.0f);
      setmenuitem(MO_SET, &control[2].joysticknum, -1);
    }
    count = 304;
    if (control[3].joysticknum == -1)
    {
      createmenuitem(TXT_NONE, 320, count, 16, 1.0f, 1.0f, 1.0f, 1.0f);
      setmenuitem(MO_SET, &control[3].joysticknum, 0);
    }
    if (control[3].joysticknum == 0)
    {
      createmenuitem(TXT_JOY1, 320, count, 16, 1.0f, 1.0f, 1.0f, 1.0f);
      setmenuitem(MO_SET, &control[3].joysticknum, 1);
    }
    if (control[3].joysticknum == 1)
    {
      createmenuitem(TXT_JOY2, 320, count, 16, 1.0f, 1.0f, 1.0f, 1.0f);
      setmenuitem(MO_SET, &control[3].joysticknum, 2);
    }
    if (control[3].joysticknum == 2)
    {
      createmenuitem(TXT_JOY3, 320, count, 16, 1.0f, 1.0f, 1.0f, 1.0f);
      setmenuitem(MO_SET, &control[3].joysticknum, 3);
    }
    if (control[3].joysticknum == 3)
    {
      createmenuitem(TXT_JOY4, 320, count, 16, 1.0f, 1.0f, 1.0f, 1.0f);
      setmenuitem(MO_SET, &control[3].joysticknum, -1);
    }

    checksystemmessages();

    /* See optionsmenu(): remember which rows were already armed, so the Start
       press that arms one is not mistaken for the binding itself. */
    for (count = 0; count < numofmenuitems; count++)
      rowactive[count] = (unsigned char)menuitem[count].active;

    /* See optionsmenu(): no pad navigation while a row waits for a button. */
    joystickmenu = 1;
    for (count = 0; count < numofmenuitems; count++)
      if (rowactive[count])
        joystickmenu = 0;

    checkkeyboard();
    checkmouse();
    checkjoystick();
    checkmenuitems();

    if (control[2].joysticknum >= numofjoysticks)
      control[2].joysticknum = -1;
    if (control[3].joysticknum >= numofjoysticks)
      control[3].joysticknum = -1;

    setuptextdisplay();

    drawtext(TXT_PLAYER, 0, 80, 16, 1.0f, 1.0f, 1.0f, 1.0f);
    drawtext(TXT_JOYSTICK, 320, 80, 16, 1.0f, 1.0f, 1.0f, 1.0f);

    drawtext(TXT_PLAYER3, 0, 112, 16, 1.0f, 1.0f, 1.0f, 1.0f);

    count2 = 2;
    count  = 0;
    drawtext(TXT_MOVE_LEFT, 0, 144 + count * 16, 16, 0.75f, 0.75f, 0.75f, 1.0f);
    drawfixedlabel(control[count2].button[count], 320, 144 + count * 16, 16, TXT_AXIS);
    count++;

    drawtext(TXT_MOVE_RIGHT, 0, 144 + count * 16, 16, 0.75f, 0.75f, 0.75f, 1.0f);
    drawfixedlabel(control[count2].button[count], 320, 144 + count * 16, 16, TXT_AXIS);
    count++;

    drawtext(TXT_MOVE_DOWN, 0, 144 + count * 16, 16, 0.75f, 0.75f, 0.75f, 1.0f);
    drawfixedlabel(control[count2].button[count], 320, 144 + count * 16, 16, TXT_AXIS);
    count++;

    drawtext(TXT_MOVE_UP, 0, 144 + count * 16, 16, 0.75f, 0.75f, 0.75f, 1.0f);
    drawfixedlabel(control[count2].button[count], 320, 144 + count * 16, 16, TXT_AXIS);
    count++;

    drawtext(TXT_STICK, 0, 144 + count * 16, 16, 0.75f, 0.75f, 0.75f, 1.0f);
    if (!menuitem[count - 3].active)
    {
      if (control[count2].button[count] != -1)
        drawjoylabel(control[count2].button[count], 320, 144 + count * 16, 16);
      else
        drawtext(TXT_NONE, 320, 144 + count * 16, 16, 1.0f, 1.0f, 1.0f, 1.0f);
    }
    else
      drawtext("?", 320, 144 + count * 16, 16, 1.0f, 1.0f, 1.0f, 1.0f);
    count++;

    drawtext(TXT_JUMP, 0, 144 + count * 16, 16, 0.75f, 0.75f, 0.75f, 1.0f);
    if (!menuitem[count - 3].active)
    {
      if (control[count2].button[count] != -1)
        drawjoylabel(control[count2].button[count], 320, 144 + count * 16, 16);
      else
        drawtext(TXT_NONE, 320, 144 + count * 16, 16, 1.0f, 1.0f, 1.0f, 1.0f);
    }
    else
      drawtext("?", 320, 144 + count * 16, 16, 1.0f, 1.0f, 1.0f, 1.0f);
    count++;

    drawtext(TXT_SLIDE, 0, 144 + count * 16, 16, 0.75f, 0.75f, 0.75f, 1.0f);
    if (!menuitem[count - 3].active)
    {
      if (control[count2].button[count] != -1)
        drawjoylabel(control[count2].button[count], 320, 144 + count * 16, 16);
      else
        drawtext(TXT_NONE, 320, 144 + count * 16, 16, 1.0f, 1.0f, 1.0f, 1.0f);
    }
    else
      drawtext("?", 320, 144 + count * 16, 16, 1.0f, 1.0f, 1.0f, 1.0f);
    count++;

    drawtext(TXT_HEAVY, 0, 144 + count * 16, 16, 0.75f, 0.75f, 0.75f, 1.0f);
    if (!menuitem[count - 3].active)
    {
      if (control[count2].button[count] != -1)
        drawjoylabel(control[count2].button[count], 320, 144 + count * 16, 16);
      else
        drawtext(TXT_NONE, 320, 144 + count * 16, 16, 1.0f, 1.0f, 1.0f, 1.0f);
    }
    else
      drawtext("?", 320, 144 + count * 16, 16, 1.0f, 1.0f, 1.0f, 1.0f);
    count++;
    /*
    drawtext("Start//Pause",0,144+count*16,16,0.75f,0.75f,0.75f,1.0f);
    if (!menuitem[count + 1].active)
      {
      if (control[count2].button[count]!=-1)
        drawtext(TXT_BUTTON" /i",320,144+count*16,16,1.0f,1.0f,1.0f,1.0f,control[count2].button[count]+1);
      else
        drawtext(TXT_NONE,320,144+count*16,16,1.0f,1.0f,1.0f,1.0f);
      }
    else
      drawtext("?",320,144+count*16,16,1.0f,1.0f,1.0f,1.0f);
    count++;
    */

    drawtext(TXT_PLAYER4, 0, 304, 16, 1.0f, 1.0f, 1.0f, 1.0f);

    count2 = 3;
    count  = 0;
    drawtext(TXT_MOVE_LEFT, 0, 336 + count * 16, 16, 0.75f, 0.75f, 0.75f, 1.0f);
    drawfixedlabel(control[count2].button[count], 320, 336 + count * 16, 16, TXT_AXIS);
    count++;

    drawtext(TXT_MOVE_RIGHT, 0, 336 + count * 16, 16, 0.75f, 0.75f, 0.75f, 1.0f);
    drawfixedlabel(control[count2].button[count], 320, 336 + count * 16, 16, TXT_AXIS);
    count++;

    drawtext(TXT_MOVE_DOWN, 0, 336 + count * 16, 16, 0.75f, 0.75f, 0.75f, 1.0f);
    drawfixedlabel(control[count2].button[count], 320, 336 + count * 16, 16, TXT_AXIS);
    count++;

    drawtext(TXT_MOVE_UP, 0, 336 + count * 16, 16, 0.75f, 0.75f, 0.75f, 1.0f);
    drawfixedlabel(control[count2].button[count], 320, 336 + count * 16, 16, TXT_AXIS);
    count++;

    drawtext(TXT_STICK, 0, 336 + count * 16, 16, 0.75f, 0.75f, 0.75f, 1.0f);
    if (!menuitem[count + 1].active)
    {
      if (control[count2].button[count] != -1)
        drawjoylabel(control[count2].button[count], 320, 336 + count * 16, 16);
      else
        drawtext(TXT_NONE, 320, 336 + count * 16, 16, 1.0f, 1.0f, 1.0f, 1.0f);
    }
    else
      drawtext("?", 320, 336 + count * 16, 16, 1.0f, 1.0f, 1.0f, 1.0f);
    count++;

    drawtext(TXT_JUMP, 0, 336 + count * 16, 16, 0.75f, 0.75f, 0.75f, 1.0f);
    if (!menuitem[count + 1].active)
    {
      if (control[count2].button[count] != -1)
        drawjoylabel(control[count2].button[count], 320, 336 + count * 16, 16);
      else
        drawtext(TXT_NONE, 320, 336 + count * 16, 16, 1.0f, 1.0f, 1.0f, 1.0f);
    }
    else
      drawtext("?", 320, 336 + count * 16, 16, 1.0f, 1.0f, 1.0f, 1.0f);
    count++;

    drawtext(TXT_SLIDE, 0, 336 + count * 16, 16, 0.75f, 0.75f, 0.75f, 1.0f);
    if (!menuitem[count + 1].active)
    {
      if (control[count2].button[count] != -1)
        drawjoylabel(control[count2].button[count], 320, 336 + count * 16, 16);
      else
        drawtext(TXT_NONE, 320, 336 + count * 16, 16, 1.0f, 1.0f, 1.0f, 1.0f);
    }
    else
      drawtext("?", 320, 336 + count * 16, 16, 1.0f, 1.0f, 1.0f, 1.0f);
    count++;

    drawtext(TXT_HEAVY, 0, 336 + count * 16, 16, 0.75f, 0.75f, 0.75f, 1.0f);
    if (!menuitem[count + 1].active)
    {
      if (control[count2].button[count] != -1)
        drawjoylabel(control[count2].button[count], 320, 336 + count * 16, 16);
      else
        drawtext(TXT_NONE, 320, 336 + count * 16, 16, 1.0f, 1.0f, 1.0f, 1.0f);
    }
    else
      drawtext("?", 320, 336 + count * 16, 16, 1.0f, 1.0f, 1.0f, 1.0f);
    count++;

    if (control[2].joysticknum != -1)
      drawtext(joystick[control[2].joysticknum].name, 320, 128, 10, 0.5f, 0.5f, 0.5f, 1.0f);
    if (control[3].joysticknum != -1)
      drawtext(joystick[control[3].joysticknum].name, 320, 320, 10, 0.5f, 0.5f, 0.5f, 1.0f);

    drawmenuitems();

    drawmousecursor(768 + font.cursornum, mouse.x, mouse.y, 16, 1.0f, 1.0f, 1.0f, 1.0f);

#if defined(PC_GLES)
    eglSwapBuffers(eglDisplay, eglSurface);
#else
    SDL_GL_SwapWindow(globalwindow);
#endif

    /* See optionsmenu(): Select cancels any armed rebind row. */
    if (control[0].joysticknum != -1)
      if (joystick[control[0].joysticknum].numofbuttons > 10)
        if (joystick[control[0].joysticknum].button[10]
            && !prevjoystick[control[0].joysticknum].button[10])
          for (count = 0; count < numofmenuitems; count++)
            menuitem[count].active = 0;

      for (count = 4; count < 8; count++)
        if (menuitem[count - 3].active && rowactive[count - 3])
        {
          for (count2 = 0; count2 < joystick[control[2].joysticknum].numofbuttons; count2++)
            if (!reservedbutton(count2) && joystick[control[2].joysticknum].button[count2] && !prevjoystick[control[2].joysticknum].button[count2])
            {
              control[2].button[count] = count2;

              if (control[2].joysticknum != -1)
                for (count3 = 0; count3 < joystick[control[2].joysticknum].numofbuttons; count3++)
                  if (count3 != count)
                    if (control[2].button[count3] == count2)
                      control[2].button[count3] = -1;

              menuitem[count - 3].active = 0;
            }
          if (keyboard[SCAN_DELETE] && !prevkeyboard[SCAN_DELETE])
          {
            control[2].button[count]    = -1;
            menuitem[count - 3].active = 0;
          }
        }
    if (control[3].joysticknum != -1)
      for (count = 4; count < 8; count++)
        if (menuitem[count + 1].active && rowactive[count + 1])
        {
          for (count2 = 0; count2 < joystick[control[3].joysticknum].numofbuttons; count2++)
            if (!reservedbutton(count2) && joystick[control[3].joysticknum].button[count2] && !prevjoystick[control[3].joysticknum].button[count2])
            {
              control[3].button[count] = count2;

              if (control[3].joysticknum != -1)
                for (count3 = 0; count3 < 8; count3++)
                  if (count3 != count)
                    if (control[3].button[count3] == count2)
                      control[3].button[count3] = -1;

              menuitem[count + 1].active = 0;
            }
          if (keyboard[SCAN_DELETE] && !prevkeyboard[SCAN_DELETE])
          {
            control[3].button[count]    = -1;
            menuitem[count + 1].active = 0;
          }
        }
  }

  /* Never leave the parent screen with navigation switched off: on the last
     frame of a cancelled 4 Player screen a row could still have been armed. */
  joystickmenu = 1;

  resetmenuitems();
}

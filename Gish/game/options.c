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
   is what to show when nothing is bound (TXT_AXIS or TXT_NONE). */
static void drawbindlabel(int button, int item, int x, int y, int textsize, char *none)
{
  char text[64];
  int len;

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

  drawtext(text, x, y, textsize, 1.0f, 1.0f, 1.0f, 1.0f);
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
       With the KEY column gone the items run in one visual column: Save, the
       four rebindable player-1 rows, the four rebindable player-2 rows,
       4 Player, the player-2 pad slot, then one line holding two groups: the
       Sound row (toggle, "-", "+") at x = 0 and the Music row (toggle, "-",
       "+") at x = 320.  All three items of each group always exist; a group
       whose toggle is off is drawn dim and is skipped by the navigation, so
       the item order (and therefore every index below) never changes.  Down/Up
       move, Start activates - the same path every other menu uses.  Touch
       still works, because the hit test in checkmenuitems() runs either way. */
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

      /* Vita port: the "- N% +" volume rows (see adjustvolume()).  Both groups
         are laid out identically and exactly 320 apart: the toggle at x, "-"
         at x + 160, the readout centred at x + 216 and "+" at x + 256.  Sound
         is the left column (x = 0) and Music the right one (x = 320) on the
         same line, so the two read as a pair; Music used to sit on a line of
         its own at y = 48.  The gaps work out at 8 px either side of a
         four-glyph "100%" and 16 px around a three-glyph "50%", i.e. the row
         reads as one control rather than three spread-out labels.  To widen it
         again, move the readout centre and "+" right together.

         All three items of a group are created every frame; only their state
         changes.  A group whose toggle is off stays on screen as a dimmed,
         unusable row (the user's choice - the earlier build removed it, which
         also moved every later item's index around), so the item order below
         is fixed: Sound 11, sound "-" 12, sound "+" 13, Music 14, music "-"
         15, music "+" 16.  Disabling is MO_HIGHLIGHT 0: checkmenuitems() then
         skips the item entirely (no hit test, no hotkey, no activation) and
         drawmenuitems() draws it at 25% brightness instead of the usual 50%. */
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
      /* The Music toggle is disabled only by Sound Off (master mute - see the
         knowledge file), never by Music being off: that toggle is the way back
         on.  The Music group is disabled by either - Music off means there is
         nothing to adjust, Sound off means Music does nothing at all.  Both
         cases share one condition, so they look identical (the same 25%) and
         cannot stack into something dimmer or leave a button live that the
         other case would grey: with Music already off, turning Sound off
         changes nothing about this row. */
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
      drawbindlabel(control[count2].button[count], count - 3, 320, 144 + count * 16, 16, TXT_NONE);
      count++;

      drawtext(TXT_JUMP, 0, 144 + count * 16, 16, 0.75f, 0.75f, 0.75f, 1.0f);
      drawbindlabel(control[count2].button[count], count - 3, 320, 144 + count * 16, 16, TXT_NONE);
      count++;

      drawtext(TXT_SLIDE, 0, 144 + count * 16, 16, 0.75f, 0.75f, 0.75f, 1.0f);
      drawbindlabel(control[count2].button[count], count - 3, 320, 144 + count * 16, 16, TXT_NONE);
      count++;

      drawtext(TXT_HEAVY, 0, 144 + count * 16, 16, 0.75f, 0.75f, 0.75f, 1.0f);
      drawbindlabel(control[count2].button[count], count - 3, 320, 144 + count * 16, 16, TXT_NONE);
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
      drawbindlabel(control[count2].button[count], count + 1, 320, 336 + count * 16, 16, TXT_NONE);
      count++;

      drawtext(TXT_JUMP, 0, 336 + count * 16, 16, 0.75f, 0.75f, 0.75f, 1.0f);
      drawbindlabel(control[count2].button[count], count + 1, 320, 336 + count * 16, 16, TXT_NONE);
      count++;

      drawtext(TXT_SLIDE, 0, 336 + count * 16, 16, 0.75f, 0.75f, 0.75f, 1.0f);
      drawbindlabel(control[count2].button[count], count + 1, 320, 336 + count * 16, 16, TXT_NONE);
      count++;

      drawtext(TXT_HEAVY, 0, 336 + count * 16, 16, 0.75f, 0.75f, 0.75f, 1.0f);
      drawbindlabel(control[count2].button[count], count + 1, 320, 336 + count * 16, 16, TXT_NONE);
      count++;

      if (control[0].joysticknum != -1)
        drawtext(joystick[control[0].joysticknum].name, 320, 128, 10, 0.5f, 0.5f, 0.5f, 1.0f);
      if (control[1].joysticknum != -1)
        drawtext(joystick[control[1].joysticknum].name, 320, 320, 10, 0.5f, 0.5f, 0.5f, 1.0f);

      /* Vita port: the percent readouts of the two volume rows.  Drawn here,
         not as menu items, so the number cannot be selected or clicked - only
         the "-" and "+" either side of it can.  They are drawn unconditionally
         now, because the groups themselves are always on screen; brightness
         mirrors drawmenuitems()'s dimming - 1.0 for a usable row, 0.25 for a
         disabled one - under exactly the same conditions the items use, so the
         number can never be brighter or dimmer than its buttons. */
      drawvolume(option.soundvolume, 0, 32, option.sound ? 1.0f : 0.25f);
      drawvolume(option.musicvolume, 320, 32, (option.music && option.sound) ? 1.0f : 0.25f);

      drawmenuitems();

      /* Vita port: the drag-only volume bars are replaced by the "- N% +" rows
         above, so drawsliderbars() is not called any more.  It is still
         compiled (options.h declares it); restore this call to bring the bars
         back. */
      /* drawsliderbars(); */

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

      /* Vita port: the old drag-only volume bars used to be read here.  They
         are replaced by the "- N% +" rows, whose buttons go through
         checkmenuitems() like every other item.  Kept, commented, in case the
         bars come back.

      if (mouse.lmb)
        if (mouse.x >= 160 && mouse.x < 288)
        {
          if (mouse.y >= 32 && mouse.y < 48)
            option.soundvolume = (float)(mouse.x - 160) / 128.0f;
          if (mouse.y >= 48 && mouse.y < 64)
            option.musicvolume = (float)(mouse.x - 160) / 128.0f;
        }
      */
    }

    resetmenuitems();

    joystickmenu = 1;

    /* Vita port: write config.txt as soon as the player backs out of Options.
       The only other saves are the one at boot (main.c, after loadconfig) and
       the one after mainmenu() returns, i.e. after the Exit item - and the
       Vita has no Exit item worth trusting: closing the application from the
       LiveArea kills the process without reaching that code, so every volume
       change, pad assignment and rebind made here was silently lost.  The file
       is small and saveconfig() rewrites it from memory, so a write per Back
       is cheap.  This is also reached when windowinfo.shutdown ends the loop.
       The 4 Player screen needs nothing of its own: it returns here, and the
       Back that leaves this screen saves players 3 and 4 as well. */
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
    drawbindlabel(control[count2].button[count], count - 3, 320, 144 + count * 16, 16, TXT_NONE);
    count++;

    drawtext(TXT_JUMP, 0, 144 + count * 16, 16, 0.75f, 0.75f, 0.75f, 1.0f);
    drawbindlabel(control[count2].button[count], count - 3, 320, 144 + count * 16, 16, TXT_NONE);
    count++;

    drawtext(TXT_SLIDE, 0, 144 + count * 16, 16, 0.75f, 0.75f, 0.75f, 1.0f);
    drawbindlabel(control[count2].button[count], count - 3, 320, 144 + count * 16, 16, TXT_NONE);
    count++;

    drawtext(TXT_HEAVY, 0, 144 + count * 16, 16, 0.75f, 0.75f, 0.75f, 1.0f);
    drawbindlabel(control[count2].button[count], count - 3, 320, 144 + count * 16, 16, TXT_NONE);
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
    drawbindlabel(control[count2].button[count], count + 1, 320, 336 + count * 16, 16, TXT_NONE);
    count++;

    drawtext(TXT_JUMP, 0, 336 + count * 16, 16, 0.75f, 0.75f, 0.75f, 1.0f);
    drawbindlabel(control[count2].button[count], count + 1, 320, 336 + count * 16, 16, TXT_NONE);
    count++;

    drawtext(TXT_SLIDE, 0, 336 + count * 16, 16, 0.75f, 0.75f, 0.75f, 1.0f);
    drawbindlabel(control[count2].button[count], count + 1, 320, 336 + count * 16, 16, TXT_NONE);
    count++;

    drawtext(TXT_HEAVY, 0, 336 + count * 16, 16, 0.75f, 0.75f, 0.75f, 1.0f);
    drawbindlabel(control[count2].button[count], count + 1, 320, 336 + count * 16, 16, TXT_NONE);
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

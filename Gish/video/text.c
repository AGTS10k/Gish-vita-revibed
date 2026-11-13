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

#include "../video/text.h"

#include "../config.h"
#include "../game/game.h"
#include "../sdl/event.h"
#include "../video/opengl.h"
#include "../video/texture.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

char textstring2[1024];
char textstring3[1024];
_font font;

/* Vita port: when this is set, drawtext lays every glyph out at exactly 16x16
   physical pixels, snaps each quad to a whole pixel boundary and samples the
   16 texels of the atlas cell from 0.5 to 16.5.  One atlas texel then maps to
   exactly one screen pixel and the font is not resampled at all.
   The HUD otherwise lives in the 640x480 design space, which expand mode scales
   by 725/640, so a 16 unit glyph comes out 18.13 px and every stroke lands on a
   fractional pixel.  Enabled by the level 1 tutorial and key legend only -
   menus, dialogs and the score digits keep the design space layout they were
   authored in (their drawbackground tiles live in that same space). */
int textpixelsnap = 0;

/* Screen-space drawing - the frame, HUD icons, menus and text - is not part of
   the level, so it keeps whatever filtering its textures were loaded with,
   even in the retro warp levels where settexturefiltering() switches every
   level texture to GL_NEAREST.  Call right after the texture is bound. */
static void useuitexture(int texturenum)
{
  if (!nearestfiltering)
    return;

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, texture[texturenum].magfilter);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, texture[texturenum].minfilter);
}

void drawtext(char *textstring, int x, int y, int textsize, float red, float green, float blue, float alpha, ...)
{
  int count, count2;
  int variabletemp;
  // int texturenum;
  va_list ap;
  float vec[3];
  float texcoord[2];
  float advx, szx, szy, bx, by, texspan, texoff;

  count  = 0;
  count2 = 0;

  va_start(ap, alpha);
  while (textstring[count] != 0 && count2 < 1000)
  {
    if (textstring[count] == '/')
    {
      switch (textstring[count + 1])
      {
        case '/':
          count++;
          textstring2[count2++] = '/';
          break;

        case 'i':
          count++;
          sprintf(textstring3, "%d", (int)va_arg(ap, int));
          memcpy(textstring2 + count2, textstring3, strlen(textstring3));
          count2 += strlen(textstring3);
          break;

        case 'I':
          count++;
          variabletemp = (int)va_arg(ap, int);
          if (variabletemp >= 1000000)
          {
            sprintf(textstring3, "%d", (variabletemp / 1000000));
            memcpy(textstring2 + count2, textstring3, strlen(textstring3));
            count2 += strlen(textstring3);
            textstring2[count2++] = ',';
          }
          if (variabletemp >= 1000)
          {
            if (variabletemp >= 1000000)
              sprintf(textstring3, "%.3d", ((variabletemp / 1000) % 1000));
            else
              sprintf(textstring3, "%d", ((variabletemp / 1000) % 1000));
            memcpy(textstring2 + count2, textstring3, strlen(textstring3));
            count2 += strlen(textstring3);
            textstring2[count2++] = ',';
          }
          if (variabletemp >= 1000)
            sprintf(textstring3, "%.3d", (variabletemp % 1000));
          else
            sprintf(textstring3, "%d", variabletemp);
          memcpy(textstring2 + count2, textstring3, strlen(textstring3));
          count2 += strlen(textstring3);
          break;

        case 'u':
          count++;
          sprintf(textstring3, "%u", (unsigned int)va_arg(ap, unsigned int));
          memcpy(textstring2 + count2, textstring3, strlen(textstring3));
          count2 += strlen(textstring3);
          break;

        case 'f':
          count++;
          sprintf(textstring3, "%f", (double)va_arg(ap, double));
          memcpy(textstring2 + count2, textstring3, strlen(textstring3));
          count2 += strlen(textstring3);
          break;

        case 'c':
          count++;
          variabletemp = va_arg(ap, int);
          if (variabletemp != 0)
            textstring2[count2++] = variabletemp;
          break;

        case 's':
          count++;
          sprintf(textstring3, "%s", va_arg(ap, char *));
          memcpy(textstring2 + count2, textstring3, strlen(textstring3));
          count2 += strlen(textstring3);
          break;

        case 'x':
          count++;
          sprintf(textstring3, "%X", (unsigned int)va_arg(ap, unsigned int));
          memcpy(textstring2 + count2, textstring3, strlen(textstring3));
          count2 += strlen(textstring3);
          break;
      }
    }
    else
      textstring2[count2++] = textstring[count];

    count++;
  }
  va_end(ap);

  textstring2[count2] = 0;

  if (textpixelsnap)
  {
    /* 1:1 path: 16x16 physical pixels per glyph, origin snapped to the pixel
       grid.  advx/szx/szy stay in design units so the maths below is unchanged. */
    float pxscale_x = (float)hudareaw / ((float)font.sizex * hudfrustumscale);
    float pxscale_y = (float)hudareah / (0.75f * (float)font.sizex);

    advx    = 16.0f / pxscale_x;
    szx     = advx;
    szy     = 16.0f / pxscale_y;
    texspan = 16.0f;
    texoff  = 0.0f;

    /* x and y still carry the TEXT_CENTER/TEXT_END flag bits here - the
       original path below strips them with x &= 0xFFFF, so mask them off
       before they become coordinates. */
    bx = (float)(x & 0xFFFF);
    by = (float)(y & 0xFFFF);
    if ((x & TEXT_CENTER) == TEXT_CENTER)
      bx -= (float)count2 * advx * 0.5f;
    if ((y & TEXT_CENTER) == TEXT_CENTER)
      by -= szy * 0.5f;
    if ((x & TEXT_END) == TEXT_END)
      bx -= (float)count2 * advx;
    if ((y & TEXT_END) == TEXT_END)
      by -= szy;

    bx = (floorf((bx - 0.5f * (float)font.sizex) * pxscale_x + 0.5f * (float)hudareaw + 0.5f) -
          0.5f * (float)hudareaw) /
             pxscale_x +
         0.5f * (float)font.sizex;
    by = floorf(by * pxscale_y + 0.5f) / pxscale_y;
  }
  else
  {
    if ((x & TEXT_CENTER) == TEXT_CENTER)
      x = x - (count2 * textsize) / 2;
    if ((y & TEXT_CENTER) == TEXT_CENTER)
      y = y - (textsize) / 2;
    if ((x & TEXT_END) == TEXT_END)
      x = x - (count2 * textsize);
    if ((y & TEXT_END) == TEXT_END)
      y = y - textsize;
    x &= 0xFFFF;
    y &= 0xFFFF;

    advx    = (float)textsize;
    szx     = (float)textsize;
    szy     = (float)textsize;
    texspan = 16.0f - 0.5f;
    texoff  = 0.5f;
    bx      = (float)x;
    by      = (float)y;
  }

  glBindTexture(GL_TEXTURE_2D, texture[1000 + font.texturenum].glname);
  useuitexture(1000 + font.texturenum);

  count = 0;
  while (textstring2[count] != 0)
  {
    if (textstring2[count] != 127)
    {
#if defined(GLES)
      GLfloat quad[12];
      GLfloat tex[8];
      glEnableClientState(GL_VERTEX_ARRAY);
      glEnableClientState(GL_TEXTURE_COORD_ARRAY);
#else
      glPushMatrix();
      glBegin(GL_QUADS);
#endif
      glColor4f(red, green, blue, alpha);

      vec[0] = bx + (float)count * advx;
      vec[1] = by;
      convertscreenvertex(vec, font.sizex, font.sizey);
      texcoord[0] = (float)(textstring2[count] & 15) * 16.0f + texoff;
      texcoord[1] = (float)((unsigned char)textstring2[count] >> 4) * 16.0f + texoff;
      texcoord[0] /= 256.0f;
      texcoord[1] /= 256.0f;
#if defined(GLES)
      quad[0] = vec[0];
      quad[1] = vec[1];
      quad[2] = vec[2];
      tex[0]  = texcoord[0];
      tex[1]  = texcoord[1];
#else
      glTexCoord2fv(texcoord);
      glVertex3fv(vec);
#endif
      vec[0] = bx + szx + (float)count * advx;
      vec[1] = by;
      convertscreenvertex(vec, font.sizex, font.sizey);
      texcoord[0] = (float)(textstring2[count] & 15) * 16.0f + texspan;
      texcoord[1] = (float)((unsigned char)textstring2[count] >> 4) * 16.0f + texoff;
      texcoord[0] /= 256.0f;
      texcoord[1] /= 256.0f;

#if defined(GLES)
      quad[3] = vec[0];
      quad[4] = vec[1];
      quad[5] = vec[2];
      tex[2]  = texcoord[0];
      tex[3]  = texcoord[1];
#else
      glTexCoord2fv(texcoord);
      glVertex3fv(vec);
#endif
      vec[0] = bx + szx + (float)count * advx;
      vec[1] = by + szy;
      convertscreenvertex(vec, font.sizex, font.sizey);
      texcoord[0] = (float)(textstring2[count] & 15) * 16.0f + texspan;
      texcoord[1] = (float)((unsigned char)textstring2[count] >> 4) * 16.0f + texspan;
      texcoord[0] /= 256.0f;
      texcoord[1] /= 256.0f;

#if defined(GLES)
      quad[6] = vec[0];
      quad[7] = vec[1];
      quad[8] = vec[2];
      tex[4]  = texcoord[0];
      tex[5]  = texcoord[1];
#else
      glTexCoord2fv(texcoord);
      glVertex3fv(vec);
#endif
      vec[0] = bx + (float)count * advx;
      vec[1] = by + szy;
      convertscreenvertex(vec, font.sizex, font.sizey);
      texcoord[0] = (float)(textstring2[count] & 15) * 16.0f + texoff;
      texcoord[1] = (float)((unsigned char)textstring2[count] >> 4) * 16.0f + texspan;
      texcoord[0] /= 256.0f;
      texcoord[1] /= 256.0f;
#if defined(GLES)
      quad[9]  = vec[0];
      quad[10] = vec[1];
      quad[11] = vec[2];
      tex[6]   = texcoord[0];
      tex[7]   = texcoord[1];
#else
      glTexCoord2fv(texcoord);
      glVertex3fv(vec);
#endif

#if defined(GLES)
      glVertexPointer(3, GL_FLOAT, 0, quad);
      glTexCoordPointer(2, GL_FLOAT, 0, tex);
      glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
      glDisableClientState(GL_TEXTURE_COORD_ARRAY);
      glDisableClientState(GL_VERTEX_ARRAY);
#else
      glEnd();
      glPopMatrix();
#endif
    }
    else
    {
      glDisable(GL_TEXTURE_2D);

#if defined(GLES)
      GLfloat quad[12];
      glEnableClientState(GL_VERTEX_ARRAY);
#else
      glPushMatrix();
      glBegin(GL_QUADS);
#endif
      glColor4f(red, green, blue, alpha);

      vec[0] = bx + (float)count * advx;
      vec[1] = by;
      convertscreenvertex(vec, font.sizex, font.sizey);
#if defined(GLES)
      quad[0] = vec[0];
      quad[1] = vec[1];
      quad[2] = vec[2];
#else
      glVertex3fv(vec);
#endif

      vec[0] = bx + szx + (float)count * advx;
      vec[1] = by;
      convertscreenvertex(vec, font.sizex, font.sizey);
#if defined(GLES)
      quad[3] = vec[0];
      quad[4] = vec[1];
      quad[5] = vec[2];
#else
      glVertex3fv(vec);
#endif
      vec[0] = bx + szx + (float)count * advx;
      vec[1] = by + szy;
      convertscreenvertex(vec, font.sizex, font.sizey);
#if defined(GLES)
      quad[6] = vec[0];
      quad[7] = vec[1];
      quad[8] = vec[2];
#else
      glVertex3fv(vec);
#endif
      vec[0] = bx + (float)count * advx;
      vec[1] = by + szy;
      convertscreenvertex(vec, font.sizex, font.sizey);
#if defined(GLES)
      quad[9]  = vec[0];
      quad[10] = vec[1];
      quad[11] = vec[2];
#else
      glVertex3fv(vec);
#endif

#if defined(GLES)
      glVertexPointer(3, GL_FLOAT, 0, quad);
      glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
      glDisableClientState(GL_VERTEX_ARRAY);
#else
      glEnd();
      glPopMatrix();
#endif
      glEnable(GL_TEXTURE_2D);
    }

    count++;
  }
}

void setuptextdisplay(void)
{
  glEnable(GL_TEXTURE_2D);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  /* In expand mode the HUD viewport is the whole screen and the frustum is
     widened by hudfrustumscale, so the 640x480 design space keeps its 4:3
     physical size and lands in the centred strip.  In every other mode
     hudfrustumscale is 1.0 and this is exactly what it always was. */
  glViewport(hudareax, hudareay, hudareaw, hudareah);

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glFrustum(-hudfrustumscale, hudfrustumscale, -0.75f, 0.75f, 1.0f, 1000.0f);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  glMatrixMode(GL_TEXTURE);
  glLoadIdentity();
}

void convertscreenvertex(float vec[3], int resolutionx, int resolutiony)
{
  vec[0] = (vec[0] - (float)(resolutionx >> 1)) / (float)(resolutionx >> 1);
  vec[1] = ((float)(resolutiony >> 1) - vec[1]) / (float)(resolutionx >> 1);
  vec[2] = -1.0f;
}

/* Draws a quad that maps the sub-rectangle (u0,v0)-(u1,v1) of the texture onto
   the screen rectangle (x,y)-(x+sizex,y+sizey).  u/v are 0..1 texture
   coordinates, so a tile can be drawn partially without resampling it.
   drawbackground() is this with the whole texture (0,0)-(1,1). */
void drawbackgroundsub(int texturenum, int x, int y, int sizex, int sizey, float u0, float v0, float u1, float v1,
                       int resolutionx, int resolutiony)
{
  float vec[3];

  if ((x & TEXT_CENTER) == TEXT_CENTER)
    x = x - (sizex) / 2;
  if ((y & TEXT_CENTER) == TEXT_CENTER)
    y = y - (sizey) / 2;
  if ((x & TEXT_END) == TEXT_END)
    x = x - (sizex);
  if ((y & TEXT_END) == TEXT_END)
    y = y - sizey;
  x &= 0xFFFF;
  y &= 0xFFFF;

  // printf("drawing bg %d, %x\n", texturenum, texture[texturenum].glname);
  glBindTexture(GL_TEXTURE_2D, texture[texturenum].glname);
  useuitexture(texturenum);
  // printf("bind %x\n", glGetError());

#if defined(GLES)
  GLfloat quad[12];
  GLfloat tex[] = {u0, v0, u1, v0, u1, v1, u0, v1};
  glEnableClientState(GL_VERTEX_ARRAY);
  // printf("enable client state %x\n", glGetError());
  glEnableClientState(GL_TEXTURE_COORD_ARRAY);
  // printf("enable client state %x\n", glGetError());
#else
  glBegin(GL_QUADS);
#endif
  vec[0] = (float)x;
  vec[1] = (float)y;
  convertscreenvertex(vec, resolutionx, resolutiony);
  // printf("converted screen\n");

#if defined(GLES)
  quad[0] = vec[0];
  quad[1] = vec[1];
  quad[2] = vec[2];
#else
  glTexCoord2f(u0, v0);
  glVertex3fv(vec);
#endif
  vec[0] = (float)(x + sizex);
  vec[1] = (float)y;
  convertscreenvertex(vec, resolutionx, resolutiony);
  // printf("converted screen\n");

#if defined(GLES)
  quad[3] = vec[0];
  quad[4] = vec[1];
  quad[5] = vec[2];
#else
  glTexCoord2f(u1, v0);
  glVertex3fv(vec);
#endif
  vec[0] = (float)(x + sizex);
  vec[1] = (float)(y + sizey);
  convertscreenvertex(vec, resolutionx, resolutiony);
  // printf("converted screen\n");
#if defined(GLES)
  quad[6] = vec[0];
  quad[7] = vec[1];
  quad[8] = vec[2];
#else
  glTexCoord2f(u1, v1);
  glVertex3fv(vec);
#endif
  vec[0] = (float)x;
  vec[1] = (float)(y + sizey);
  convertscreenvertex(vec, resolutionx, resolutiony);
  // printf("converted screen\n");
#if defined(GLES)
  quad[9]  = vec[0];
  quad[10] = vec[1];
  quad[11] = vec[2];
#else
  glTexCoord2f(u0, v1);
  glVertex3fv(vec);
#endif

#if defined(GLES)
  glVertexPointer(3, GL_FLOAT, 0, quad);
  // printf("vertex pointer %x\n", glGetError());
  glTexCoordPointer(2, GL_FLOAT, 0, tex);
  // printf("texcoord pointer %x\n", glGetError());
  glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
  // printf("draw arrays %x\n", glGetError());
  glDisableClientState(GL_TEXTURE_COORD_ARRAY);
  // printf("disable state %x\n", glGetError());
  glDisableClientState(GL_VERTEX_ARRAY);
  // printf("disable state %x\n", glGetError());
#else
  glEnd();
#endif
}

void drawbackground(int texturenum, int x, int y, int sizex, int sizey, int resolutionx, int resolutiony)
{
  drawbackgroundsub(texturenum, x, y, sizex, sizey, 0.0f, 0.0f, 1.0f, 1.0f, resolutionx, resolutiony);
}

void drawmousecursor(int texturenum, int x, int y, int textsize, float red, float green, float blue, float alpha)
{
#ifdef __vita__
  /* Vita port: no mouse cursor. */
  return;
#endif
  if (!touchcontrols)
  {
    float vec[3];

    glBindTexture(GL_TEXTURE_2D, texture[texturenum].glname);
    useuitexture(texturenum);

#if defined(GLES)
    GLfloat quad[12];
    GLfloat tex[] = {0, 0, 1, 0, 1, 1, 0, 1};
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
#else
    glPushMatrix();
    glBegin(GL_QUADS);
#endif
    vec[0] = (float)x - (float)textsize;
    vec[1] = (float)y - (float)textsize;
    convertscreenvertex(vec, font.sizex, font.sizey);

    glColor4f(red, green, blue, alpha);
#if defined(GLES)
    quad[0] = vec[0];
    quad[1] = vec[1];
    quad[2] = vec[2];
#else
    glTexCoord2f(0.0f, 0.0f);
    glVertex3fv(vec);
#endif
    vec[0] = (float)x + (float)textsize;
    vec[1] = (float)y - (float)textsize;
    convertscreenvertex(vec, font.sizex, font.sizey);
#if defined(GLES)
    quad[3] = vec[0];
    quad[4] = vec[1];
    quad[5] = vec[2];
#else
    glTexCoord2f(1.0f, 0.0f);
    glVertex3fv(vec);
#endif
    vec[0] = (float)x + (float)textsize;
    vec[1] = (float)y + (float)textsize;
    convertscreenvertex(vec, font.sizex, font.sizey);
#if defined(GLES)
    quad[6] = vec[0];
    quad[7] = vec[1];
    quad[8] = vec[2];
#else
    glTexCoord2f(1.0f, 1.0f);
    glVertex3fv(vec);
#endif
    vec[0] = (float)x - (float)textsize;
    vec[1] = (float)y + (float)textsize;
    convertscreenvertex(vec, font.sizex, font.sizey);
#if defined(GLES)
    quad[9]  = vec[0];
    quad[10] = vec[1];
    quad[11] = vec[2];
#else
    glTexCoord2f(0.0f, 1.0f);
    glVertex3fv(vec);
#endif

#if defined(GLES)
    glVertexPointer(3, GL_FLOAT, 0, quad);
    glTexCoordPointer(2, GL_FLOAT, 0, tex);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
#else
    glEnd();
    glPopMatrix();
#endif
  }
}

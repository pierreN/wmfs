/*
*      bar.c
*      Copyright © 2008 Martin Duquesnoy <xorg62@gmail.com>
*      All rights reserved.
*
*      Redistribution and use in source and binary forms, with or without
*      modification, are permitted provided that the following conditions are
*      met:
*
*      * Redistributions of source code must retain the above copyright
*        notice, this list of conditions and the following disclaimer.
*      * Redistributions in binary form must reproduce the above
*        copyright notice, this list of conditions and the following disclaimer
*        in the documentation and/or other materials provided with the
*        distribution.
*      * Neither the name of the  nor the names of its
*        contributors may be used to endorse or promote products derived from
*        this software without specific prior written permission.
*
*      THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
*      "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
*      LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
*      A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
*      OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
*      SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
*      LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
*      DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
*      THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
*      (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
*      OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "wmfs.h"

BarWindow*
bar_create(int x, int y, uint w, uint h, int bord, uint color, Bool entermask)
{
     XSetWindowAttributes at;
     BarWindow *bw;

     bw = emalloc(1, sizeof(BarWindow));

     at.override_redirect = 1;
     at.background_pixmap = ParentRelative;
     if(entermask)
          at.event_mask = ButtonPressMask | ExposureMask | EnterWindowMask;
     else
          at.event_mask = ButtonPressMask | ExposureMask;

     bw->dr = XCreatePixmap(dpy, root, w, h, DefaultDepth(dpy, screen));
     bw->win = XCreateWindow(dpy, root, x, y, w, h, bord, DefaultDepth(dpy, screen),
                             CopyFromParent, DefaultVisual(dpy, screen),
                             CWOverrideRedirect | CWBackPixmap | CWEventMask, &at);

     bw->x = x; bw->y = y;
     bw->w = w; bw->h = h;
     bw->bord = bord;
     bw->color = color;

     return bw;
}

void
bar_delete(BarWindow *bw)
{
     XDestroyWindow(dpy, bw->win);
     XFreePixmap(dpy, bw->dr);
     free(bw);

     return;
}

void
bar_moveresize(BarWindow *bw, int x, int y, uint w, uint h)
{
     if(w != bw->w || h != bw->h)
     {
          XFreePixmap(dpy, bw->dr);
          bw->dr = XCreatePixmap(dpy, root, w, h, DefaultDepth(dpy, screen));
          bw->w = w; bw->h = h;
     }

     if(x != bw->x || y != bw->y)
     {
          bw->x = x;
          bw->y = y;
     }

     XMoveResizeWindow(dpy, bw->win, x, y, w, h);

     return;
}

void
bar_refresh_color(BarWindow *bw)
{
     draw_rectangle(bw->dr, 0, 0, bw->w, bw->h, bw->color);

     return;
}

void
bar_refresh(BarWindow *bw)
{
     XCopyArea(dpy, bw->dr, bw->win, gc, 0, 0, bw->w, bw->h, 0, 0);

     return;
}

/* Top/Bottom Bar Manage Function */
void
updatebar(void)
{
     /* Refresh bar color */
     bar_refresh_color(bar);

     /* Draw taglist */
     draw_taglist(bar->dr);

     /* Draw layout symbol */
     draw_layout(taglen[conf.ntag], bary + (!conf.bartop));

     /* Draw status text */
     draw_text(bar->dr, mw - textw(bartext), fonth, conf.colors.text, conf.colors.bar, 0, bartext);

     /* Bar border */
     if(conf.tagbordwidth)
     {
          draw_rectangle(bar->dr, 0, ((conf.bartop) ? barheight-1: 0),
                         mw, 1, conf.colors.tagbord);
          draw_rectangle(bar->dr, mw - textw(bartext) - 5,
                         0, conf.tagbordwidth, barheight, conf.colors.tagbord);
     }

     /* Refresh the bar */
     bar_refresh(bar);

     /* Update Bar Buttons */
     updatebutton(True);

     return;
}

/* BARBUTTON MANAGE FUNCTION
 * if c is False, you can execute this function for the first time
 * else the button is just updated *TestingButWorking* */
void
updatebutton(Bool c)
{
     int i, j, x, pm = 0, buttonw = 0;
     int y = 0, hi = 0;

     /* Calcul the position of the first button with the layout image size */
     j = taglen[conf.ntag] + get_image_attribute(tags[seltag].layout.image)->width + PAD / 2;

     if(!conf.bartop)
          y = bary + 1;

     if(conf.tagbordwidth)
          hi = -1;

     for(i = 0; i < conf.nbutton; ++i)
     {

          /* CALCUL POSITION */
          {
               if(!(x = conf.barbutton[i].x))
               {
                    if(i)
                         pm += (conf.barbutton[i-1].type) ?
                              get_image_attribute(conf.barbutton[i-1].content)->width :
                              textw(conf.barbutton[i-1].content) + BPAD;

                         x = (!i) ? j : j + pm;
               }

               buttonw = (conf.barbutton[i].type) ?
                    get_image_attribute(conf.barbutton[i].content)->width :
                    textw(conf.barbutton[i].content) + BPAD;
          }


          /* FIRST TIME */
          {
               if(!c)
               {
                    conf.barbutton[i].bw = bar_create(x, y, buttonw, barheight + hi, 0,
                                                      conf.barbutton[i].bg_color, False);
                    XMapRaised(dpy, conf.barbutton[i].bw->win);
               }
          }

          /* REFRESH/DRAW TEXT/IMAGE */
          {
               if(!conf.barbutton[i].bw)
                    return;
               if(!conf.barbutton[i].type)
                    bar_refresh_color(conf.barbutton[i].bw);
               bar_moveresize(conf.barbutton[i].bw, x, y, buttonw, barheight + hi);

               /* Check the button type (image/text) */
               if(conf.barbutton[i].type)
                    draw_image(conf.barbutton[i].bw->dr, 0, 0,
                               conf.barbutton[i].content);
               else
                    draw_text(conf.barbutton[i].bw->dr, BPAD/2, fonth, conf.barbutton[i].fg_color,
                              conf.barbutton[i].bg_color, BPAD, conf.barbutton[i].content);

               /* Refresh button */
               bar_refresh(conf.barbutton[i].bw);
          }
     }
     XSync(dpy, False);

     return;
}

void
uicb_togglebarpos(uicb_t cmd)
{
     int i;

     conf.bartop = !conf.bartop;
     if(conf.bartop)
          bary = 0;
     else
          bary = mh - barheight;
     bar_moveresize(bar, 0, bary, mw, barheight);
     updatebar();
     for(i = 0; i < conf.nbutton; ++i)
          XUnmapWindow(dpy, conf.barbutton[i].bw->win);
     updatebutton(False);
     for(i = 0; i < conf.nbutton; ++i)
          XMapWindow(dpy, conf.barbutton[i].bw->win);

     arrange();

     return;
}

void
updatetitlebar(Client *c)
{
     XFetchName(dpy, c->win, &(c->title));
     if(!c->title)
          c->title = strdup("WMFS");

     if(conf.ttbarheight > 10)
     {
          bar_refresh_color(c->tbar);
          draw_text(c->tbar->dr, 3, ((fonth - xftfont->descent) + ((conf.ttbarheight - fonth) / 2)),
                 ((c == sel) ? conf.colors.ttbar_text_focus : conf.colors.ttbar_text_normal),
                 conf.colors.bar, 0, c->title);
          bar_refresh(c->tbar);
     }

     return;
}
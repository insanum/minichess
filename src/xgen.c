/*
 * "$Id: xgen.c,v 1.6 1999/07/29 05:00:50 edavis Exp $"
 *
 *  General X routines...
 *
 *  This code taken from wmgeneral which is found in many dock apps.
 *  Original author:    Martijn Pieterse (pieterse@xs4all.nl)
 *  Modified by:        Eric Davis (ead@pobox.com)
 */

/*
 * miniCHESS - another stupid dock app
 * Copyright (c) 1999 by Eric Davis, ead@pobox.com
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 * 
 * (See COPYING / GPL-2.0)
 */

#include <stdio.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/xpm.h>
#include <X11/extensions/shape.h>

#include "xgen.h"

/* typedefs */

typedef struct
    {
    Pixmap              pixmap;
    Pixmap              mask;
    XpmAttributes       attributes;
    } XpmIcon;

/* globals */

Display         *display;

/* locals */

static Window          Root;
static Window          iconwin;
static Window          win;
static GC              NormalGC;
static XpmIcon         wmgen;


/* getXPM() */
static void GetXPM
    (
    XpmIcon *   wmgen,
    char *      pixmap_bytes[]
    )
    {
    XWindowAttributes   attributes;

    /* For the colormap */
    XGetWindowAttributes(display, Root, &attributes);
    wmgen->attributes.valuemask |= (XpmReturnPixels | XpmReturnExtensions);
    if (XpmCreatePixmapFromData(display, Root, pixmap_bytes,
                                &(wmgen->pixmap), &(wmgen->mask),
                                &(wmgen->attributes)) != XpmSuccess)
        {
        fprintf(stderr, "Not enough free colorcells.\n");
        exit(1);
        }
    }


/* GetColor() */
static Pixel GetColor
    (
    char * name
    )
    {
    XColor              color;
    XWindowAttributes   attributes;

    XGetWindowAttributes(display, Root, &attributes);
    color.pixel = 0;

    if (!XParseColor(display, attributes.colormap, name, &color))
        fprintf(stderr, "Can't parse %s.\n", name);
    else if (!XAllocColor(display, attributes.colormap, &color))
        fprintf(stderr, "Can't allocate %s.\n", name);

    return color.pixel;
    }


/* flush_expose() */
static int flush_expose
    (
    Window w
    )
    {
    XEvent      dummy;
    int         i = 0;
    while (XCheckTypedWindowEvent(display, w, Expose, &dummy)) i++;
    return i;
    }


/* RedrawWindow() */
void RedrawWindow
    (
    void
    )
    {
    flush_expose(iconwin);
    XCopyArea(display, wmgen.pixmap, iconwin, NormalGC, 0, 0,
              wmgen.attributes.width, wmgen.attributes.height, 0, 0);

    flush_expose(win);
    XCopyArea(display, wmgen.pixmap, win, NormalGC, 0, 0,
              wmgen.attributes.width, wmgen.attributes.height, 0, 0);
    }


/* copyXPMArea() */
void copyXPMArea
    (
    int  x, int  y,
    int sx, int sy,
    int dx, int dy
    )
    {
    XCopyArea(display, wmgen.pixmap, wmgen.pixmap, NormalGC,
              x, y, sx, sy, dx, dy);
    }


/* openXwindow() */
void openXwindow
    (
    int argc,
    char *argv[],
    char *pixmap_bytes[],
    char *pixmask_bits,
    int pixmask_width,
    int pixmask_height
    )
    {
    unsigned int        borderwidth = 1;
    XClassHint          classHint;
    char               *display_name = NULL;
    char               *wname = argv[0];
    XTextProperty       name;
    XGCValues           gcv;
    unsigned long       gcm;
    int                 screen;
    XSizeHints          mysizehints;
    XWMHints            mywmhints;
    Pixel               back_pix;
    Pixel               fore_pix;
    char               *Geometry = "";
    Pixmap              pixmask;
    int                 dummy = 0;
    int                 i;

    for (i = 1; argv[i]; i++)
        {
        if (!strcmp(argv[i], "-display")) display_name = argv[i+1];
        }

    if (!(display = XOpenDisplay(display_name)))
        {
        fprintf(stderr, "%s: can't open display %s\n",
                wname, XDisplayName(display_name));
        exit(1);
        }

    screen  = DefaultScreen(display);
    Root    = RootWindow(display, screen);

    /* Convert XPM to XImage */
    GetXPM(&wmgen, pixmap_bytes);

    /* Create a window to hold the stuff */
    mysizehints.flags = USSize | USPosition;
    mysizehints.x = 0;
    mysizehints.y = 0;

    back_pix = GetColor("white");
    fore_pix = GetColor("black");

    XWMGeometry(display, screen, Geometry, NULL, borderwidth,
                &mysizehints, &mysizehints.x, &mysizehints.y,
                &mysizehints.width, &mysizehints.height, &dummy);

    mysizehints.width = 64;
    mysizehints.height = 64;

    win = XCreateSimpleWindow(display, Root, mysizehints.x, mysizehints.y,
                              mysizehints.width, mysizehints.height,
                              borderwidth, fore_pix, back_pix);

    iconwin = XCreateSimpleWindow(display, win, mysizehints.x, mysizehints.y,
                                  mysizehints.width, mysizehints.height,
                                  borderwidth, fore_pix, back_pix);

    /* Activate hints */
    XSetWMNormalHints(display, win, &mysizehints);
    classHint.res_name = wname;
    classHint.res_class = wname;
    XSetClassHint(display, win, &classHint);

    XSelectInput(display, win,
                 (ButtonPressMask | ExposureMask | ButtonReleaseMask |
                  PointerMotionMask | StructureNotifyMask));
    XSelectInput(display, iconwin,
                 (ButtonPressMask | ExposureMask | ButtonReleaseMask |
                  PointerMotionMask | StructureNotifyMask));

    if (XStringListToTextProperty(&wname, 1, &name) == 0)
        {
        fprintf(stderr, "%s: can't allocate window name\n", wname);
        exit(1);
        }

    XSetWMName(display, win, &name);

    /* Create GC for drawing */

    gcm = (GCForeground | GCBackground | GCGraphicsExposures);
    gcv.foreground = fore_pix;
    gcv.background = back_pix;
    gcv.graphics_exposures = 0;
    NormalGC = XCreateGC(display, Root, gcm, &gcv);

    /* ONLYSHAPE ON */

    pixmask = XCreateBitmapFromData(display, win, pixmask_bits,
                                    pixmask_width, pixmask_height);

    XShapeCombineMask(display, win, ShapeBounding, 0, 0, pixmask, ShapeSet);
    XShapeCombineMask(display, iconwin, ShapeBounding, 0, 0, pixmask, ShapeSet);

    /* ONLYSHAPE OFF */

    mywmhints.initial_state = WithdrawnState;
    mywmhints.icon_window = iconwin;
    mywmhints.icon_x = mysizehints.x;
    mywmhints.icon_y = mysizehints.y;
    mywmhints.window_group = win;
    mywmhints.flags = (StateHint | IconWindowHint | IconPositionHint |
                       WindowGroupHint);

    XSetWMHints(display, win, &mywmhints);

    XSetCommand(display, win, argv, argc);
    XMapWindow(display, win);

    }


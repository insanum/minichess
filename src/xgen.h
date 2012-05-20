/*
 * "$Id: xgen.h,v 1.5 1999/07/29 05:00:50 edavis Exp $"
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

#ifndef __INCxgenh
#define __INCxgenh

void RedrawWindow(void);
void copyXPMArea(int, int, int, int, int, int);
void openXwindow(int argc, char *argv[], char **, char *, int, int);

#endif /* __INCxgenh */


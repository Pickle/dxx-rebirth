/*
THE COMPUTER CODE CONTAINED HEREIN IS THE SOLE PROPERTY OF PARALLAX
SOFTWARE CORPORATION ("PARALLAX").  PARALLAX, IN DISTRIBUTING THE CODE TO
END-USERS, AND SUBJECT TO ALL OF THE TERMS AND CONDITIONS HEREIN, GRANTS A
ROYALTY-FREE, PERPETUAL LICENSE TO SUCH END-USERS FOR USE BY SUCH END-USERS
IN USING, DISPLAYING,  AND CREATING DERIVATIVE WORKS THEREOF, SO LONG AS
SUCH USE, DISPLAY OR CREATION IS FOR NON-COMMERCIAL, ROYALTY OR REVENUE
FREE PURPOSES.  IN NO EVENT SHALL THE END-USER USE THE COMPUTER CODE
CONTAINED HEREIN FOR REVENUE-BEARING PURPOSES.  THE END-USER UNDERSTANDS
AND AGREES TO THE TERMS HEREIN AND ACCEPTS THE SAME BY USE OF THIS FILE.
COPYRIGHT 1993-1998 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

/*
 *
 * Internal definitions for graphics lib.
 *
 */

#define MINX    0
#define MINY    0
#define MAXX    (GWIDTH-1)
#define MAXY    (GHEIGHT-1)
#define TYPE    grd_curcanv->cv_bitmap.bm_type
#define DATA    grd_curcanv->cv_bitmap.bm_data
#define XOFFSET grd_curcanv->cv_bitmap.bm_x
#define YOFFSET grd_curcanv->cv_bitmap.bm_y
#define ROWSIZE grd_curcanv->cv_bitmap.bm_rowsize
#define COLOR   grd_curcanv->cv_color

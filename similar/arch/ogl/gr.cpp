/*
 *
 * OGL video functions. - Added 9/15/99 Matthew Mueller
 *
 */

#define DECLARE_VARS

#ifdef RPI
// extra libraries for the Raspberry Pi
#include  "bcm_host.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifdef _MSC_VER
#include <windows.h>
#endif

#if !defined(_MSC_VER) && !defined(macintosh)
#include <unistd.h>
#endif
#if !defined(macintosh)
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#endif

#include <algorithm>
#include <errno.h>
#include <SDL.h>
#include "hudmsg.h"
#include "game.h"
#include "text.h"
#include "gr.h"
#include "gamefont.h"
#include "grdef.h"
#include "palette.h"
#include "u_mem.h"
#include "dxxerror.h"
#include "inferno.h"
#include "screens.h"
#include "strutil.h"
#include "args.h"
#include "key.h"
#include "physfsx.h"
#include "internal.h"
#include "render.h"
#include "console.h"
#include "config.h"
#include "playsave.h"
#include "vers_id.h"
#include "game.h"
#if defined(OGL1) || defined(OGL2) 
#include "ogl.h"
#endif

using std::max;

GLuint ProgramCurrent = PROGRAM_NONE;
glstate_t GLState;
CShader Shaders[PROGRAM_TOTAL];
//CBuffer Buffer[BUFFER_TOTAL];
vector<mat44f_t>    ModelView;
mat44f_t            Projection;
mat44f_t            ProjPersp;
mat44f_t            ProjOrtho;

int sdl_video_flags = SDL_OPENGL;
int gr_installed = 0;
int gl_initialized=0;
int linedotscale=1; // scalar of glLinewidth and glPointSize - only calculated once when resolution changes
#ifdef RPI
int sdl_no_modeswitch=0;
#else
enum { sdl_no_modeswitch = 0 };
#endif

void ogl_get_verinfo(void);

void ogl_swap_buffers_internal(void)
{
#ifdef OGLES
	eglSwapBuffers(eglDisplay, eglSurface);
#else
	SDL_GL_SwapBuffers();
#endif
}

int ogl_init_window(int x, int y)
{
	int use_x,use_y,use_bpp;
	Uint32 use_flags;

	if (gl_initialized)
		ogl_smash_texture_list_internal();//if we are or were fullscreen, changing vid mode will invalidate current textures

	SDL_WM_SetCaption(DESCENT_VERSION, DXX_SDL_WINDOW_CAPTION);
	SDL_WM_SetIcon( SDL_LoadBMP( DXX_SDL_WINDOW_ICON_BITMAP ), NULL );

	use_x=x;
	use_y=y;
	use_bpp=GameArg.DbgBpp;
	use_flags=sdl_video_flags;
	if (sdl_no_modeswitch) {
		const SDL_VideoInfo *vinfo=SDL_GetVideoInfo();
		if (vinfo) {
			use_x=vinfo->current_w;
			use_y=vinfo->current_h;
			use_bpp=vinfo->vfmt->BitsPerPixel;
			use_flags=SDL_SWSURFACE | SDL_ANYFORMAT;
		} else {
			con_printf(CON_URGENT, "Could not query video info\n");
		}
	}

	if (!SDL_SetVideoMode(use_x, use_y, use_bpp, use_flags))
	{
#ifdef RPI
		con_printf(CON_URGENT, "Could not set %dx%dx%d opengl video mode: %s\n (Ignored for RPI)",
			    x, y, GameArg.DbgBpp, SDL_GetError());
#else
		Error("Could not set %dx%dx%d opengl video mode: %s\n", x, y, GameArg.DbgBpp, SDL_GetError());
#endif
	}

#ifdef OGLES
	EGL_Open();
#endif

	linedotscale = ((x/640<y/480?x/640:y/480)<1?1:(x/640<y/480?x/640:y/480));

	gl_initialized=1;
	return 0;
}

int gr_check_fullscreen(void)
{
	return (sdl_video_flags & SDL_FULLSCREEN)?1:0;
}

int gr_toggle_fullscreen(void)
{
	if (sdl_video_flags & SDL_FULLSCREEN)
		sdl_video_flags &= ~SDL_FULLSCREEN;
	else
		sdl_video_flags |= SDL_FULLSCREEN;

	if (gl_initialized)
	{
		if (sdl_no_modeswitch == 0) {
			if (!SDL_VideoModeOK(SM_W(Game_screen_mode), SM_H(Game_screen_mode), GameArg.DbgBpp, sdl_video_flags))
			{
				con_printf(CON_URGENT,"Cannot set %ix%i. Fallback to 640x480\n",SM_W(Game_screen_mode), SM_H(Game_screen_mode));
				Game_screen_mode=SM(640,480);
			}
			if (!SDL_SetVideoMode(SM_W(Game_screen_mode), SM_H(Game_screen_mode), GameArg.DbgBpp, sdl_video_flags))
			{
				Error("Could not set %dx%dx%d opengl video mode: %s\n", SM_W(Game_screen_mode), SM_H(Game_screen_mode), GameArg.DbgBpp, SDL_GetError());
			}
		}
#ifdef RPI
		if (rpi_setup_element(SM_W(Game_screen_mode), SM_H(Game_screen_mode), sdl_video_flags, 1)) {
			 Error("RPi: Could not set up %dx%d element\n", SM_W(Game_screen_mode), SM_H(Game_screen_mode));
		}
#endif
	}

	gr_remap_color_fonts();
	gr_remap_mono_fonts();

	if (gl_initialized) // update viewing values for menus
	{
#if defined(OGL1)
	glMatrixMode( GL_PROJECTION );
	glLoadMatrixf( ProjOrtho.data() );
#endif
#if defined(OGL2)    
        Projection = ProjOrtho;
#endif		
        ModelView.back() = cml::identity_4x4();

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		ogl_smash_texture_list_internal();//if we are or were fullscreen, changing vid mode will invalidate current textures
	}
	GameCfg.WindowMode = (sdl_video_flags & SDL_FULLSCREEN)?0:1;
	return (sdl_video_flags & SDL_FULLSCREEN)?1:0;
}

static void ogl_init_state(void)
{
#if defined(OGL2)
	ogl2_shader_compile();
#endif
	
	GLState.mode = 0;
	GLState.view = 0;
	GLState.count = 0;
	GLState.stride = 0;
	GLState.pointsize = 1.0f;
	GLState.tex[0] = -1;
	GLState.tex[1] = -1;
	GLState.texwrap[0] = GL_CLAMP_TO_EDGE; 
	GLState.texwrap[1] = GL_CLAMP_TO_EDGE; 
	GLState.v_pos = NULL;
	GLState.v_clr = NULL;
	GLState.v_tex[0] = NULL;
	GLState.v_tex[1] = NULL;
	
	/* select clearing (background) color   */
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

	/* initialize viewing values */
	/* parameters: matrix, xfov, aspect, near, far, z_clip */
	cml::matrix_perspective_xfov_RH( ProjPersp,
				    cml::rad(90.0f), 1.0f,
				    0.1f, 5000.0f, cml::z_clip_neg_one );

	/* parameters: matrix, left, right, bottom, top, near, far, z_clip */
	cml::matrix_orthographic_RH( ProjOrtho,
				    0.0f, 1.0f,
				    0.0f, 1.0f,
				    -1.0f, 1.0f, cml::z_clip_neg_one );

#if defined(OGL1)
	glMatrixMode( GL_PROJECTION );
	glLoadMatrixf( ProjOrtho.data() );
#endif
#if defined(OGL2)    
        Projection = ProjOrtho;
#endif		

	ModelView.push_back( cml::identity_4x4() );

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	gr_palette_step_up(0,0,0);//in case its left over from in game

	ogl_init_pixel_buffers(grd_curscreen->sc_w, grd_curscreen->sc_h);
}

const char *gl_vendor, *gl_renderer, *gl_version, *gl_extensions, *gl_shaderversion;
int max_texture_units;

void ogl_get_verinfo(void)
{
	gl_vendor = (const char *) glGetString (GL_VENDOR);
	gl_renderer = (const char *) glGetString (GL_RENDERER);
	gl_version = (const char *) glGetString (GL_VERSION);
	gl_extensions = (const char *) glGetString (GL_EXTENSIONS);
	con_printf(CON_VERBOSE, "OpenGL: vendor: %s\nOpenGL: renderer: %s\nOpenGL: version: %s\n",gl_vendor,gl_renderer,gl_version);
	
	glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &max_texture_units);
	con_printf(CON_VERBOSE, "OpenGL: max texture units: %d\n",max_texture_units);	
#if defined(OGL2)
	gl_shaderversion = (const char *) glGetString( GL_SHADING_LANGUAGE_VERSION );
	con_printf(CON_VERBOSE, "OpenGL: shader version: %s\n",gl_shaderversion);
#endif	

	if (!d_stricmp(gl_extensions,"GL_EXT_texture_filter_anisotropic")==0)
	{
		glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &ogl_maxanisotropy);
		con_printf(CON_VERBOSE,"ogl_maxanisotropy:%f\n",ogl_maxanisotropy);
	}
	else if (GameCfg.TexFilt >= 3)
		GameCfg.TexFilt = 2;
}

// returns possible (fullscreen) resolutions if any.
int gr_list_modes( u_int32_t gsmodes[] )
{
	SDL_Rect** modes;
	int i = 0, modesnum = 0;
#ifdef OGLES
	int sdl_check_flags = SDL_FULLSCREEN; // always use Fullscreen as lead.
#else
	int sdl_check_flags = SDL_OPENGL | SDL_FULLSCREEN; // always use Fullscreen as lead.
#endif

	if (sdl_no_modeswitch) {
		/* TODO: we could use the tvservice to list resolutions on the RPi */
		return 0;
	}

	modes = SDL_ListModes(NULL, sdl_check_flags);

	if (modes == (SDL_Rect**)0) // check if we get any modes - if not, return 0
		return 0;


	if (modes == (SDL_Rect**)-1)
	{
		return 0; // can obviously use any resolution... strange!
	}
	else
	{
		for (i = 0; modes[i]; ++i)
		{
			if (modes[i]->w > 0xFFF0 || modes[i]->h > 0xFFF0 // resolutions saved in 32bits. so skip bigger ones (unrealistic in 2010) (changed to 0xFFF0 to fix warning)
				|| modes[i]->w < 320 || modes[i]->h < 200) // also skip everything smaller than 320x200
				continue;
			gsmodes[modesnum] = SM(modes[i]->w,modes[i]->h);
			modesnum++;
			if (modesnum >= 50) // that really seems to be enough big boy.
				break;
		}
		return modesnum;
	}
}

int gr_check_mode(u_int32_t mode)
{
	unsigned int w, h;

	w=SM_W(mode);
	h=SM_H(mode);

	if (sdl_no_modeswitch == 0) {
		return SDL_VideoModeOK(w, h, GameArg.DbgBpp, sdl_video_flags);
	} else {
		// just tell the caller that any mode is valid...
		return 32;
	}
}

int gr_set_mode(u_int32_t mode)
{
	unsigned int w, h;
	unsigned char *gr_bm_data;

	if (mode<=0)
		return 0;

	w=SM_W(mode);
	h=SM_H(mode);

	if (!gr_check_mode(mode))
	{
		con_printf(CON_URGENT,"Cannot set %ix%i. Fallback to 640x480\n",w,h);
		w=640;
		h=480;
		Game_screen_mode=mode=SM(w,h);
	}

	gr_bm_data=grd_curscreen->sc_canvas.cv_bitmap.bm_data;//since we use realloc, we want to keep this pointer around.
	unsigned char *gr_new_bm_data = (unsigned char *)d_realloc(gr_bm_data,w*h);
	if (!gr_new_bm_data)
		return 0;
	memset( grd_curscreen, 0, sizeof(grs_screen));
	grd_curscreen->sc_mode = mode;
	grd_curscreen->sc_w = w;
	grd_curscreen->sc_h = h;
	grd_curscreen->sc_aspect = fixdiv(grd_curscreen->sc_w*GameCfg.AspectX,grd_curscreen->sc_h*GameCfg.AspectY);
	gr_init_canvas(&grd_curscreen->sc_canvas, gr_new_bm_data, BM_OGL, w, h);
	gr_set_current_canvas(NULL);

	ogl_init_window(w,h);//platform specific code
	ogl_get_verinfo();
	OGL_VIEWPORT(0,0,w,h);
	ogl_init_state();
	gamefont_choose_game_font(w,h);
	gr_remap_color_fonts();
	gr_remap_mono_fonts();

	return 0;
}

#define GLstrcmptestr(a,b) if (d_stricmp(a,#b)==0 || d_stricmp(a,"GL_" #b)==0)return GL_ ## b;

#ifdef _WIN32
static const char OglLibPath[]="opengl32.dll";

static int ogl_rt_loaded=0;
static int ogl_init_load_library(void)
{
	int retcode=0;
	if (!ogl_rt_loaded)
	{
		retcode = OpenGL_LoadLibrary(true, OglLibPath);
		if(retcode)
		{
			if(!glEnd)
			{
				Error("Opengl: Functions not imported\n");
			}
		}
		else
		{
			Error("Opengl: error loading %s\n", OglLibPath);
		}
		ogl_rt_loaded=1;
	}
	return retcode;
}
#endif

void gr_set_attributes(void)
{
#ifndef OGLES
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE,0);
	SDL_GL_SetAttribute(SDL_GL_ACCUM_RED_SIZE,0);
	SDL_GL_SetAttribute(SDL_GL_ACCUM_GREEN_SIZE,0);
	SDL_GL_SetAttribute(SDL_GL_ACCUM_BLUE_SIZE,0);
	SDL_GL_SetAttribute(SDL_GL_ACCUM_ALPHA_SIZE,0);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER,1);
	SDL_GL_SetAttribute(SDL_GL_SWAP_CONTROL,GameCfg.VSync);
	if (GameCfg.Multisample)
	{
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);
	}
	else
	{
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 0);
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 0);
	}
#endif
	ogl_smash_texture_list_internal();
	gr_remap_color_fonts();
	gr_remap_mono_fonts();
}

int gr_init(int mode)
{
    int retcode;

	// Only do this function once!
	if (gr_installed==1)
		return -1;

#ifdef _WIN32
	ogl_init_load_library();
#endif

	if (!GameCfg.WindowMode && !GameArg.SysWindow)
		sdl_video_flags|=SDL_FULLSCREEN;

	if (GameArg.SysNoBorders)
		sdl_video_flags|=SDL_NOFRAME;

	gr_set_attributes();

	ogl_init_texture_list_internal();

	CALLOC( grd_curscreen,grs_screen,1 );
	grd_curscreen->sc_canvas.cv_bitmap.bm_data = NULL;

	// Set the mode.
	if ((retcode=gr_set_mode(mode)))
		return retcode;

	grd_curscreen->sc_canvas.cv_color = 0;
	grd_curscreen->sc_canvas.cv_fade_level = GR_FADE_OFF;
	grd_curscreen->sc_canvas.cv_blend_func = GR_BLEND_NORMAL;
	grd_curscreen->sc_canvas.cv_drawmode = 0;
	grd_curscreen->sc_canvas.cv_font = NULL;
	grd_curscreen->sc_canvas.cv_font_fg_color = 0;
	grd_curscreen->sc_canvas.cv_font_bg_color = 0;
	gr_set_current_canvas( &grd_curscreen->sc_canvas );

	ogl_init_pixel_buffers(256, 128);       // for gamefont_init

	gr_installed = 1;

	return 0;
}

void gr_close()
{
	ogl_brightness_r = ogl_brightness_g = ogl_brightness_b = 0;

	if (gl_initialized)
	{
		ogl_smash_texture_list_internal();
	}

	if (grd_curscreen)
	{
		if (grd_curscreen->sc_canvas.cv_bitmap.bm_data)
			d_free(grd_curscreen->sc_canvas.cv_bitmap.bm_data);
		d_free(grd_curscreen);
	}
	ogl_close_pixel_buffers();
#ifdef _WIN32
	if (ogl_rt_loaded)
		OpenGL_LoadLibrary(false);
#endif

#if defined(OGL2)
	Shaders[PROGRAM_COLOR].Close();
	Shaders[PROGRAM_COLOR_UNIFORM].Close();
	Shaders[PROGRAM_TEXTURE].Close();
	Shaders[PROGRAM_TEXTURE_UNIFORM].Close();
	Shaders[PROGRAM_MULTITEXTURE].Close();
#endif

#ifdef OGLES
	EGL_Close();
#endif
}

void ogl_upixelc(int x, int y, int c)
{
    vec2f_t v[2];

    /* Setup vertex array */
    v[0][0] = (x+grd_curcanv->cv_bitmap.bm_x)/(float)last_width;
    v[0][1] = 1.0-(y+grd_curcanv->cv_bitmap.bm_y)/(float)last_height;
    
    GLState.color4f[0] = CPAL2Tr(c);
    GLState.color4f[1] = CPAL2Tg(c);
    GLState.color4f[2] = CPAL2Tb(c);
    GLState.color4f[3] = 1.0f;
    
    GLState.mode = GL_POINTS;
    GLState.view = POSITION_XY;
    GLState.count = 1;
    GLState.stride = 0;
    GLState.pointsize = linedotscale;
    GLState.v_pos = &v[0][0];

    ogl_draw_arrays( GLState );
}

unsigned char ogl_ugpixel( grs_bitmap * bitmap, int x, int y )
{
	ubyte buf[4];

	glReadPixels(bitmap->bm_x + x, SHEIGHT - bitmap->bm_y - y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, buf);

	return gr_find_closest_color(buf[0]/4, buf[1]/4, buf[2]/4);
}

void ogl_urect(int left,int top,int right,int bot)
{
    vec2f_t v[4];
    GLfloat xo, yo, xf, yf, color_a;

    /* Setup vertex array */
    xo = (left+grd_curcanv->cv_bitmap.bm_x)/(float)last_width;
    xf = (right + 1 + grd_curcanv->cv_bitmap.bm_x) / (float)last_width;
    yo = 1.0-(top+grd_curcanv->cv_bitmap.bm_y)/(float)last_height;
    yf = 1.0 - (bot + 1 + grd_curcanv->cv_bitmap.bm_y) / (float)last_height;

    if (grd_curcanv->cv_fade_level >= GR_FADE_OFF)
	    color_a = 1.0f;
    else
	    color_a = 1.0f - (float)grd_curcanv->cv_fade_level / ((float)GR_FADE_LEVELS - 1.0f);

    GLState.color4f[0] = CPAL2Tr(COLOR);
    GLState.color4f[1] = CPAL2Tg(COLOR);
    GLState.color4f[2] = CPAL2Tb(COLOR);
    GLState.color4f[3] = color_a;

    v[0][0] = v[1][0] = xo;
    v[0][1] = v[3][1] = yo;
    v[1][1] = v[2][1] = yf;
    v[2][0] = v[3][0] = xf;
    
    GLState.mode = GL_TRIANGLE_FAN;
    GLState.view = POSITION_XY;
    GLState.count = 4;
    GLState.stride = 0;
    GLState.v_pos = &v[0][0];

    ogl_draw_arrays( GLState );
}

void ogl_ulinec(int left,int top,int right,int bot,int c)
{
    vec2f_t v[2];

    /* Setup vertex array */
    v[0][0] = (left + grd_curcanv->cv_bitmap.bm_x + 0.5) / (float)last_width;
    v[0][1] = 1.0 - (top + grd_curcanv->cv_bitmap.bm_y + 0.5) / (float)last_height;
    v[1][0] = (right + grd_curcanv->cv_bitmap.bm_x + 1.0) / (float)last_width;
    v[1][1] = 1.0 - (bot + grd_curcanv->cv_bitmap.bm_y + 1.0) / (float)last_height;

    GLState.color4f[0] = CPAL2Tr(c);
    GLState.color4f[1] = CPAL2Tg(c);
    GLState.color4f[2] = CPAL2Tb(c);
    GLState.color4f[3] = (grd_curcanv->cv_fade_level >= GR_FADE_OFF)?1.0:1.0 - (float)grd_curcanv->cv_fade_level / ((float)GR_FADE_LEVELS - 1.0);

    GLState.mode = GL_LINES;
    GLState.view = POSITION_XY;
    GLState.count = 2;
    GLState.stride = 0;
    GLState.v_pos = &v[0][0];
    
    ogl_draw_arrays( GLState );
}

GLfloat last_r=0, last_g=0, last_b=0;
int do_pal_step=0;

void ogl_do_palfx(void)
{
    vec2f_t v[4];

    /* Setup vertex array */
    GLState.color4f[0] = last_r;
    GLState.color4f[1] = last_g;
    GLState.color4f[2] = last_b;
    GLState.color4f[3] = 1.0;

    v[0][0] = v[1][0] = 0;
    v[0][1] = v[3][1] = 0;
    v[1][1] = v[2][1] = 1;
    v[2][0] = v[3][0] = 1;

    if (do_pal_step)
    {
	    glEnable(GL_BLEND);
	    glBlendFunc(GL_ONE,GL_ONE);
    }
    else
	    return;
	
    GLState.mode = GL_TRIANGLE_FAN;
    GLState.view = POSITION_XY;
    GLState.count = 4;
    GLState.stride = 0;
    GLState.v_pos = &v[0][0];
	
    ogl_draw_arrays( GLState );
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

int ogl_brightness_ok = 0;
int ogl_brightness_r = 0, ogl_brightness_g = 0, ogl_brightness_b = 0;
static int old_b_r = 0, old_b_g = 0, old_b_b = 0;

void gr_palette_step_up(int r, int g, int b)
{
	old_b_r = ogl_brightness_r;
	old_b_g = ogl_brightness_g;
	old_b_b = ogl_brightness_b;

	ogl_brightness_r = max(r + gr_palette_gamma, 0);
	ogl_brightness_g = max(g + gr_palette_gamma, 0);
	ogl_brightness_b = max(b + gr_palette_gamma, 0);

	if (!ogl_brightness_ok)
	{
		last_r = ogl_brightness_r / 63.0;
		last_g = ogl_brightness_g / 63.0;
		last_b = ogl_brightness_b / 63.0;

		do_pal_step = (r || g || b || gr_palette_gamma);
	}
	else
	{
		do_pal_step = 0;
	}
}

#undef min
using std::min;

void gr_palette_load( ubyte *pal )
{
	int i;

	for (i=0; i<768; i++ )
	{
		gr_current_pal[i] = pal[i];
		if (gr_current_pal[i] > 63)
			gr_current_pal[i] = 63;
	}

	gr_palette_step_up(0, 0, 0); // make ogl_setbrightness_internal get run so that menus get brightened too.
	init_computed_colors();
}

void gr_palette_read(ubyte * pal)
{
	int i;
	for (i=0; i<768; i++ )
	{
		pal[i]=gr_current_pal[i];
		if (pal[i] > 63)
			pal[i] = 63;
	}
}

typedef struct
{
      unsigned char TGAheader[12];
      unsigned char header[6];
} TGA_header;

//writes out an uncompressed RGB .tga file
//if we got really spiffy, we could optionally link in libpng or something, and use that.
static void write_bmp(char *savename,unsigned w,unsigned h,unsigned char *buf)
{
	PHYSFS_file* TGAFile;
	TGA_header TGA;
	GLbyte HeightH,HeightL,WidthH,WidthL;
	unsigned int pixel;
	unsigned char *rgbaBuf;

	buf = (unsigned char*)d_calloc(w*h*4,sizeof(unsigned char));

	rgbaBuf = (unsigned char*) d_calloc(w * h * 4, sizeof(unsigned char));
	glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, rgbaBuf);
	for(pixel = 0; pixel < w * h; pixel++) {
		*(buf + pixel * 3) = *(rgbaBuf + pixel * 4 + 2);
		*(buf + pixel * 3 + 1) = *(rgbaBuf + pixel * 4 + 1);
		*(buf + pixel * 3 + 2) = *(rgbaBuf + pixel * 4);
	}
	d_free(rgbaBuf);

	if (!(TGAFile = PHYSFSX_openWriteBuffered(savename)))
	{
		con_printf(CON_URGENT,"Could not create TGA file to dump screenshot!");
		d_free(buf);
		return;
	}

	HeightH = (GLbyte)(h / 256);
	HeightL = (GLbyte)(h % 256);
	WidthH  = (GLbyte)(w / 256);
	WidthL  = (GLbyte)(w % 256);
	// Write TGA Header
	TGA.TGAheader[0] = 0;
	TGA.TGAheader[1] = 0;
	TGA.TGAheader[2] = 2;
	TGA.TGAheader[3] = 0;
	TGA.TGAheader[4] = 0;
	TGA.TGAheader[5] = 0;
	TGA.TGAheader[6] = 0;
	TGA.TGAheader[7] = 0;
	TGA.TGAheader[8] = 0;
	TGA.TGAheader[9] = 0;
	TGA.TGAheader[10] = 0;
	TGA.TGAheader[11] = 0;
	TGA.header[0] = (GLbyte) WidthL;
	TGA.header[1] = (GLbyte) WidthH;
	TGA.header[2] = (GLbyte) HeightL;
	TGA.header[3] = (GLbyte) HeightH;
	TGA.header[4] = (GLbyte) 24;
	TGA.header[5] = 0;
	PHYSFS_write(TGAFile,&TGA,sizeof(TGA_header),1);
	PHYSFS_write(TGAFile,buf,w*h*3*sizeof(unsigned char),1);
	PHYSFS_close(TGAFile);
	d_free(buf);
}

void save_screen_shot(int automap_flag)
{
	static int savenum=0;
	char savename[13+sizeof(SCRNS_DIR)];
	unsigned char *buf;

	if (!GameArg.DbgGlReadPixelsOk){
		if (!automap_flag)
			HUD_init_message_literal(HM_DEFAULT, "glReadPixels not supported on your configuration");
		return;
	}

	stop_time();

	if (!PHYSFSX_exists(SCRNS_DIR,0))
		PHYSFS_mkdir(SCRNS_DIR); //try making directory

	do
	{
		sprintf(savename, "%sscrn%04d.tga",SCRNS_DIR, savenum++);
	} while (PHYSFSX_exists(savename,0));

	if (!automap_flag)
		HUD_init_message(HM_DEFAULT, "%s 'scrn%04d.tga'", TXT_DUMPING_SCREEN, savenum-1 );

	MALLOC(buf, unsigned char, grd_curscreen->sc_w*grd_curscreen->sc_h*3);
	write_bmp(savename,grd_curscreen->sc_w,grd_curscreen->sc_h,buf);
	d_free(buf);

	start_time();
}

/*
 *
 * Graphics support functions for OpenGL.
 *
 */

//#include <stdio.h>
#ifdef _WIN32
#include <windows.h>
#include <stddef.h>
#endif

#include <string.h>
#include <math.h>
#include <stdio.h>

#include "ogl_init.h"
#include "3d.h"
#include "piggy.h"
#include "common/3d/globvars.h"
#include "dxxerror.h"
#include "texmap.h"
#include "palette.h"
#include "rle.h"
#include "console.h"
#include "u_mem.h"
#ifdef HAVE_LIBPNG
#include "pngfile.h"
#endif

#include "segment.h"
#include "textures.h"
#include "texmerge.h"
#include "effects.h"
#include "weapon.h"
#include "powerup.h"
#include "laser.h"
#include "player.h"
#include "polyobj.h"
#include "gamefont.h"
#include "byteswap.h"
#include "internal.h"
#include "gauges.h"
#include "playsave.h"
#include "args.h"
#if defined(OGL1) || defined(OGL2) 
#include "ogl.h"
#endif

#include <algorithm>
using std::max;

//change to 1 for lots of spew.
#if 0
#define glmprintf(0,a) con_printf(CON_DEBUG, a)
#else
#define glmprintf(a)
#endif

#ifndef M_PI
#define M_PI 3.14159
#endif

#if defined(_WIN32) || (defined(__APPLE__) && defined(__MACH__)) || defined(__sun__) || defined(macintosh)
#define cosf(a) cos(a)
#define sinf(a) sin(a)
#endif

#define f2glf(x) (f2fl(x))

static GLubyte *pixels = NULL;
static GLubyte *texbuf = NULL;

static unsigned char *ogl_pal=gr_palette;

unsigned last_width=~0u,last_height=~0u;
GLfloat ogl_maxanisotropy = 0;

static int r_texcount = 0, r_cachedtexcount = 0;
#ifdef OGLES
static int ogl_rgba_internalformat = GL_RGBA;
static int ogl_rgb_internalformat = GL_RGB;
#else
static int ogl_rgba_internalformat = GL_RGBA8;
static int ogl_rgb_internalformat = GL_RGB8;
#endif
static GLfloat *sphere_va = NULL, *circle_va = NULL, *disk_va = NULL;
static GLfloat *secondary_lva[3]={NULL, NULL, NULL};
static int r_polyc,r_tpolyc,r_bitmapc,r_ubitbltc;
int r_upixelc;

static ogl_texture ogl_texture_list[OGL_TEXTURE_LIST_SIZE];
static int ogl_texture_list_cur;

/* some function prototypes */

static int ogl_loadtexture(unsigned char *data, int dxo, int dyo, ogl_texture *tex, int bm_flags, int data_format, int texfilt);
static void ogl_freetexture(ogl_texture *gltexture);

void ogl_texture_stats(void);
void ogl_cache_vclip_textures(vclip*);
void ogl_cache_vclipn_textures(int);
void ogl_cache_weapon_textures(int);

int8_t check_opengl_errors( const string& file, uint16_t line );

static void ogl_loadbmtexture(grs_bitmap *bm)
{
	ogl_loadbmtexture_f(bm, GameCfg.TexFilt);
}

static void ogl_init_texture_stats(ogl_texture* t){
	t->prio=0.3;//default prio
	t->numrend=0;
}

void ogl_init_texture(ogl_texture* t, int w, int h, int flags)
{
	t->handle = 0;
#ifndef OGLES
	if (flags & OGL_FLAG_NOCOLOR)
	{
		// use GL_INTENSITY instead of GL_RGB
		if (flags & OGL_FLAG_ALPHA)
		{
			if (GameArg.DbgGlIntensity4Ok)
			{
				t->internalformat = GL_INTENSITY4;
				t->format = GL_LUMINANCE;
			}
			else if (GameArg.DbgGlLuminance4Alpha4Ok)
			{
				t->internalformat = GL_LUMINANCE4_ALPHA4;
				t->format = GL_LUMINANCE_ALPHA;
			}
			else if (GameArg.DbgGlRGBA2Ok)
			{
				t->internalformat = GL_RGBA2;
				t->format = GL_RGBA;
			}
			else
			{
				t->internalformat = ogl_rgba_internalformat;
				t->format = GL_RGBA;
			}
		}
		else
		{
			// there are certainly smaller formats we could use here, but nothing needs it ATM.
			t->internalformat = ogl_rgb_internalformat;
			t->format = GL_RGB;
		}
	}
	else
	{
#endif
		if (flags & OGL_FLAG_ALPHA)
		{
			t->internalformat = ogl_rgba_internalformat;
			t->format = GL_RGBA;
		}
		else
		{
			t->internalformat = ogl_rgb_internalformat;
			t->format = GL_RGB;
		}
#ifndef OGLES
	}
#endif
	t->wrapstate = -1;
	t->lw = t->w = w;
	t->h = h;
	ogl_init_texture_stats(t);
}

static void ogl_reset_texture(ogl_texture* t)
{
	ogl_init_texture(t, 0, 0, 0);
}

static void ogl_reset_texture_stats_internal(void){
	int i;
	for (i=0;i<OGL_TEXTURE_LIST_SIZE;i++)
		if (ogl_texture_list[i].handle>0){
			ogl_init_texture_stats(&ogl_texture_list[i]);
		}
}

void ogl_init_texture_list_internal(void){
	int i;
	ogl_texture_list_cur=0;
	for (i=0;i<OGL_TEXTURE_LIST_SIZE;i++)
		ogl_reset_texture(&ogl_texture_list[i]);
}

void ogl_smash_texture_list_internal(void){
	int i;
	if (sphere_va != NULL)
	{
		d_free(sphere_va);
		sphere_va = NULL;
	}
	if (circle_va != NULL)
	{
		d_free(circle_va);
		circle_va = NULL;
	}
	if (disk_va != NULL)
	{
		d_free(disk_va);
		disk_va = NULL;
	}
	for(i = 0; i < 3; i++) {
		if (secondary_lva[i] != NULL)
		{
			d_free(secondary_lva[i]);
			secondary_lva[i] = NULL;
		}
	}
	for (i=0;i<OGL_TEXTURE_LIST_SIZE;i++){
		if (ogl_texture_list[i].handle>0){
			glDeleteTextures( 1, &ogl_texture_list[i].handle );
			ogl_texture_list[i].handle=0;
		}
		ogl_texture_list[i].wrapstate = -1;
	}
}

ogl_texture* ogl_get_free_texture(void){
	int i;
	for (i=0;i<OGL_TEXTURE_LIST_SIZE;i++){
		if (ogl_texture_list[ogl_texture_list_cur].handle<=0 && ogl_texture_list[ogl_texture_list_cur].w==0)
			return &ogl_texture_list[ogl_texture_list_cur];
		if (++ogl_texture_list_cur>=OGL_TEXTURE_LIST_SIZE)
			ogl_texture_list_cur=0;
	}
	Error("OGL: texture list full!\n");
}

void ogl_texture_stats(void)
{
	int used = 0, usedother = 0, usedidx = 0, usedrgb = 0, usedrgba = 0;
	int databytes = 0, truebytes = 0, datatexel = 0, truetexel = 0, i;
	int prio0=0,prio1=0,prio2=0,prio3=0,prioh=0;
	GLint idx, r, g, b, a, dbl, depth;
	int res, colorsize, depthsize;
	ogl_texture* t;

	for (i=0;i<OGL_TEXTURE_LIST_SIZE;i++){
		t=&ogl_texture_list[i];
		if (t->handle>0){
			used++;
			datatexel+=t->w*t->h;
			truetexel+=t->tw*t->th;
			databytes+=t->bytesu;
			truebytes+=t->bytes;
			if (t->prio<0.299)prio0++;
			else if (t->prio<0.399)prio1++;
			else if (t->prio<0.499)prio2++;
			else if (t->prio<0.599)prio3++;
			else prioh++;
			if (t->format == GL_RGBA)
				usedrgba++;
			else if (t->format == GL_RGB)
				usedrgb++;
#ifndef OGLES
			else if (t->format == GL_COLOR_INDEX)
				usedidx++;
#endif
			else
				usedother++;
		}
	}

	res = SWIDTH * SHEIGHT;
#ifndef OGLES
	glGetIntegerv(GL_INDEX_BITS, &idx);
#endif
	glGetIntegerv(GL_RED_BITS, &r);
	glGetIntegerv(GL_GREEN_BITS, &g);
	glGetIntegerv(GL_BLUE_BITS, &b);
	glGetIntegerv(GL_ALPHA_BITS, &a);
#ifndef OGLES
	glGetIntegerv(GL_DOUBLEBUFFER, &dbl);
#endif
	dbl += 1;
	glGetIntegerv(GL_DEPTH_BITS, &depth);
	gr_set_current_canvas(NULL);
	gr_set_curfont( GAME_FONT );
	gr_set_fontcolor( BM_XRGB(255,255,255),-1 );
	colorsize = (idx * res * dbl) / 8;
	depthsize = res * depth / 8;
	gr_printf(FSPACX(2),FSPACY(1),"%i flat %i tex %i bitmaps",r_polyc,r_tpolyc,r_bitmapc);
	gr_printf(FSPACX(2), FSPACY(1)+LINE_SPACING, "%i(%i,%i,%i,%i) %iK(%iK wasted) (%i postcachedtex)", used, usedrgba, usedrgb, usedidx, usedother, truebytes / 1024, (truebytes - databytes) / 1024, r_texcount - r_cachedtexcount);
	gr_printf(FSPACX(2), FSPACY(1)+(LINE_SPACING*2), "%ibpp(r%i,g%i,b%i,a%i)x%i=%iK depth%i=%iK", idx, r, g, b, a, dbl, colorsize / 1024, depth, depthsize / 1024);
	gr_printf(FSPACX(2), FSPACY(1)+(LINE_SPACING*3), "total=%iK", (colorsize + depthsize + truebytes) / 1024);
}

static void ogl_checkbmtex(grs_bitmap *bm){
	if (bm->gltexture==NULL || bm->gltexture->handle<=0)
		ogl_loadbmtexture(bm);
	bm->gltexture->numrend++;
}

//crude texture precaching
//handles: powerups, walls, weapons, polymodels, etc.
//it is done with the horrid do_special_effects kludge so that sides that have to be texmerged and have animated textures will be correctly cached.
//similarly, with the objects(esp weapons), we could just go through and cache em all instead, but that would get ones that might not even be on the level
//TODO: doors

void ogl_cache_polymodel_textures(int model_num)
{
	polymodel *po;
	int i;

	if (model_num < 0)
		return;
	po = &Polygon_models[model_num];
	for (i=0;i<po->n_textures;i++)  {
		ogl_loadbmtexture(&GameBitmaps[ObjBitmaps[ObjBitmapPtrs[po->first_texture+i]].index]);
	}
}

void ogl_cache_vclip_textures(vclip *vc){
	int i;
	for (i=0;i<vc->num_frames;i++){
		PIGGY_PAGE_IN(vc->frames[i]);
		ogl_loadbmtexture(&GameBitmaps[vc->frames[i].index]);
	}
}

void ogl_cache_vclipn_textures(int i)
{
	if (i >= 0 && i < VCLIP_MAXNUM)
		ogl_cache_vclip_textures(&Vclip[i]);
}

void ogl_cache_weapon_textures(int weapon_type)
{
	weapon_info *w;

	if (weapon_type < 0)
		return;
	w = &Weapon_info[weapon_type];
	ogl_cache_vclipn_textures(w->flash_vclip);
	ogl_cache_vclipn_textures(w->robot_hit_vclip);
	ogl_cache_vclipn_textures(w->wall_hit_vclip);
	if (w->render_type==WEAPON_RENDER_VCLIP)
		ogl_cache_vclipn_textures(w->weapon_vclip);
	else if (w->render_type == WEAPON_RENDER_POLYMODEL)
	{
		ogl_cache_polymodel_textures(w->model_num);
		ogl_cache_polymodel_textures(w->model_num_inner);
	}
}

void ogl_cache_level_textures(void)
{
	int seg,side,i;
	eclip *ec;
	short tmap1,tmap2;
	grs_bitmap *bm,*bm2;
	struct side *sidep;
	int max_efx=0,ef;

	ogl_reset_texture_stats_internal();//loading a new lev should reset textures

	for (i=0,ec=Effects;i<Num_effects;i++,ec++) {
		ogl_cache_vclipn_textures(Effects[i].dest_vclip);
		if ((Effects[i].changing_wall_texture == -1) && (Effects[i].changing_object_texture==-1) )
			continue;
		if (ec->vc.num_frames>max_efx)
			max_efx=ec->vc.num_frames;
	}
	glmprintf((0,"max_efx:%i\n",max_efx));
	for (ef=0;ef<max_efx;ef++){
		for (i=0,ec=Effects;i<Num_effects;i++,ec++) {
			if ((Effects[i].changing_wall_texture == -1) && (Effects[i].changing_object_texture==-1) )
				continue;
			ec->time_left=-1;
		}
		do_special_effects();

		for (seg=0;seg<Num_segments;seg++){
			for (side=0;side<MAX_SIDES_PER_SEGMENT;side++){
				sidep=&Segments[seg].sides[side];
				tmap1=sidep->tmap_num;
				tmap2=sidep->tmap_num2;
				if (tmap1<0 || tmap1>=NumTextures){
					glmprintf((0,"ogl_cache_level_textures %i %i %i %i\n",seg,side,tmap1,NumTextures));
					//				tmap1=0;
					continue;
				}
				PIGGY_PAGE_IN(Textures[tmap1]);
				bm = &GameBitmaps[Textures[tmap1].index];
				if (tmap2 != 0){
					PIGGY_PAGE_IN(Textures[tmap2&0x3FFF]);
					bm2 = &GameBitmaps[Textures[tmap2&0x3FFF].index];
					if (GameArg.DbgAltTexMerge == 0 || (bm2->bm_flags & BM_FLAG_SUPER_TRANSPARENT))
						bm = texmerge_get_cached_bitmap( tmap1, tmap2 );
					else {
						ogl_loadbmtexture(bm2);
					}
				}
				ogl_loadbmtexture(bm);
			}
		}
		glmprintf((0,"finished ef:%i\n",ef));
	}
	reset_special_effects();
	init_special_effects();
	{
		// always have lasers, concs, flares.  Always shows player appearance, and at least concs are always available to disappear.
		ogl_cache_weapon_textures(Primary_weapon_to_weapon_info[LASER_INDEX]);
		ogl_cache_weapon_textures(Secondary_weapon_to_weapon_info[CONCUSSION_INDEX]);
		ogl_cache_weapon_textures(FLARE_ID);
		ogl_cache_vclipn_textures(VCLIP_PLAYER_APPEARANCE);
		ogl_cache_vclipn_textures(VCLIP_POWERUP_DISAPPEARANCE);
		ogl_cache_polymodel_textures(Player_ship->model_num);
		ogl_cache_vclipn_textures(Player_ship->expl_vclip_num);

		for (i=0;i<=Highest_object_index;i++){
			if(Objects[i].render_type==RT_POWERUP){
				ogl_cache_vclipn_textures(Objects[i].rtype.vclip_info.vclip_num);
				switch (Objects[i].id){
					case POW_VULCAN_WEAPON:
						ogl_cache_weapon_textures(Primary_weapon_to_weapon_info[VULCAN_INDEX]);
						break;
					case POW_SPREADFIRE_WEAPON:
						ogl_cache_weapon_textures(Primary_weapon_to_weapon_info[SPREADFIRE_INDEX]);
						break;
					case POW_PLASMA_WEAPON:
						ogl_cache_weapon_textures(Primary_weapon_to_weapon_info[PLASMA_INDEX]);
						break;
					case POW_FUSION_WEAPON:
						ogl_cache_weapon_textures(Primary_weapon_to_weapon_info[FUSION_INDEX]);
						break;
					case POW_PROXIMITY_WEAPON:
						ogl_cache_weapon_textures(Secondary_weapon_to_weapon_info[PROXIMITY_INDEX]);
						break;
					case POW_HOMING_AMMO_1:
					case POW_HOMING_AMMO_4:
						ogl_cache_weapon_textures(Primary_weapon_to_weapon_info[HOMING_INDEX]);
						break;
					case POW_SMARTBOMB_WEAPON:
						ogl_cache_weapon_textures(Secondary_weapon_to_weapon_info[SMART_INDEX]);
						break;
					case POW_MEGA_WEAPON:
						ogl_cache_weapon_textures(Secondary_weapon_to_weapon_info[MEGA_INDEX]);
						break;
				}
			}
			else if(Objects[i].render_type==RT_POLYOBJ){
				if (Objects[i].type == OBJ_ROBOT)
				{
					ogl_cache_vclipn_textures(Robot_info[Objects[i].id].exp1_vclip_num);
					ogl_cache_vclipn_textures(Robot_info[Objects[i].id].exp2_vclip_num);
					ogl_cache_weapon_textures(Robot_info[Objects[i].id].weapon_type);
				}
				if (Objects[i].rtype.pobj_info.tmap_override != -1)
					ogl_loadbmtexture(&GameBitmaps[Textures[Objects[i].rtype.pobj_info.tmap_override].index]);
				else
					ogl_cache_polymodel_textures(Objects[i].rtype.pobj_info.model_num);
			}
		}
	}
	glmprintf((0,"finished caching\n"));
	r_cachedtexcount = r_texcount;
}

bool g3_draw_line(g3s_point *p0,g3s_point *p1)
{
	vec3f_t v[2];

	v[0][0] =  f2glf(p0->p3_vec.x);
	v[0][1] =  f2glf(p0->p3_vec.y);
	v[0][2] = -f2glf(p0->p3_vec.z);
	v[1][0] =  f2glf(p1->p3_vec.x);
	v[1][1] =  f2glf(p1->p3_vec.y);
	v[1][2] = -f2glf(p1->p3_vec.z);

	GLState.color4f[0] = PAL2Tr(grd_curcanv->cv_color);
	GLState.color4f[1] = PAL2Tg(grd_curcanv->cv_color);
	GLState.color4f[2] = PAL2Tb(grd_curcanv->cv_color);
	GLState.color4f[3] = 1.0;
	
	GLState.mode = GL_LINES;
	GLState.view = POSITION_XYZ;
	GLState.count = 2;
	GLState.stride = 0;
	GLState.v_pos = &v[0][0];

	ogl_draw_arrays( GLState );

	return 1;
}

static GLfloat *circle_array_init(int nsides)
{
	int i;
	float ang;
	GLfloat *vertex_array = (GLfloat *) d_malloc(sizeof(GLfloat) * nsides * 2);

	for(i = 0; i < nsides; i++) {
		ang = 2.0 * M_PI * i / nsides;
		vertex_array[i * 2] = cosf(ang);
        	vertex_array[i * 2 + 1] = sinf(ang);
	}

	return vertex_array;
}

static GLfloat *circle_array_init_2(int nsides, float xsc, float xo, float ysc, float yo)
{
 	int i;
 	float ang;
	GLfloat *vertex_array = (GLfloat *) d_malloc(sizeof(GLfloat) * nsides * 2);

	for(i = 0; i < nsides; i++) {
		ang = 2.0 * M_PI * i / nsides;
		vertex_array[i * 2] = cosf(ang) * xsc + xo;
		vertex_array[i * 2 + 1] = sinf(ang) * ysc + yo;
	}

	return vertex_array;
}

void ogl_draw_vertex_reticle(int cross,int primary,int secondary,int color,int alpha,int size_offs)
{
	GLfloat *vtx_pos; 
	GLubyte *vtx_clr;

	int size=270+(size_offs*20), i;
	float scale = ((float)SWIDTH/SHEIGHT);
	GLubyte ret_rgba[4], ret_dark_rgba[4];
	GLfloat cross_lva[8 * 2] = {
		-4.0, 2.0, -2.0, 0, -3.0, -4.0, -2.0, -3.0, 4.0, 2.0, 2.0, 0, 3.0, -4.0, 2.0, -3.0,
	};
	GLfloat primary_lva[4][4 * 2] = {
		{ -5.5, -5.0, -6.5, -7.5, -10.0, -7.0, -10.0, -8.7 },
		{ -10.0, -7.0, -10.0, -8.7, -15.0, -8.5, -15.0, -9.5 },
		{ 5.5, -5.0, 6.5, -7.5, 10.0, -7.0, 10.0, -8.7 },
		{ 10.0, -7.0, 10.0, -8.7, 15.0, -8.5, 15.0, -9.5 }
	};
	GLubyte dark_lca[16 * 4] = {
		32, 138, 32, 153, 32, 138, 32, 153, 32, 138, 32, 153, 32, 138, 32, 153,
		32, 138, 32, 153, 32, 138, 32, 153, 32, 138, 32, 153, 32, 138, 32, 153,
		32, 138, 32, 153, 32, 138, 32, 153, 32, 138, 32, 153, 32, 138, 32, 153,
		32, 138, 32, 153, 32, 138, 32, 153, 32, 138, 32, 153, 32, 138, 32, 153
	};
	GLubyte bright_lca[16 * 4] = {
		32, 255, 32, 255, 32, 255, 32, 255, 32, 255, 32, 255, 32, 255, 32, 255,
		32, 255, 32, 255, 32, 255, 32, 255, 32, 255, 32, 255, 32, 255, 32, 255,
		32, 255, 32, 255, 32, 255, 32, 255, 32, 255, 32, 255, 32, 255, 32, 255,
		32, 255, 32, 255, 32, 255, 32, 255, 32, 255, 32, 255, 32, 255, 32, 255
	};
	GLubyte cross_lca[8 * 4] = {
		32, 138, 32, 153, 32, 255, 32, 255,
		32, 138, 32, 153, 32, 255, 32, 255,
		32, 138, 32, 153, 32, 255, 32, 255,
		32, 138, 32, 153, 32, 255, 32, 255
	};
	GLubyte primary_lca[2][4 * 4] = {
		{32, 255, 32, 255, 32, 255, 32, 255, 32, 138, 32, 153, 32, 138, 32, 153},
		{32, 138, 32, 153, 32, 138, 32, 153, 32, 255, 32, 255, 32, 255, 32, 255}
	};
	
	ret_rgba[0] = 255.0f*(PAL2Tr(color));
	ret_dark_rgba[0] = ret_rgba[0]/2;
	ret_rgba[1] = 255.0f*(PAL2Tg(color));
	ret_dark_rgba[1] = ret_rgba[1]/2;
	ret_rgba[2] = 255.0f*(PAL2Tb(color));
	ret_dark_rgba[2] = ret_rgba[2]/2;
	ret_rgba[3] = 255.0f*(1.0 - ((float)alpha / ((float)GR_FADE_LEVELS)));
	ret_dark_rgba[3] = ret_rgba[3]/2;

	for (i = 0; i < 16*4; i += 4)
	{
		bright_lca[i] = ret_rgba[0];
		dark_lca[i] = ret_dark_rgba[0];
		bright_lca[i+1] = ret_rgba[1];
		dark_lca[i+1] = ret_dark_rgba[1];
		bright_lca[i+2] = ret_rgba[2];
		dark_lca[i+2] = ret_dark_rgba[2];
		bright_lca[i+3] = ret_rgba[3];
		dark_lca[i+3] = ret_dark_rgba[3];
	}
	for (i = 0; i < 8*4; i += 8)
	{
		cross_lca[i] = ret_dark_rgba[0];
		cross_lca[i+1] = ret_dark_rgba[1];
		cross_lca[i+2] = ret_dark_rgba[2];
		cross_lca[i+3] = ret_dark_rgba[3];
		cross_lca[i+4] = ret_rgba[0];
		cross_lca[i+5] = ret_rgba[1];
		cross_lca[i+6] = ret_rgba[2];
		cross_lca[i+7] = ret_rgba[3];
	}

	primary_lca[0][0] = primary_lca[0][4] = primary_lca[1][8] = primary_lca[1][12] = ret_rgba[0];
	primary_lca[0][1] = primary_lca[0][5] = primary_lca[1][9] = primary_lca[1][13] = ret_rgba[1];
	primary_lca[0][2] = primary_lca[0][6] = primary_lca[1][10] = primary_lca[1][14] = ret_rgba[2];
	primary_lca[0][3] = primary_lca[0][7] = primary_lca[1][11] = primary_lca[1][15] = ret_rgba[3];
	primary_lca[1][0] = primary_lca[1][4] = primary_lca[0][8] = primary_lca[0][12] = ret_dark_rgba[0];
	primary_lca[1][1] = primary_lca[1][5] = primary_lca[0][9] = primary_lca[0][13] = ret_dark_rgba[1];
	primary_lca[1][2] = primary_lca[1][6] = primary_lca[0][10] = primary_lca[0][14] = ret_dark_rgba[2];
	primary_lca[1][3] = primary_lca[1][7] = primary_lca[0][11] = primary_lca[0][15] = ret_dark_rgba[3];

	ModelView.push_back( ModelView.back() );
	ogl_translate((grd_curcanv->cv_bitmap.bm_w/2+grd_curcanv->cv_bitmap.bm_x)/(float)last_width,1.0-(grd_curcanv->cv_bitmap.bm_h/2+grd_curcanv->cv_bitmap.bm_y)/(float)last_height,0);

	if (scale >= 1)
	{
		size/=scale;
		ogl_scale(f2glf(size),f2glf(size*scale),f2glf(size));
	}
	else
	{
		size*=scale;
		ogl_scale(f2glf(size/scale),f2glf(size),f2glf(size));
	}

	glLineWidth(linedotscale*2);
	//OGL_DISABLE(TEXTURE_2D);
	glDisable(GL_CULL_FACE);

	//cross
	if(cross)
		vtx_clr = &cross_lca[0];
	else
		vtx_clr = &dark_lca[0];
	vtx_pos = &cross_lva[0];
    
	GLState.mode = GL_LINES;
	GLState.view = POSITION_XY;
	GLState.count = 8;
	GLState.stride = 0;
	GLState.v_pos = vtx_pos;
	GLState.v_clr = vtx_clr;
	ogl_draw_arrays( GLState );

	//left primary bar
	if(primary == 0)
		vtx_clr = &dark_lca[0];
	else
		vtx_clr = &primary_lca[0][0];
	vtx_pos = &primary_lva[0][0];
    
	GLState.mode = GL_TRIANGLE_STRIP;
	GLState.view = POSITION_XY;
	GLState.count = 4;
	GLState.stride = 0;
	GLState.v_pos = vtx_pos;
	GLState.v_clr = vtx_clr;
	ogl_draw_arrays( GLState );

	if(primary != 2)
		vtx_clr = &dark_lca[0];
	else
		vtx_clr = &primary_lca[1][0];
	vtx_pos = &primary_lva[1][0];

	GLState.v_pos = vtx_pos;
	GLState.v_clr = vtx_clr;
	ogl_draw_arrays( GLState );
	//right primary bar
	if(primary == 0)
		vtx_clr = &dark_lca[0];
	else
		vtx_clr = &primary_lca[0][0];
	vtx_pos = &primary_lva[2][0];
    
	GLState.v_pos = vtx_pos;
	GLState.v_clr = vtx_clr;	
	ogl_draw_arrays( GLState );
    
	if(primary != 2)
		vtx_clr = &dark_lca[0];
	else
		vtx_clr = &primary_lca[1][0];
	vtx_pos = &primary_lva[3][0];
    
	GLState.v_pos = vtx_pos;
	GLState.v_clr = vtx_clr;	
	ogl_draw_arrays( GLState );

	if (secondary<=2){
		//left secondary
		if (secondary != 1)
			vtx_clr = &dark_lca[0];
		else
			vtx_clr = &bright_lca[0];
		if(!secondary_lva[0])
			secondary_lva[0] = circle_array_init_2(16, 2.0, -10.0, 2.0, -2.0);
		vtx_pos = &secondary_lva[0][0];
		//right secondary
		if (secondary != 2)
			vtx_clr = &dark_lca[0];
		else
			vtx_clr = &bright_lca[0];
		if(!secondary_lva[1])
			secondary_lva[1] = circle_array_init_2(16, 2.0, 10.0, 2.0, -2.0);
		vtx_pos = &secondary_lva[1][0];
	}
	else {
		//bottom/middle secondary
		if (secondary != 4)
			vtx_clr = &dark_lca[0];
		else
			vtx_clr = &bright_lca[0];
		if(!secondary_lva[2])
			secondary_lva[2] = circle_array_init_2(16, 2.0, 0.0, 2.0, -8.0);
		vtx_pos = &secondary_lva[2][0];
	}

	GLState.mode = GL_LINE_LOOP;
	GLState.view = POSITION_XY;
	GLState.count = 16;
	GLState.stride = 0;
	GLState.v_pos = vtx_pos;
	GLState.v_clr = vtx_clr;
	
	ogl_draw_arrays( GLState );

	ModelView.pop_back();
	glLineWidth(linedotscale);
}

/*
 * Stars on heaven in exit sequence, automap objects
 */
int g3_draw_sphere(g3s_point *pnt,fix rad)
{ 
	float scale = ((float)grd_curcanv->cv_bitmap.bm_w/grd_curcanv->cv_bitmap.bm_h);

	glDisable(GL_CULL_FACE);
	ModelView.push_back( ModelView.back() );
	ogl_translate(f2glf(pnt->p3_vec.x),f2glf(pnt->p3_vec.y),-f2glf(pnt->p3_vec.z));
	if (scale >= 1)
	{
		rad/=scale;
		ogl_scale(f2glf(rad),f2glf(rad*scale),f2glf(rad));
	}
	else
	{
		rad*=scale;
		ogl_scale(f2glf(rad/scale),f2glf(rad),f2glf(rad));
	}
	if(!sphere_va)
		sphere_va = circle_array_init(20);

	GLState.color4f[0] = PAL2Tr(grd_curcanv->cv_color);
	GLState.color4f[1] = PAL2Tg(grd_curcanv->cv_color);
	GLState.color4f[2] = PAL2Tb(grd_curcanv->cv_color);
	GLState.color4f[3] = 1.0;
	
	GLState.mode = GL_TRIANGLE_FAN;
	GLState.view = POSITION_XY;
	GLState.count = 20;
	GLState.stride = 0;
	GLState.v_pos = sphere_va;

	ogl_draw_arrays( GLState );

	ModelView.pop_back();
	return 0;
}

int gr_ucircle(fix xc1, fix yc1, fix r1)
{
	int nsides;

	ModelView.push_back(  ModelView.back() );
	ogl_translate(
	              (f2fl(xc1) + grd_curcanv->cv_bitmap.bm_x + 0.5) / (float)last_width,
	              1.0 - (f2fl(yc1) + grd_curcanv->cv_bitmap.bm_y + 0.5) / (float)last_height,0);
	ogl_scale(f2fl(r1) / last_width, f2fl(r1) / last_height, 1.0);

	nsides = 10 + 2 * (int)(M_PI * f2fl(r1) / 19);
	if(!circle_va)
		circle_va = circle_array_init(nsides);

	GLState.color4f[0] = PAL2Tr(grd_curcanv->cv_color);
	GLState.color4f[1] = PAL2Tg(grd_curcanv->cv_color);
	GLState.color4f[2] = PAL2Tb(grd_curcanv->cv_color);
	GLState.color4f[3] = (grd_curcanv->cv_fade_level >= GR_FADE_OFF)?1.0:1.0 - (float)grd_curcanv->cv_fade_level / ((float)GR_FADE_LEVELS - 1.0);

	GLState.mode = GL_LINE_LOOP;
	GLState.view = POSITION_XY;
	GLState.count = nsides;
	GLState.stride = 0;
	GLState.v_pos = circle_va;

	ogl_draw_arrays( GLState );

	ModelView.pop_back();
	return 0;
}

int gr_circle(fix xc1,fix yc1,fix r1){
	return gr_ucircle(xc1,yc1,r1);
}

int gr_disk(fix x,fix y,fix r)
{
	int nsides;

	ModelView.push_back(  ModelView.back() );
	ogl_translate( 
                   (f2fl(x) + grd_curcanv->cv_bitmap.bm_x + 0.5) / (float)last_width,
                   1.0 - (f2fl(y) + grd_curcanv->cv_bitmap.bm_y + 0.5) / (float)last_height,0);
	ogl_scale( f2fl(r) / last_width, f2fl(r) / last_height, 1.0 );

	nsides = 10 + 2 * (int)(M_PI * f2fl(r) / 19);
	if(!disk_va)
		disk_va = circle_array_init(nsides);
	
	GLState.color4f[0] = PAL2Tr(grd_curcanv->cv_color);
	GLState.color4f[1] = PAL2Tg(grd_curcanv->cv_color);
	GLState.color4f[2] = PAL2Tb(grd_curcanv->cv_color);
	GLState.color4f[3] = (grd_curcanv->cv_fade_level >= GR_FADE_OFF)?1.0:1.0 - (float)grd_curcanv->cv_fade_level / ((float)GR_FADE_LEVELS - 1.0);

	GLState.mode = GL_TRIANGLE_FAN;
	GLState.view = POSITION_XY;
	GLState.count = nsides;
	GLState.stride = 0;
	GLState.v_pos = disk_va;

	ogl_draw_arrays( GLState );

	ModelView.pop_back();
	return 0;
}

/*
 * Draw flat-shaded Polygon (Lasers, Drone-arms, Driller-ears)
 */
bool g3_draw_poly(int nv,g3s_point **pointlist)
{
	int c;
	vec3f_t v[nv];
	GLfloat color_a;

	r_polyc++;

	if (grd_curcanv->cv_fade_level >= GR_FADE_OFF)
		color_a = 1.0;
	else
		color_a = 1.0 - (float)grd_curcanv->cv_fade_level / ((float)GR_FADE_LEVELS - 1.0);

	for (c=0; c<nv; c++){
		v[c][0] =  f2glf(pointlist[c]->p3_vec.x);
		v[c][1] =  f2glf(pointlist[c]->p3_vec.y);
		v[c][2] = -f2glf(pointlist[c]->p3_vec.z);		
	}

	GLState.color4f[0] = PAL2Tr(grd_curcanv->cv_color);
	GLState.color4f[1] = PAL2Tg(grd_curcanv->cv_color);
	GLState.color4f[2] = PAL2Tb(grd_curcanv->cv_color);
	GLState.color4f[3] = color_a;
	
	GLState.mode = GL_TRIANGLE_FAN;
	GLState.view = POSITION_XYZ;
	GLState.count = nv;
	GLState.stride = 0;
	GLState.v_pos = &v[0][0];

	ogl_draw_arrays( GLState );

	return 0;
}

void gr_upoly_tmap(int, const int *){
		glmprintf((0,"gr_upoly_tmap: unhandled\n"));//should never get called
}

void draw_tmap_flat(grs_bitmap *,int,g3s_point **){
		glmprintf((0,"draw_tmap_flat: unhandled\n"));//should never get called
}

/*
 * Everything texturemapped (walls, robots, ship)
 */
bool g3_draw_tmap(int nv,g3s_point **pointlist,g3s_uvl *uvl_list,g3s_lrgb *light_rgb,grs_bitmap *bm)
{
	int c;
	vertex_tex_t v[nv];
	GLfloat color_alpha = 1.0;

	if (tmap_drawer_ptr == draw_tmap) {
		ogl_checkbmtex(bm);
		r_tpolyc++;
		color_alpha = (grd_curcanv->cv_fade_level >= GR_FADE_OFF)?1.0:(1.0 - (float)grd_curcanv->cv_fade_level / ((float)GR_FADE_LEVELS - 1.0));
	} else if (tmap_drawer_ptr == draw_tmap_flat) {
		/* for cloaked state faces */
		GLState.color4f[0] = 0;
		GLState.color4f[1] = 0;
		GLState.color4f[2] = 0;
		GLState.color4f[3] = 1.0 - (grd_curcanv->cv_fade_level/(GLfloat)NUM_LIGHTING_LEVELS);
		
	} else {
		glmprintf((0,"g3_draw_tmap: unhandled tmap_drawer %p\n",tmap_drawer_ptr));
		return 0;
	}
	
	for (c=0; c<nv; c++)
	{
		v[c].position[0] =  f2glf(pointlist[c]->p3_vec.x);
		v[c].position[1] =  f2glf(pointlist[c]->p3_vec.y);
		v[c].position[2] = -f2glf(pointlist[c]->p3_vec.z);

		if (tmap_drawer_ptr == draw_tmap)
		{
			v[c].color[0] = 255.0f*((bm->bm_flags & BM_FLAG_NO_LIGHTING) ? 1.0 : f2glf(light_rgb[c].r));
			v[c].color[1] = 255.0f*((bm->bm_flags & BM_FLAG_NO_LIGHTING) ? 1.0 : f2glf(light_rgb[c].g));
			v[c].color[2] = 255.0f*((bm->bm_flags & BM_FLAG_NO_LIGHTING) ? 1.0 : f2glf(light_rgb[c].b));
			v[c].color[3] = 255.0f*(color_alpha);
			
			v[c].texcoord[0] = f2glf(uvl_list[c].u);
			v[c].texcoord[1] = f2glf(uvl_list[c].v);
		}
	}

	GLState.mode = GL_TRIANGLE_FAN;
	GLState.view = POSITION_XYZ;
	GLState.count = nv;
	GLState.stride = sizeof(vertex_tex_t);
	GLState.v_pos = &v[0].position[0];
	
	if (tmap_drawer_ptr == draw_tmap)
	{
	    GLState.tex[0] = bm->gltexture->handle;
	    GLState.texwrap[0] = GL_REPEAT;
	    GLState.v_tex[0] = &v[0].texcoord[0];
	    GLState.v_clr = &v[0].color[0];
	}
	
	ogl_draw_arrays( GLState );	
	
	return 0;
}

/*
 * Everything texturemapped with secondary texture (walls with secondary texture)
 */
bool g3_draw_tmap_2(int nv, g3s_point **pointlist, g3s_uvl *uvl_list, g3s_lrgb *light_rgb, grs_bitmap *bmbot, grs_bitmap *bm, int orient)
{
	int c;  
	vertex_mtex_t v[nv];
	
	r_tpolyc++;

	ogl_checkbmtex(bm);
	ogl_checkbmtex(bmbot);

	for (c=0; c<nv; c++) {
		switch(orient){
			case 1:
				v[c].texcoord[1][0] = 1.0-f2glf(uvl_list[c].v);
				v[c].texcoord[1][1] = f2glf(uvl_list[c].u);
				break;
			case 2:
				v[c].texcoord[1][0] = 1.0-f2glf(uvl_list[c].u);
				v[c].texcoord[1][1] = 1.0-f2glf(uvl_list[c].v);
				break;
			case 3:
				v[c].texcoord[1][0] = f2glf(uvl_list[c].v);
				v[c].texcoord[1][1] = 1.0-f2glf(uvl_list[c].u);
				break;
			default:
				v[c].texcoord[1][0] = f2glf(uvl_list[c].u);
				v[c].texcoord[1][1] = f2glf(uvl_list[c].v);
				break;
		}
		
		v[c].texcoord[0][0] = f2glf(uvl_list[c].u);
		v[c].texcoord[0][1] = f2glf(uvl_list[c].v);
		
		v[c].color[0] = 255.0f*(bm->bm_flags & BM_FLAG_NO_LIGHTING ? 1.0 : f2glf(light_rgb[c].r));
		v[c].color[1] = 255.0f*(bm->bm_flags & BM_FLAG_NO_LIGHTING ? 1.0 : f2glf(light_rgb[c].g));
		v[c].color[2] = 255.0f*(bm->bm_flags & BM_FLAG_NO_LIGHTING ? 1.0 : f2glf(light_rgb[c].b));
		v[c].color[3] = 255.0f*((grd_curcanv->cv_fade_level >= GR_FADE_OFF)?1.0:(1.0 - (float)grd_curcanv->cv_fade_level / ((float)GR_FADE_LEVELS - 1.0)));

		v[c].position[0] =  f2glf(pointlist[c]->p3_vec.x);
		v[c].position[1] =  f2glf(pointlist[c]->p3_vec.y);
		v[c].position[2] = -f2glf(pointlist[c]->p3_vec.z);
	}

	GLState.mode = GL_TRIANGLE_FAN;
	GLState.view = POSITION_XYZ;
	GLState.count = nv;
	GLState.stride = sizeof(vertex_mtex_t);
	GLState.v_pos = &v[0].position[0];
	GLState.v_clr = &v[0].color[0];
	
	GLState.tex[0] = bmbot->gltexture->handle;
	GLState.tex[1] = bm->gltexture->handle;
	GLState.texwrap[0] = GLState.texwrap[1] = GL_REPEAT;
	GLState.v_tex[0] = &v[0].texcoord[0][0];
	GLState.v_tex[1] = &v[0].texcoord[1][0];
	
	ogl_draw_arrays( GLState );

	return 0;
}

/*
 * 2d Sprites (Fireaballs, powerups, explosions). NOT hostages
 */
bool g3_draw_bitmap(vms_vector *pos,fix width,fix height,grs_bitmap *bm)
{
    vertex_tex_uni_t v[4];
	vms_vector pv,v1;
	int i;

	r_bitmapc++;
	v1.z=0;

	ogl_checkbmtex(bm);

	width = fixmul(width,Matrix_scale.x);
	height = fixmul(height,Matrix_scale.y);
	for (i=0;i<4;i++){
		vm_vec_sub(&v1,pos,&View_position);
		vm_vec_rotate(&pv,&v1,&View_matrix);
		switch (i){
			case 0:
				v[i].texcoord[0] = 0.0;
				v[i].texcoord[1] = 0.0;
				pv.x+=-width;
				pv.y+=height;
				break;
			case 1:
				v[i].texcoord[0] = bm->gltexture->u;
				v[i].texcoord[1] = 0.0;
				pv.x+=width;
				pv.y+=height;
				break;
			case 2:
				v[i].texcoord[0] = bm->gltexture->u;
				v[i].texcoord[1] = bm->gltexture->v;
				pv.x+=width;
				pv.y+=-height;
				break;
			case 3:
				v[i].texcoord[0] = 0.0;
				v[i].texcoord[1] = bm->gltexture->v;
				pv.x+=-width;
				pv.y+=-height;
				break;
		}

		v[i].position[0] =  f2glf(pv.x);
		v[i].position[1] =  f2glf(pv.y);
		v[i].position[2] = -f2glf(pv.z);
	}
	
	GLState.color4f[0] = 1.0;
	GLState.color4f[1] = 1.0;
	GLState.color4f[2] = 1.0;
	GLState.color4f[3] = (grd_curcanv->cv_fade_level >= GR_FADE_OFF)?1.0:(1.0 - (float)grd_curcanv->cv_fade_level / ((float)GR_FADE_LEVELS - 1.0));	

	GLState.mode = GL_TRIANGLE_FAN;
	GLState.view = POSITION_XYZ;
	GLState.count = 4;
	GLState.stride = sizeof(vertex_tex_uni_t);
	GLState.v_pos = &v[0].position[0];
	
	GLState.tex[0] = bm->gltexture->handle;
	GLState.texwrap[0] = GL_CLAMP_TO_EDGE;
	GLState.v_tex[0] = &v[0].texcoord[0];
	
	ogl_draw_arrays( GLState );

	return 0;
}

/*
 * Movies
 * Since this function will create a new texture each call, mipmapping can be very GPU intensive - so it has an optional setting for texture filtering.
 */
bool ogl_ubitblt_i(int dw,int dh,int dx,int dy, int sw, int sh, int sx, int sy, grs_bitmap * src, grs_bitmap * dest, int texfilt)
{
	vertex_tex_uni_t v[4];
	GLfloat xo,yo,xs,ys,u1,v1;
	ogl_texture tex;
	r_ubitbltc++;

	ogl_init_texture(&tex, sw, sh, OGL_FLAG_ALPHA);
	tex.prio = 0.0;
	tex.lw=src->bm_rowsize;

	u1=v1=0;

	dx+=dest->bm_x;
	dy+=dest->bm_y;
	xo=dx/(float)last_width;
	xs=dw/(float)last_width;
	yo=1.0-dy/(float)last_height;
	ys=dh/(float)last_height;

	ogl_pal=gr_current_pal;
	ogl_loadtexture(src->bm_data, sx, sy, &tex, src->bm_flags, 0, texfilt);
	ogl_pal=gr_palette;

	v[0].position[0] = xo;
	v[0].position[1] = yo;
	v[1].position[0] = xo+xs;
	v[1].position[1] = yo;
	v[2].position[0] = xo+xs;
	v[2].position[1] = yo-ys;
	v[3].position[0] = xo;
	v[3].position[1] = yo-ys;

	v[0].texcoord[0] = u1;
	v[0].texcoord[1] = v1;
	v[1].texcoord[0] = tex.u;
	v[1].texcoord[1] = v1;
	v[2].texcoord[0] = tex.u;
	v[2].texcoord[1] = tex.v;
	v[3].texcoord[0] = u1;
	v[3].texcoord[1] = tex.v;
	
	GLState.color4f[0] = 1.0;
	GLState.color4f[1] = 1.0;
	GLState.color4f[2] = 1.0;
	GLState.color4f[3] = 1.0;	

	GLState.mode = GL_TRIANGLE_FAN;
	GLState.view = POSITION_XY;
	GLState.count = 4;
	GLState.stride = sizeof(vertex_tex_uni_t);
	GLState.v_pos = &v[0].position[0];
	
	GLState.tex[0] = tex.handle;
	GLState.texwrap[0] = GL_CLAMP_TO_EDGE;
	GLState.v_tex[0] = &v[0].texcoord[0];	
	
	ogl_draw_arrays( GLState );

	ogl_freetexture(&tex);
	return 0;
}

bool ogl_ubitblt(int w,int h,int dx,int dy, int sx, int sy, grs_bitmap * src, grs_bitmap * dest){
	return ogl_ubitblt_i(w,h,dx,dy,w,h,sx,sy,src,dest,0);
}

/*
 * set depth testing on or off
 */
void ogl_toggle_depth_test(int enable)
{
	if (enable)
		glEnable(GL_DEPTH_TEST);
	else
		glDisable(GL_DEPTH_TEST);
}

/*
 * set blending function
 */
void ogl_set_blending()
{
	switch ( grd_curcanv->cv_blend_func )
	{
		case GR_BLEND_ADDITIVE_A:
			glBlendFunc( GL_SRC_ALPHA, GL_ONE );
			break;
		case GR_BLEND_ADDITIVE_C:
			glBlendFunc( GL_ONE, GL_ONE );
			break;
		case GR_BLEND_NORMAL:
		default:
			glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
			break;
	}
}

void ogl_start_frame(void){
	r_polyc=0;r_tpolyc=0;r_bitmapc=0;r_ubitbltc=0;r_upixelc=0;

	OGL_VIEWPORT(grd_curcanv->cv_bitmap.bm_x,grd_curcanv->cv_bitmap.bm_y,Canvas_width,Canvas_height);
	glClearColor(0.0, 0.0, 0.0, 0.0);

	glLineWidth(linedotscale);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GEQUAL,0.02);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	glClear(GL_DEPTH_BUFFER_BIT);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);

#if defined(OGL1)
	glShadeModel(GL_SMOOTH);
	glMatrixMode( GL_PROJECTION );
	glLoadMatrixf( ProjPersp.data() );
#endif
#if defined(OGL2)    
        Projection = ProjPersp;
#endif	
	ModelView.back() = cml::identity_4x4();
}

void ogl_end_frame(void){
	OGL_VIEWPORT(0,0,grd_curscreen->sc_w,grd_curscreen->sc_h);

#if defined(OGL1)
	glMatrixMode( GL_PROJECTION );
	glLoadMatrixf( ProjOrtho.data() );
#endif
#if defined(OGL2)    
        Projection = ProjOrtho;
#endif
	ModelView.back() = cml::identity_4x4();

	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
}

void gr_flip(void)
{
	if (GameArg.DbgRenderStats)
		ogl_texture_stats();

	ogl_do_palfx();
	ogl_swap_buffers_internal();
	glClear(GL_COLOR_BUFFER_BIT);
}

//little hack to find the nearest bigger power of 2 for a given number
unsigned pow2ize(unsigned f0){
	unsigned f1 = (f0 - 1) | 1;
	for (unsigned i = 4; i -- > 0;)
		f1 |= f1 >> (1 << i);
	unsigned f2 = f1 + 1;
	assert(f2 >= f0);
	assert(!(f2 & f1));
	assert((f2 >> 1) < f0);
	return f2;
}

// Allocate the pixel buffers 'pixels' and 'texbuf' based on current screen resolution
void ogl_init_pixel_buffers(unsigned w, unsigned h)
{
	w = pow2ize(w);	// convert to OpenGL texture size
	h = pow2ize(h);

	if (pixels)
		d_free(pixels);
	MALLOC(pixels, GLubyte, w*h*4);

	if (texbuf)
		d_free(texbuf);
	MALLOC(texbuf, GLubyte, max(w, 1024u)*max(h, 256u)*4);	// must also fit big font texture

	if ((pixels == NULL) || (texbuf == NULL))
		Error("Not enough memory for current resolution");
}

void ogl_close_pixel_buffers(void)
{
	d_free(pixels);
	d_free(texbuf);
}

static void ogl_filltexbuf(unsigned char *data, GLubyte *texp, unsigned truewidth, unsigned width, unsigned height, int dxo, int dyo, unsigned twidth, unsigned theight, int type, int bm_flags, int data_format)
{
	if ((width > max(static_cast<unsigned>(grd_curscreen->sc_w), 1024u)) || (height > max(static_cast<unsigned>(grd_curscreen->sc_h), 256u)))
		Error("Texture is too big: %ix%i", width, height);

	for (unsigned y=0;y<theight;y++)
	{
		int i=dxo+truewidth*(y+dyo);
		for (unsigned x=0;x<twidth;x++)
		{
			int c;
			if (x<width && y<height)
			{
				if (data_format)
				{
					int j;

					for (j = 0; j < data_format; ++j)
						(*(texp++)) = data[i * data_format + j];
					i++;
					continue;
				}
				else
				{
					c = data[i++];
				}
			}
			else if (x == width && y < height) // end of bitmap reached - fill this pixel with last color to make a clean border when filtering this texture
			{
				c = data[(width*(y+1))-1];
			}
			else if (y == height && x < width) // end of bitmap reached - fill this row with color or last row to make a clean border when filtering this texture
			{
				c = data[(width*(height-1))+x];
			}
			else
			{
				c = 256; // fill the pad space with transparency (or blackness)
			}

			if (c == 254 && (bm_flags & BM_FLAG_SUPER_TRANSPARENT))
			{
				switch (type)
				{
					case GL_LUMINANCE_ALPHA:
						(*(texp++)) = 255;
						(*(texp++)) = 0;
						break;
					case GL_RGBA:
						(*(texp++)) = 255;
						(*(texp++)) = 255;
						(*(texp++)) = 255;
						(*(texp++)) = 0; // transparent pixel
						break;
#ifndef OGLES
					case GL_COLOR_INDEX:
						(*(texp++)) = c;
						break;
#endif
					default:
						Error("ogl_filltexbuf unhandled super-transparent texformat\n");
						break;
				}
			}
			else if ((c == 255 && (bm_flags & BM_FLAG_TRANSPARENT)) || c == 256)
			{
				switch (type)
				{
					case GL_LUMINANCE:
						(*(texp++))=0;
						break;
					case GL_LUMINANCE_ALPHA:
						(*(texp++))=0;
						(*(texp++))=0;
						break;
					case GL_RGB:
						(*(texp++)) = 0;
						(*(texp++)) = 0;
						(*(texp++)) = 0;
						break;
					case GL_RGBA:
						(*(texp++))=0;
						(*(texp++))=0;
						(*(texp++))=0;
						(*(texp++))=0;//transparent pixel
						break;
#ifndef OGLES
					case GL_COLOR_INDEX:
						(*(texp++)) = c;
						break;
#endif
					default:
						Error("ogl_filltexbuf unknown texformat\n");
						break;
				}
			}
			else
			{
				switch (type)
				{
					case GL_LUMINANCE://these could prolly be done to make the intensity based upon the intensity of the resulting color, but its not needed for anything (yet?) so no point. :)
						(*(texp++))=255;
						break;
					case GL_LUMINANCE_ALPHA:
						(*(texp++))=255;
						(*(texp++))=255;
						break;
					case GL_RGB:
						(*(texp++)) = ogl_pal[c * 3] * 4;
						(*(texp++)) = ogl_pal[c * 3 + 1] * 4;
						(*(texp++)) = ogl_pal[c * 3 + 2] * 4;
						break;
					case GL_RGBA:
						(*(texp++))=ogl_pal[c*3]*4;
						(*(texp++))=ogl_pal[c*3+1]*4;
						(*(texp++))=ogl_pal[c*3+2]*4;
						(*(texp++))=255;//not transparent
						break;
#ifndef OGLES
					case GL_COLOR_INDEX:
						(*(texp++)) = c;
						break;
#endif
					default:
						Error("ogl_filltexbuf unknown texformat\n");
						break;
				}
			}
		}
	}
}

static void tex_set_size1(ogl_texture *tex,int dbits,int bits,int w, int h){
	int u;
	if (tex->tw!=w || tex->th!=h){
		u=(tex->w/(float)tex->tw*w) * (tex->h/(float)tex->th*h);
		glmprintf((0,"shrunken texture?\n"));
	}else
		u=tex->w*tex->h;
	if (bits<=0){//the beta nvidia GLX server. doesn't ever return any bit sizes, so just use some assumptions.
		tex->bytes=((float)w*h*dbits)/8.0;
		tex->bytesu=((float)u*dbits)/8.0;
	}else{
		tex->bytes=((float)w*h*bits)/8.0;
		tex->bytesu=((float)u*bits)/8.0;
	}
	glmprintf((0,"tex_set_size1: %ix%i, %ib(%i) %iB\n",w,h,bits,dbits,tex->bytes));
}

static void tex_set_size(ogl_texture *tex){
	GLint w,h;
	int bi=16,a=0;
#ifndef OGLES
	if (GameArg.DbgGlGetTexLevelParamOk){
		GLint t;
		glGetTexLevelParameteriv(GL_TEXTURE_2D,0,GL_TEXTURE_WIDTH,&w);
		glGetTexLevelParameteriv(GL_TEXTURE_2D,0,GL_TEXTURE_HEIGHT,&h);
		glGetTexLevelParameteriv(GL_TEXTURE_2D,0,GL_TEXTURE_LUMINANCE_SIZE,&t);a+=t;
		glGetTexLevelParameteriv(GL_TEXTURE_2D,0,GL_TEXTURE_INTENSITY_SIZE,&t);a+=t;
		glGetTexLevelParameteriv(GL_TEXTURE_2D,0,GL_TEXTURE_RED_SIZE,&t);a+=t;
		glGetTexLevelParameteriv(GL_TEXTURE_2D,0,GL_TEXTURE_GREEN_SIZE,&t);a+=t;
		glGetTexLevelParameteriv(GL_TEXTURE_2D,0,GL_TEXTURE_BLUE_SIZE,&t);a+=t;
		glGetTexLevelParameteriv(GL_TEXTURE_2D,0,GL_TEXTURE_ALPHA_SIZE,&t);a+=t;
	}
	else
#endif
	{
		w=tex->tw;
		h=tex->th;
	}
	switch (tex->format){
		case GL_LUMINANCE:
			bi=8;
			break;
		case GL_LUMINANCE_ALPHA:
			bi=8;
			break;
		case GL_RGB:
		case GL_RGBA:
			bi=16;
			break;
#ifndef OGLES
		case GL_COLOR_INDEX:
			bi = 8;
			break;
#endif
		default:
			Error("tex_set_size unknown texformat\n");
			break;
	}
	tex_set_size1(tex,bi,a,w,h);
}

//loads a palettized bitmap into a ogl RGBA texture.
//Sizes and pads dimensions to multiples of 2 if necessary.
//In theory this could be a problem for repeating textures, but all real
//textures (not sprites, etc) in descent are 64x64, so we are ok.
//stores OpenGL textured id in *texid and u/v values required to get only the real data in *u/*v
static int ogl_loadtexture (unsigned char *data, int dxo, int dyo, ogl_texture *tex, int bm_flags, int data_format, int texfilt)
{
	GLubyte	*bufP = texbuf;
	tex->tw = pow2ize (tex->w);
	tex->th = pow2ize (tex->h);//calculate smallest texture size that can accomodate us (must be multiples of 2)

	//calculate u/v values that would make the resulting texture correctly sized
	tex->u = (float) ((double) tex->w / (double) tex->tw);
	tex->v = (float) ((double) tex->h / (double) tex->th);

	if (data) {
		if (bm_flags >= 0)
			ogl_filltexbuf (data, texbuf, tex->lw, tex->w, tex->h, dxo, dyo, tex->tw, tex->th,
								 tex->format, bm_flags, data_format);
		else {
			if (!dxo && !dyo && (tex->w == tex->tw) && (tex->h == tex->th))
				bufP = data;
			else {
				int h, w, tw;

				h = tex->lw / tex->w;
				w = (tex->w - dxo) * h;
				data += tex->lw * dyo + h * dxo;
				bufP = texbuf;
				tw = tex->tw * h;
				h = tw - w;
				for (; dyo < tex->h; dyo++, data += tex->lw) {
					memcpy (bufP, data, w);
					bufP += w;
					memset (bufP, 0, h);
					bufP += h;
				}
				memset (bufP, 0, tex->th * tw - (bufP - texbuf));
				bufP = texbuf;
			}
		}
	}
	// Generate OpenGL texture IDs.
	glGenTextures (1, &tex->handle);

	// Give our data to OpenGL.
	glBindTexture( GL_TEXTURE_2D, tex->handle);
#if defined(OG1)
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
#endif	

	if (texfilt)
	{
#if defined(OGL1)
		glTexParameteri (GL_TEXTURE_2D, GL_GENERATE_MIPMAP, texfilt ? GL_TRUE : GL_FALSE);
#endif
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (texfilt>=2?GL_LINEAR_MIPMAP_LINEAR:GL_LINEAR_MIPMAP_NEAREST));
#ifndef OGLES
		if (texfilt >= 3 && ogl_maxanisotropy > 1.0)
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, ogl_maxanisotropy);
#endif
	}
	else
	{
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	}

	glTexImage2D (
	    GL_TEXTURE_2D, 0, tex->internalformat,
	    tex->tw, tex->th, 0, tex->format, // RGBA textures.
	    GL_UNSIGNED_BYTE, // imageData is a GLubyte pointer.
	    bufP);

#if defined(OGL2)
	glGenerateMipmap( GL_TEXTURE_2D );
#endif

	tex_set_size (tex);
	r_texcount++;
	return 0;
}

unsigned char decodebuf[1024*1024];

void ogl_loadbmtexture_f(grs_bitmap *bm, int texfilt)
{
	unsigned char *buf;
#ifdef HAVE_LIBPNG
	char *bitmapname;
#endif

	while (bm->bm_parent)
		bm=bm->bm_parent;
	if (bm->gltexture && bm->gltexture->handle > 0)
		return;
	buf=bm->bm_data;
#ifdef HAVE_LIBPNG
	if ((bitmapname = piggy_game_bitmap_name(bm)))
	{
		char filename[64];
		png_data pdata;

		sprintf(filename, "textures/%s.png", bitmapname);
		if (read_png(filename, &pdata))
		{
			con_printf(CON_DEBUG,"%s: %ux%ux%i p=%i(%i) c=%i a=%i chans=%i\n", filename, pdata.width, pdata.height, pdata.depth, pdata.paletted, pdata.num_palette, pdata.color, pdata.alpha, pdata.channels);
			if (pdata.depth == 8 && pdata.color)
			{
				if (bm->gltexture == NULL)
					ogl_init_texture(bm->gltexture = ogl_get_free_texture(), pdata.width, pdata.height, flags | ((pdata.alpha || bm->bm_flags & BM_FLAG_TRANSPARENT) ? OGL_FLAG_ALPHA : 0));
				ogl_loadtexture(pdata.data, 0, 0, bm->gltexture, bm->bm_flags, pdata.paletted ? 0 : pdata.channels, texfilt);
				free(pdata.data);
				if (pdata.palette)
					free(pdata.palette);
				return;
			}
			else
			{
				con_printf(CON_DEBUG,"%s: unsupported texture format: must be rgb, rgba, or paletted, and depth 8\n", filename);
				free(pdata.data);
				if (pdata.palette)
					free(pdata.palette);
			}
		}
	}
#endif
	if (bm->gltexture == NULL){
 		ogl_init_texture(bm->gltexture = ogl_get_free_texture(), bm->bm_w, bm->bm_h, ((bm->bm_flags & (BM_FLAG_TRANSPARENT | BM_FLAG_SUPER_TRANSPARENT))? OGL_FLAG_ALPHA : 0));
	}
	else {
		if (bm->gltexture->handle>0)
			return;
		if (bm->gltexture->w==0){
			bm->gltexture->lw=bm->bm_w;
			bm->gltexture->w=bm->bm_w;
			bm->gltexture->h=bm->bm_h;
		}
	}

	if (bm->bm_flags & BM_FLAG_RLE){
		unsigned char * dbits;
		unsigned char * sbits;
		int i, data_offset;

		data_offset = 1;
		if (bm->bm_flags & BM_FLAG_RLE_BIG)
			data_offset = 2;

		sbits = &bm->bm_data[4 + (bm->bm_h * data_offset)];
		dbits = decodebuf;

		for (i=0; i < bm->bm_h; i++ )    {
			gr_rle_decode(sbits,dbits);
			if ( bm->bm_flags & BM_FLAG_RLE_BIG )
				sbits += (int)INTEL_SHORT(*((short *)&(bm->bm_data[4+(i*data_offset)])));
			else
				sbits += (int)bm->bm_data[4+i];
			dbits += bm->bm_w;
		}
		buf=decodebuf;
	}
	ogl_loadtexture(buf, 0, 0, bm->gltexture, bm->bm_flags, 0, texfilt);
}

static void ogl_freetexture(ogl_texture *gltexture)
{
	if (gltexture->handle>0) {
		r_texcount--;
		glmprintf((0,"ogl_freetexture(%p):%i (%i left)\n",gltexture,gltexture->handle,r_texcount));
		glDeleteTextures( 1, &gltexture->handle );
//		gltexture->handle=0;
		ogl_reset_texture(gltexture);
	}
}
void ogl_freebmtexture(grs_bitmap *bm){
	if (bm->gltexture){
		ogl_freetexture(bm->gltexture);
		bm->gltexture=NULL;
	}
}

/*
 * Menu / gauges
 */
bool ogl_ubitmapm_cs(int x, int y,int dw, int dh, grs_bitmap *bm,int c, int scale) // to scale bitmaps
{
	vertex_tex_uni_t v[4];
	GLfloat xo,yo,xf,yf,u1,u2,v1,v2,color_r,color_g,color_b,h;

	x+=grd_curcanv->cv_bitmap.bm_x;
	y+=grd_curcanv->cv_bitmap.bm_y;
	xo=x/(float)last_width;
	xf=(bm->bm_w+x)/(float)last_width;
	yo=1.0-y/(float)last_height;
	yf=1.0-(bm->bm_h+y)/(float)last_height;

	if (dw < 0)
		dw = grd_curcanv->cv_bitmap.bm_w;
	else if (dw == 0)
		dw = bm->bm_w;
	if (dh < 0)
		dh = grd_curcanv->cv_bitmap.bm_h;
	else if (dh == 0)
		dh = bm->bm_h;

	h = (double) scale / (double) F1_0;

	xo = x / ((double) last_width * h);
	xf = (dw + x) / ((double) last_width * h);
	yo = 1.0 - y / ((double) last_height * h);
	yf = 1.0 - (dh + y) / ((double) last_height * h);

	ogl_checkbmtex(bm);

	if (bm->bm_x==0){
		u1=0;
		if (bm->bm_w==bm->gltexture->w)
			u2=bm->gltexture->u;
		else
			u2=(bm->bm_w+bm->bm_x)/(float)bm->gltexture->tw;
	}else {
		u1=bm->bm_x/(float)bm->gltexture->tw;
		u2=(bm->bm_w+bm->bm_x)/(float)bm->gltexture->tw;
	}
	if (bm->bm_y==0){
		v1=0;
		if (bm->bm_h==bm->gltexture->h)
			v2=bm->gltexture->v;
		else
			v2=(bm->bm_h+bm->bm_y)/(float)bm->gltexture->th;
	}else{
		v1=bm->bm_y/(float)bm->gltexture->th;
		v2=(bm->bm_h+bm->bm_y)/(float)bm->gltexture->th;
	}

	if (c < 0) {
		color_r = 1.0;
		color_g = 1.0;
		color_b = 1.0;
	} else {
		color_r = CPAL2Tr(c);
		color_g = CPAL2Tg(c);
		color_b = CPAL2Tb(c);
	}

	v[0].position[0] = xo;
	v[0].position[1] = yo;
	v[1].position[0] = xf;
	v[1].position[1] = yo;
	v[2].position[0] = xf;
	v[2].position[1] = yf;
	v[3].position[0] = xo;
	v[3].position[1] = yf;

	v[0].texcoord[0] = u1;
	v[0].texcoord[1] = v1;
	v[1].texcoord[0] = u2;
	v[1].texcoord[1] = v1;
	v[2].texcoord[0] = u2;
	v[2].texcoord[1] = v2;
	v[3].texcoord[0] = u1;
	v[3].texcoord[1] = v2;
	
	GLState.color4f[0] = color_r;
	GLState.color4f[1] = color_g;
	GLState.color4f[2] = color_b;
	GLState.color4f[3] = 1.0;	

	GLState.mode = GL_TRIANGLE_FAN;
	GLState.view = POSITION_XY;
	GLState.count = 4;
	GLState.stride = sizeof(vertex_tex_uni_t);
	GLState.v_pos = &v[0].position[0];
	
	GLState.tex[0] = bm->gltexture->handle;
	GLState.texwrap[0] = GL_CLAMP_TO_EDGE;
	GLState.v_tex[0] = &v[0].texcoord[0];
	
	ogl_draw_arrays( GLState );

	return 0;
}

#if defined(OGL1)
void ogl1_draw_arrays( const glstate_t& state )
{
    uint8_t p = (state.view >= POSITION_XYZ) ? 3 : 2;

    /* Setup and draw the array */
    if (state.mode == GL_POINTS) {
	glPointSize( state.pointsize );
    }
    
    if (state.v_pos != NULL) {
        glEnableClientState( GL_VERTEX_ARRAY );
	glVertexPointer( p, GL_FLOAT, state.stride, state.v_pos );
    }

    if (state.v_clr != NULL) {
        glEnableClientState( GL_COLOR_ARRAY );
	glColorPointer( 4, GL_UNSIGNED_BYTE, state.stride, state.v_clr );
    } else {
	glColor4f( state.color4f[0], state.color4f[1], state.color4f[2], state.color4f[3] );
    }
      
    /* First texture unit */
    if (state.v_tex[0] != NULL) {
        glActiveTexture( GL_TEXTURE0 );
	glEnable( GL_TEXTURE_2D );
        glBindTexture( GL_TEXTURE_2D, state.tex[0] );
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, state.texwrap[0]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, state.texwrap[0]);	
	
	if (state.v_tex[1] != NULL) {
	  
	  // Interpolate both textures based on alpha from texture 1
	  // This use crossbar feature to combine textures into one image	  
	  glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
	  	  
	  glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_INTERPOLATE);
	  glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_TEXTURE1);
	  glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB, GL_TEXTURE0);
	  glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE2_RGB, GL_TEXTURE1);
	  glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);
	  glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR);
	  glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND2_RGB, GL_SRC_ALPHA);

	  glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_MODULATE);
	  glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA, GL_PREVIOUS);
	  glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_ALPHA, GL_TEXTURE0);
	  glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA);
	  glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_ALPHA, GL_SRC_ALPHA);

	} else {
	  glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
	}

	glClientActiveTexture( GL_TEXTURE0 );
        glEnableClientState( GL_TEXTURE_COORD_ARRAY );
	glTexCoordPointer( 2, GL_FLOAT, state.stride, state.v_tex[0] );
    }
    /* Second texture unit */
    if (state.v_tex[1] != NULL) {
        glActiveTexture( GL_TEXTURE1 );
	glEnable( GL_TEXTURE_2D );
	glBindTexture( GL_TEXTURE_2D, state.tex[1] );
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, state.texwrap[1]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, state.texwrap[1]);	
	
	// Modulate previous texture result with the vertex color
	// The previous color was full bright, this gives the lighting effect
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
	
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_PREVIOUS);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB, GL_PRIMARY_COLOR);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR);

	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_MODULATE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA, GL_PREVIOUS);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_ALPHA, GL_PRIMARY_COLOR);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_ALPHA, GL_SRC_ALPHA);

	glClientActiveTexture( GL_TEXTURE1 );	
        glEnableClientState( GL_TEXTURE_COORD_ARRAY );
	glTexCoordPointer( 2, GL_FLOAT, state.stride, state.v_tex[1] );	
    }

    /* Calculate the complete transform matrix */
    glMatrixMode( GL_MODELVIEW );
    glLoadMatrixf( ModelView.back().data() );

    glDrawArrays( state.mode, 0, state.count );

    if (state.v_pos != NULL) {
        glDisableClientState( GL_VERTEX_ARRAY );
    }
    if (state.v_clr != NULL) {
        glDisableClientState( GL_COLOR_ARRAY );
    }
    if (state.v_tex[1] != NULL) {
        glDisableClientState( GL_TEXTURE_COORD_ARRAY );
	glDisable( GL_TEXTURE_2D );
    }
    if (state.v_tex[0] != NULL) {
        glActiveTexture( GL_TEXTURE0 );
	glClientActiveTexture( GL_TEXTURE0 );
	glDisable( GL_TEXTURE_2D );
        glDisableClientState( GL_TEXTURE_COORD_ARRAY );
    }
    
    /* Reset the state */
    GLState.v_pos    = NULL;
    GLState.v_clr    = NULL;
    GLState.v_tex[0] = NULL;
    GLState.v_tex[1] = NULL;
    
    check_opengl_errors( __FILE__, __LINE__ );
}
#endif /* OGL1 */

#if defined(OGL2)
void ogl2_draw_arrays( const glstate_t& state )
{
    mat44f_t matMVP;
    uint8_t p = (state.view >= POSITION_XYZ) ? 3 : 2;

    /* Select the shader program */
    if (state.v_pos == NULL) {
        printf( "Shader OGL2 Error: position data can not be NULL\n" );
        return;
    } else
    if (state.v_clr == NULL && state.v_tex[0] == NULL && state.v_tex[1] == NULL) {
        og12_use_shader(PROGRAM_COLOR_UNIFORM);
    } else      
    if (state.v_clr != NULL && state.v_tex[0] == NULL && state.v_tex[1] == NULL) {
        og12_use_shader(PROGRAM_COLOR);
    } else
    if (state.v_clr == NULL && state.v_tex[0] != NULL && state.v_tex[1] == NULL) {
	og12_use_shader(PROGRAM_TEXTURE_UNIFORM);
    } else
    if (state.v_clr != NULL && state.v_tex[0] != NULL && state.v_tex[1] == NULL) {
	og12_use_shader(PROGRAM_TEXTURE);
    } else
    if (state.v_clr != NULL && state.v_tex[0] != NULL && state.v_tex[1] != NULL) {	
        og12_use_shader(PROGRAM_MULTITEXTURE);
    } else {
        printf( "Shader OGL2 Error: unsupported data configuration\n" );
        return;
    }

    /* Buffer the data */
    //Buffer[PROGRAM_MULTITEXTURE].BufferData( 0, sizeof(vertex_mtex_t)*count, vtx );

    /* Setup and draw the array */
    if (state.mode == GL_POINTS) {
	glEnable( GL_PROGRAM_POINT_SIZE );
	glUniform1f( Shaders[ProgramCurrent].NamePs, state.pointsize );
    }
    
    if (state.v_pos != NULL) {
        glEnableVertexAttribArray( Shaders[ProgramCurrent].NamePos );
        glVertexAttribPointer( Shaders[ProgramCurrent].NamePos, p, GL_FLOAT, GL_FALSE, state.stride, state.v_pos );
    }

    if (state.v_clr != NULL) {
        glEnableVertexAttribArray( Shaders[ProgramCurrent].NameClr );
        glVertexAttribPointer( Shaders[ProgramCurrent].NameClr, 4, GL_UNSIGNED_BYTE, GL_TRUE, state.stride, state.v_clr );
    } else {
	glUniform4fv( Shaders[ProgramCurrent].NameClr, 1, state.color4f.data() );
    }
      
    /* First texture unit */
    if (state.v_tex[0] != NULL) {
        glActiveTexture( GL_TEXTURE0 );
        glBindTexture( GL_TEXTURE_2D, state.tex[0] );
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, state.texwrap[0]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, state.texwrap[0]);
	glUniform1i( Shaders[ProgramCurrent].NameSmp0, 0 );
        glEnableVertexAttribArray( Shaders[ProgramCurrent].NameTex0 );
        glVertexAttribPointer( Shaders[ProgramCurrent].NameTex0,  2, GL_FLOAT, GL_FALSE, state.stride, state.v_tex[0] );
    }
    /* Second texture unit */
    if (state.v_tex[1] != NULL) {
        glActiveTexture( GL_TEXTURE1 );
        glBindTexture( GL_TEXTURE_2D, state.tex[1] );
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, state.texwrap[1]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, state.texwrap[1]);
	glUniform1i( Shaders[ProgramCurrent].NameSmp1, 1 );
        glEnableVertexAttribArray( Shaders[ProgramCurrent].NameTex1 );
        glVertexAttribPointer( Shaders[ProgramCurrent].NameTex1, 2, GL_FLOAT, GL_FALSE, state.stride, state.v_tex[1] );
    }

    /* Calculate the complete transform matrix */
    matMVP = Projection * ModelView.back();
    glUniformMatrix4fv( Shaders[ProgramCurrent].NameMvp, 1, GL_FALSE, matMVP.data() );

    glDrawArrays( state.mode, 0, state.count );

    if (state.v_pos != NULL) {
        glDisableVertexAttribArray( Shaders[ProgramCurrent].NamePos );
    }
    if (state.v_clr != NULL) {
        glDisableVertexAttribArray( Shaders[ProgramCurrent].NameClr );
    }
    if (state.v_tex[1] != NULL) {
        glDisableVertexAttribArray( Shaders[ProgramCurrent].NameTex1 );
    }
    if (state.v_tex[0] != NULL) {
        glActiveTexture( GL_TEXTURE0 );
        glDisableVertexAttribArray( Shaders[ProgramCurrent].NameTex0 );
    }
    if (state.mode == GL_POINTS) {
	glDisable( GL_PROGRAM_POINT_SIZE );
    }    
    
    /* Reset the state */
    GLState.v_pos    = NULL;
    GLState.v_clr    = NULL;
    GLState.v_tex[0] = NULL;
    GLState.v_tex[1] = NULL;
    
    check_opengl_errors( __FILE__, __LINE__ );    
}

bool ogl2_shader_compile( void )
{
	  // Compile the shader code and link into a program
	if (Shaders[PROGRAM_COLOR].Load( "shaders/shader_color.vert", "shaders/shader_color.frag" ) != 0) {
	    return false;
	}
	if (Shaders[PROGRAM_COLOR_UNIFORM].Load( "shaders/shader_color_uniform.vert", "shaders/shader_color_uniform.frag" ) != 0) {
	    return false;
	}	
	if (Shaders[PROGRAM_TEXTURE].Load( "shaders/shader_texture.vert", "shaders/shader_texture.frag" ) != 0) {
	    return false;
	}
	if (Shaders[PROGRAM_TEXTURE_UNIFORM].Load( "shaders/shader_texture_uniform.vert", "shaders/shader_texture_uniform.frag" ) != 0) {
	    return false;
	}	
	if (Shaders[PROGRAM_MULTITEXTURE].Load( "shaders/shader_multitexture.vert", "shaders/shader_multitexture.frag" ) != 0) {
	    return false;
	}

	// Get names for attributes and uniforms
	Shaders[PROGRAM_COLOR].NamePos = Shaders[PROGRAM_COLOR].GetAttribute( "a_Position" );
	Shaders[PROGRAM_COLOR].NameClr = Shaders[PROGRAM_COLOR].GetAttribute( "a_Color" );
	Shaders[PROGRAM_COLOR].NameMvp = Shaders[PROGRAM_COLOR].GetUniform( "u_MVPMatrix" );
	Shaders[PROGRAM_COLOR].NamePs  = Shaders[PROGRAM_COLOR].GetUniform( "u_PointSize" );
	
	Shaders[PROGRAM_COLOR_UNIFORM].NamePos = Shaders[PROGRAM_COLOR_UNIFORM].GetAttribute( "a_Position" );
	Shaders[PROGRAM_COLOR_UNIFORM].NameClr = Shaders[PROGRAM_COLOR_UNIFORM].GetUniform( "u_Color" );
	Shaders[PROGRAM_COLOR_UNIFORM].NameMvp = Shaders[PROGRAM_COLOR_UNIFORM].GetUniform( "u_MVPMatrix" );	
	Shaders[PROGRAM_COLOR_UNIFORM].NamePs  = Shaders[PROGRAM_COLOR_UNIFORM].GetUniform( "u_PointSize" );

	Shaders[PROGRAM_TEXTURE].NamePos  = Shaders[PROGRAM_TEXTURE].GetAttribute( "a_Position" );
	Shaders[PROGRAM_TEXTURE].NameClr  = Shaders[PROGRAM_TEXTURE].GetAttribute( "a_Color" );
	Shaders[PROGRAM_TEXTURE].NameTex0 = Shaders[PROGRAM_TEXTURE].GetAttribute( "a_Texcoord0" );
	Shaders[PROGRAM_TEXTURE].NameMvp  = Shaders[PROGRAM_TEXTURE].GetUniform( "u_MVPMatrix" );
	Shaders[PROGRAM_TEXTURE].NameSmp0 = Shaders[PROGRAM_TEXTURE].GetUniform( "u_Texture0" );

	Shaders[PROGRAM_TEXTURE_UNIFORM].NamePos  = Shaders[PROGRAM_TEXTURE_UNIFORM].GetAttribute( "a_Position" );
	Shaders[PROGRAM_TEXTURE_UNIFORM].NameTex0 = Shaders[PROGRAM_TEXTURE_UNIFORM].GetAttribute( "a_Texcoord0" );
	Shaders[PROGRAM_TEXTURE_UNIFORM].NameClr  = Shaders[PROGRAM_TEXTURE_UNIFORM].GetUniform( "u_Color" );
	Shaders[PROGRAM_TEXTURE_UNIFORM].NameMvp  = Shaders[PROGRAM_TEXTURE_UNIFORM].GetUniform( "u_MVPMatrix" );
	Shaders[PROGRAM_TEXTURE_UNIFORM].NameSmp0 = Shaders[PROGRAM_TEXTURE_UNIFORM].GetUniform( "u_Texture0" );	

	Shaders[PROGRAM_MULTITEXTURE].NamePos   = Shaders[PROGRAM_MULTITEXTURE].GetAttribute( "a_Position" );
	Shaders[PROGRAM_MULTITEXTURE].NameClr   = Shaders[PROGRAM_MULTITEXTURE].GetAttribute( "a_Color" );
	Shaders[PROGRAM_MULTITEXTURE].NameTex0  = Shaders[PROGRAM_MULTITEXTURE].GetAttribute( "a_Texcoord0" );
	Shaders[PROGRAM_MULTITEXTURE].NameTex1  = Shaders[PROGRAM_MULTITEXTURE].GetAttribute( "a_Texcoord1" );
	Shaders[PROGRAM_MULTITEXTURE].NameMvp   = Shaders[PROGRAM_MULTITEXTURE].GetUniform( "u_MVPMatrix" );
	Shaders[PROGRAM_MULTITEXTURE].NameSmp0  = Shaders[PROGRAM_MULTITEXTURE].GetUniform( "u_Texture0" );
	Shaders[PROGRAM_MULTITEXTURE].NameSmp1  = Shaders[PROGRAM_MULTITEXTURE].GetUniform( "u_Texture1" );
	
	check_opengl_errors( __FILE__, __LINE__ );
	
	return true;
}

void og12_use_shader( uint8_t program )
{
	// Check if the program is alreadt in use
	if (ProgramCurrent != program) {
	    Shaders[program].Use();
	}
	ProgramCurrent = program;
}
#endif /* OGL2 */

void ogl_scale( GLfloat x, GLfloat y, GLfloat z )
{
    mat44f_t matrix;

    cml::matrix_scale( matrix, x, y, z );

    ModelView.back() *= matrix;
}

void ogl_translate( GLfloat x, GLfloat y, GLfloat z )
{
    mat44f_t matrix;

    cml::matrix_translation( matrix, x, y, z );

    ModelView.back() *= matrix;
}

int8_t check_opengl_errors( const string& file, uint16_t line )
{
    GLenum error;
    string errortext;

    error = glGetError();

    if (error != GL_NO_ERROR)
    {
        switch (error)
        {
            case GL_INVALID_ENUM:                   errortext = "GL_INVALID_ENUM"; break;
            case GL_INVALID_VALUE:                  errortext = "GL_INVALID_VALUE"; break;
            case GL_INVALID_OPERATION:              errortext = "GL_INVALID_OPERATION"; break;
#if defined(USE_GLES1) || defined(USE_GLFULL)
            case GL_STACK_OVERFLOW:                 errortext = "GL_STACK_OVERFLOW"; break;
            case GL_STACK_UNDERFLOW:                errortext = "GL_STACK_UNDERFLOW"; break;
#endif
#if defined(USE_GLES2) || defined(USE_GLFULL)
            case GL_INVALID_FRAMEBUFFER_OPERATION:  errortext = "GL_INVALID_FRAMEBUFFER_OPERATION"; break;
#endif
            case GL_OUT_OF_MEMORY:                  errortext = "GL_OUT_OF_MEMORY"; break;
            default:                                errortext = "unknown"; break;
        }

        printf( "ERROR: OpenGL Error detected in file %s at line %d: %s (0x%X)\n", file.c_str(), line, errortext.c_str(), error );

        return 1;
    }
    return 0;
}
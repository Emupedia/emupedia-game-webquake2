/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
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
#ifndef __REF_H
#define __REF_H

#include "../qcommon/qcommon.h"

#define	EXTENDED_API_VERSION	2

#define	MAX_DLIGHTS		32
#define	MAX_ENTITIES	128
//#define	MAX_PARTICLES	16384
#define	MAX_LIGHTSTYLES	256

#define POWERSUIT_SCALE		4.0F

#define SHELL_RED_COLOR		0xF2
#define SHELL_GREEN_COLOR	0xD0
#define SHELL_BLUE_COLOR	0xF3

#define SHELL_RG_COLOR		0xDC
//#define SHELL_RB_COLOR		0x86
#define SHELL_RB_COLOR		0x68
#define SHELL_BG_COLOR		0x78

//ROGUE
#define SHELL_DOUBLE_COLOR	0xDF // 223
#define	SHELL_HALF_DAM_COLOR	0x90
#define SHELL_CYAN_COLOR	0x72
//ROGUE

#define SHELL_WHITE_COLOR	0xD7

typedef struct entity_s
{
	struct model_s		*model;			// opaque type outside refresh
	float				angles[3];

	/*
	** most recent data
	*/
	float				origin[3];		// also used as RF_BEAM's "from"
	int					frame;			// also used as RF_BEAM's diameter

	/*
	** previous data for lerping
	*/
	float				oldorigin[3];	// also used as RF_BEAM's "to"
	int					oldframe;

	/*
	** misc
	*/
	float	backlerp;				// 0.0 = current, 1.0 = old
	int		skinnum;				// also used as RF_BEAM's palette index

	int		lightstyle;				// for flashing entities
	float	alpha;					// ignore if RF_TRANSLUCENT isn't set

	struct image_s	*skin;			// NULL for inline skin
	int		flags;

	//WARNING: This struct size cannot be changed.
} entity_t;

#define ENTITY_FLAGS  68

typedef struct
{
	vec3_t	origin;
	vec3_t	color;
	float	intensity;
} dlight_t;

typedef struct
{
	vec3_t	origin;
	int		color;
	float	alpha;
} particle_t;

typedef struct
{
	float		rgb[3];			// 0.0 - 2.0
	float		white;			// highest of rgb
} lightstyle_t;

typedef struct
{
	int			x, y, width, height;// in virtual screen coordinates
	float		fov_x, fov_y;
	float		vieworg[3];
	float		viewangles[3];
	float		blend[4];			// rgba 0-1 full screen blend
	float		time;				// time is uesed to auto animate
	int			rdflags;			// RDF_UNDERWATER, etc

	byte		*areabits;			// if not NULL, only areas with set bits will be drawn

	lightstyle_t	*lightstyles;	// [MAX_LIGHTSTYLES]

	int			num_entities;
	entity_t	*entities;

	int			num_dlights;
	dlight_t	*dlights;

	int			num_particles;
	particle_t	*particles;
} refdef_t;


typedef struct in_state {
	// Pointers to functions back in client, set by vid_so
	void (*IN_CenterView_fp)(void);
	vec_t *viewangles;
	int *in_strafe_state;
} in_state_t;


void RW_IN_Init(in_state_t *in_state_p);
void RW_IN_Shutdown(void);
void RW_IN_Commands(void);
void RW_IN_Frame(void);

void KBD_Update(void);
void KBD_Close(void);


typedef struct vrect_s
{
	int32				x, y, width, height;
} vrect_t;

#define __VIDDEF_T
typedef struct
{
	uint32		width, height;			// coordinates from main game
} viddef_t;

extern	viddef_t	viddef;				// global video state

// Video module initialisation etc
void	VID_Init(void);
void	VID_Shutdown(void);
void	VID_CheckChanges(void);
void VID_Error (int err_level, const char *fmt, ...) __attribute__((format (printf, 2, 3), noreturn));
void VID_NewWindow ( int width, int height);

void	VID_MenuInit(void);
void	VID_MenuDraw(void);
const char *VID_MenuKey(int);
void VID_Printf (int print_level, const char *fmt, ...) __attribute__((format (printf, 2, 3)));


	// All data that will be used in a level should be
	// registered before rendering any frames to prevent disk hits,
	// but they can still be registered at a later time
	// if necessary.
	//
	// EndRegistration will free any remaining data that wasn't registered.
	// Any model_s or skin_s pointers from before the BeginRegistration
	// are no longer valid after EndRegistration.
	//
	// Skins and images need to be differentiated, because skins
	// are flood filled to eliminate mip map edge errors, and pics have
	// an implicit "pics/" prepended to the name. (a pic name that starts with a
	// slash will not use the "pics/" prefix or the ".pcx" postfix)
void	R_BeginRegistration (char *map);
struct model_s	* R_RegisterModel(const char *name);
struct image_s	* R_RegisterSkin(const char *name);
struct image_s	* Draw_FindPic (char *name);
void R_SetSky (char *name, float rotate, vec3_t axis);
void R_EndRegistration (void);


void R_RenderFrame (refdef_t *fd);


void	R_DrawString(int x, int y, const char *s, int xorVal, unsigned int maxLen);
void	R_DrawChar (int x, int y, int c);
void	Draw_FadeScreen (void);
void	Draw_Fill (int x, int y, int w, int h, int c);
void	Draw_TileClear (int x, int y, int w, int h, char *name);
void	Draw_StretchPic (int x, int y, int w, int h, char *name);
void	Draw_Pic (int x, int y, char *name);
void	Draw_GetPicSize (int *w, int *h, char *name);


int 	R_Init( void *hinstance, void *hWnd );
void	R_Shutdown( void );
void	R_SetPalette ( const unsigned char *palette);
void	R_BeginFrame(void);
void GLimp_EndFrame(void);


#endif // __REF_H

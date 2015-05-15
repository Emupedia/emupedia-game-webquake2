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
// r_main.c
#include "gl_local.h"

#ifndef WIN32
#define __stdcall
#endif

//long (*Q_ftol)(float f);

void R_Clear (void);

refimport_t		ri;
refimportnew_t	rx;

model_t		*r_worldmodel;

double		gldepthmin, gldepthmax;

double		vid_scaled_width, vid_scaled_height;

glconfig_t gl_config;
glstate_t  gl_state;

image_t		*r_notexture;		// use for bad textures
image_t		*r_particletexture;	// little dot for particles

entity_t	*currententity;
model_t		*currentmodel;

cplane_t	frustum[4];

int			r_visframecount;	// bumped when going to a new PVS
int			r_framecount;		// used for dlight push checking

int			c_brush_polys, c_alias_polys;

float		v_blend[4];			// final blending color

void GL_Strings_f( void );

//
// view origin
//
vec3_t	vup;
vec3_t	vpn;
vec3_t	vright;
vec3_t	r_origin;

float	r_world_matrix[16];
float	r_base_world_matrix[16];

//
// screen size info
//
refdef_t	r_newrefdef;

int		r_viewcluster, r_viewcluster2, r_oldviewcluster, r_oldviewcluster2;

cvar_t	*r_norefresh;
cvar_t	*r_drawentities;
cvar_t	*r_drawworld;
cvar_t	*r_speeds;
cvar_t	*r_fullbright;
cvar_t	*r_novis;
cvar_t	*r_nocull;
cvar_t	*r_lerpmodels;
cvar_t	*r_lefthand;

cvar_t	*r_lightlevel;	// FIXME: This is a HACK to get the client's light level

//cvar_t	*gl_nosubimage;
cvar_t	*gl_allow_software;

cvar_t	*gl_particle_min_size;
cvar_t	*gl_particle_max_size;
cvar_t	*gl_particle_size;
cvar_t	*gl_particle_att_a;
cvar_t	*gl_particle_att_b;
cvar_t	*gl_particle_att_c;

//cvar_t	*gl_ext_swapinterval;
cvar_t	*gl_ext_multitexture;
cvar_t	*gl_ext_pointparameters;
//cvar_t	*gl_ext_compiled_vertex_array;

//r1ch: my extensions
//cvar_t	*gl_ext_generate_mipmap;
cvar_t	*gl_ext_point_sprite;
cvar_t	*gl_ext_texture_filter_anisotropic;
cvar_t	*gl_ext_texture_non_power_of_two;
cvar_t	*gl_ext_max_anisotropy;
cvar_t	*gl_ext_nv_multisample_filter_hint;

cvar_t	*gl_colorbits;
cvar_t	*gl_alphabits;
cvar_t	*gl_depthbits;
cvar_t	*gl_stencilbits;

cvar_t	*gl_ext_multisample;
cvar_t	*gl_ext_samples;

cvar_t	*gl_zfar;
cvar_t	*gl_hudscale;

cvar_t	*cl_version;
cvar_t	*gl_r1gl_test;
cvar_t	*gl_doublelight_entities;
cvar_t	*gl_noscrap;
cvar_t	*gl_overbrights;
cvar_t	*gl_linear_mipmaps;

cvar_t	*vid_gamma_pics;

cvar_t	*gl_forcewidth;
cvar_t	*gl_forceheight;

cvar_t	*vid_topmost;

cvar_t	*gl_bitdepth;
//cvar_t	*gl_lightmap;
cvar_t	*gl_shadows;
cvar_t	*gl_mode;
cvar_t	*gl_dynamic;
//cvar_t  *gl_monolightmap;
cvar_t	*gl_modulate;
cvar_t	*gl_nobind;
cvar_t	*gl_round_down;
cvar_t	*gl_picmip;
cvar_t	*gl_skymip;
cvar_t	*gl_showtris;
cvar_t	*gl_clear;
cvar_t	*gl_cull;
cvar_t	*gl_polyblend;
cvar_t	*gl_flashblend;
//cvar_t	*gl_playermip;
//cvar_t  *gl_saturatelighting;
cvar_t	*gl_swapinterval;
cvar_t	*gl_texturemode;
cvar_t	*gl_texturealphamode;
cvar_t	*gl_texturesolidmode;
cvar_t	*gl_lockpvs;
cvar_t	*gl_jpg_quality;
cvar_t	*gl_coloredlightmaps;

//cvar_t	*gl_3dlabs_broken;

cvar_t	*vid_fullscreen;
cvar_t	*vid_gamma;
cvar_t	*vid_ref;
cvar_t	*vid_forcedrefresh;
cvar_t	*vid_optimalrefresh;
cvar_t	*vid_nowgl;
cvar_t	*vid_restore_on_switch;

cvar_t	*gl_texture_formats;
cvar_t	*gl_pic_formats;

cvar_t	*gl_dlight_falloff;
cvar_t	*gl_alphaskins;
cvar_t	*gl_defertext;

cvar_t	*gl_pic_scale;

//cvar_t	*con_alpha;

vec4_t	colorWhite = {1,1,1,1};

qboolean load_png_pics = true;
qboolean load_tga_pics = true;
qboolean load_jpg_pics = true;

qboolean load_png_wals = true;
qboolean load_tga_wals = true;
qboolean load_jpg_wals = true;

extern cvar_t		*gl_contrast;

/*
=================
R_CullBox

Returns true if the box is completely outside the frustom
=================
*/
qboolean R_CullBox (vec3_t mins, vec3_t maxs)
{
	int		i;

	if (FLOAT_NE_ZERO(r_nocull->value))
		return false;

	for (i=0 ; i<4 ; i++)
		if (BOX_ON_PLANE_SIDE(mins, maxs, &frustum[i]) == 2)
			return true;
	return false;
}


void R_RotateForEntity (entity_t *e)
{
    qglTranslatef (e->origin[0],  e->origin[1],  e->origin[2]);

    qglRotatef (e->angles[1],  0, 0, 1);
    qglRotatef (-e->angles[0],  0, 1, 0);
    qglRotatef (-e->angles[2],  1, 0, 0);
}

/*
=============================================================

  SPRITE MODELS

=============================================================
*/

/*
=================
R_DrawSpriteModel

=================
*/
void R_DrawSpriteModel (entity_t *e)
{
	float alpha = 1.0F;
	vec3_t	point;
	dsprframe_t	*frame;
	float		*up, *right;
	dsprite_t		*psprite;

	// don't even bother culling, because it's just a single
	// polygon without a surface cache

	psprite = (dsprite_t *)currentmodel->extradata;

#if 0
	if (e->frame < 0 || e->frame >= psprite->numframes)
	{
		ri.Con_Printf (PRINT_ALL, "no such sprite frame %i\n", e->frame);
		e->frame = 0;
	}
#endif
	e->frame %= psprite->numframes;

	frame = &psprite->frames[e->frame];

#if 0
	if (psprite->type == SPR_ORIENTED)
	{	// bullet marks on walls
	vec3_t		v_forward, v_right, v_up;

	AngleVectors (currententity->angles, v_forward, v_right, v_up);
		up = v_up;
		right = v_right;
	}
	else
#endif
	{	// normal sprite
		up = vup;
		right = vright;
	}

	if ( e->flags & RF_TRANSLUCENT )
		alpha = e->alpha;

	if ( alpha != 1.0F )
		qglEnable( GL_BLEND );

	qglColor4f( 1, 1, 1, alpha );

    GL_MBind(GL_TEXTURE0, currentmodel->skins[e->frame]->texnum);

	GL_TexEnv(GL_TEXTURE0, GL_MODULATE);

	if ( alpha == 1.0 )
		qglEnable (GL_ALPHA_TEST);
	else
		qglDisable( GL_ALPHA_TEST );

	qglBegin (GL_TRIANGLES);

	qglMTexCoord2f(GL_TEXTURE0, 0, 1);
	VectorMA (e->origin, -frame->origin_y, up, point);
	VectorMA (point, -frame->origin_x, right, point);
	qglVertex3f(point[0], point[1], point[2]);

	qglMTexCoord2f(GL_TEXTURE0, 0, 0);
	VectorMA (e->origin, frame->height - frame->origin_y, up, point);
	VectorMA (point, -frame->origin_x, right, point);
	qglVertex3f(point[0], point[1], point[2]);

	qglMTexCoord2f(GL_TEXTURE0, 1, 0);
	VectorMA (e->origin, frame->height - frame->origin_y, up, point);
	VectorMA (point, frame->width - frame->origin_x, right, point);
	qglVertex3f(point[0], point[1], point[2]);

	qglMTexCoord2f(GL_TEXTURE0, 0, 1);
	VectorMA (e->origin, -frame->origin_y, up, point);
	VectorMA (point, -frame->origin_x, right, point);
	qglVertex3f(point[0], point[1], point[2]);

	qglMTexCoord2f(GL_TEXTURE0, 1, 0);
	VectorMA (e->origin, frame->height - frame->origin_y, up, point);
	VectorMA (point, frame->width - frame->origin_x, right, point);
	qglVertex3f(point[0], point[1], point[2]);

	qglMTexCoord2f(GL_TEXTURE0, 1, 1);
	VectorMA (e->origin, -frame->origin_y, up, point);
	VectorMA (point, frame->width - frame->origin_x, right, point);
	qglVertex3f(point[0], point[1], point[2]);
	
	qglEnd ();

	qglDisable (GL_ALPHA_TEST);
	GL_TexEnv(GL_TEXTURE0, GL_REPLACE);

	if ( alpha != 1.0F )
		qglDisable( GL_BLEND );

	qglColor4f(colorWhite[0], colorWhite[1], colorWhite[2], colorWhite[3]);
}

//==================================================================================

/*
=============
R_DrawNullModel
=============
*/
void R_DrawNullModel (void)
{
	vec3_t	shadelight;
	int		i;

	if ( currententity->flags & RF_FULLBRIGHT )
		shadelight[0] = shadelight[1] = shadelight[2] = 1.0F;
	else
		R_LightPoint (currententity->origin, shadelight);

    qglPushMatrix ();
	R_RotateForEntity (currententity);

	qglDisable (GL_TEXTURE_2D);
	qglColor3f(shadelight[0], shadelight[1], shadelight[2]);

	qglBegin (GL_TRIANGLE_FAN);
	qglVertex3f (0, 0, -16);
	for (i=0 ; i<=4 ; i++)
		qglVertex3f (16*(float)cos(i*M_PI_DIV_2), 16*(float)sin(i*M_PI_DIV_2), 0);
	qglEnd ();

	qglBegin (GL_TRIANGLE_FAN);
	qglVertex3f (0, 0, 16);
	for (i=4 ; i>=0 ; i--)
		qglVertex3f (16*(float)cos(i*M_PI_DIV_2), 16*(float)sin(i*M_PI_DIV_2), 0);
	qglEnd ();

	qglColor3f (1,1,1);
	qglPopMatrix ();
	qglEnable (GL_TEXTURE_2D);
}

int visibleBits[MAX_ENTITIES];


/*
=============
R_DrawEntitiesOnList
=============
*/
void R_DrawEntitiesOnList (void)
{
	int		i;

	if (FLOAT_EQ_ZERO(r_drawentities->value))
		return;

	// draw non-transparent first
	for (i=0 ; i<r_newrefdef.num_entities ; i++)
	{
		currententity = &r_newrefdef.entities[i];

		if (currententity->flags & RF_TRANSLUCENT || (FLOAT_NE_ZERO(gl_alphaskins->value) && currententity->skin && currententity->skin->has_alpha))
			continue;	// solid

		if ( currententity->flags & RF_BEAM )
		{
			R_DrawBeam( currententity );
		}
		else
		{
			currentmodel = currententity->model;
			if (!currentmodel)
			{
				R_DrawNullModel ();
				continue;
			}

			switch (currentmodel->type)
			{
				case mod_alias:
					R_DrawAliasModel (currententity);
					break;
				case mod_brush:
					R_DrawBrushModel (currententity);
					break;
				case mod_sprite:
					R_DrawSpriteModel (currententity);
					break;
				default:
					ri.Sys_Error (ERR_DROP, "Bad modeltype %d on %s", currentmodel->type, currentmodel->name);
					break;
			}
		}
	}

	// draw transparent entities
	// we could sort these if it ever becomes a problem...
	qglDepthMask (0);		// no z writes
	for (i=0 ; i<r_newrefdef.num_entities ; i++)
	{
		currententity = &r_newrefdef.entities[i];
		if (!(currententity->flags & RF_TRANSLUCENT || (FLOAT_NE_ZERO(gl_alphaskins->value) && currententity->skin && currententity->skin->has_alpha)))
			continue;	// solid

		if ( currententity->flags & RF_BEAM )
		{
			R_DrawBeam( currententity );
		}
		else
		{
			currentmodel = currententity->model;

			if (!currentmodel)
			{
				R_DrawNullModel ();
				continue;
			}
			switch (currentmodel->type)
			{
			case mod_alias:
				R_DrawAliasModel (currententity);
				break;
			case mod_brush:
				R_DrawBrushModel (currententity);
				break;
			case mod_sprite:
				R_DrawSpriteModel (currententity);
				break;
			default:
				ri.Sys_Error (ERR_DROP, "Bad modeltype %d on %s", currentmodel->type, currentmodel->name);
				break;
			}
		}
	}
	qglDepthMask (1);		// back to writing

}

/*
** GL_DrawParticles
**
*/
void GL_DrawParticles( int num_particles, const particle_t particles[])
{
	const particle_t *p;
	int				i;
	vec3_t			up, right;
	float			scale;
	//byte			color[4];
	vec4_t			colorf;

    GL_MBind(GL_TEXTURE0, r_particletexture->texnum);
	qglDepthMask( GL_FALSE );		// no z buffering
	qglEnable( GL_BLEND );
	GL_TexEnv(GL_TEXTURE0, GL_MODULATE);
	qglBegin( GL_TRIANGLES );

	VectorScale (vup, 1.5f, up);
	VectorScale (vright, 1.5f, right);

	for ( p = particles, i=0 ; i < num_particles ; i++,p++)
	{
		// hack a scale up to keep particles from disapearing
		scale = ( p->origin[0] - r_origin[0] ) * vpn[0] + 
			    ( p->origin[1] - r_origin[1] ) * vpn[1] +
			    ( p->origin[2] - r_origin[2] ) * vpn[2];

		if (scale < 20)
			scale = 1;
		else
			scale = 1 + scale * 0.004f;

		//*(int *)color = colortable[p->color];
		//color[3] = (byte)Q_ftol(p->alpha*255);

		FastVectorCopy (d_8to24float[p->color], colorf);
		colorf[3] = p->alpha;

		qglColor4f(colorf[0], colorf[1], colorf[2], colorf[3]);

		qglMTexCoord2f( GL_TEXTURE0, 0.0625f, 0.0625f );
		qglVertex3f(p->origin[0], p->origin[1], p->origin[2]);

		qglMTexCoord2f( GL_TEXTURE0, 1.0625f, 0.0625f );
		qglVertex3f( p->origin[0] + up[0]*scale, 
			         p->origin[1] + up[1]*scale, 
					 p->origin[2] + up[2]*scale);

		qglMTexCoord2f( GL_TEXTURE0, 0.0625f, 1.0625f );
		qglVertex3f( p->origin[0] + right[0]*scale, 
			         p->origin[1] + right[1]*scale, 
					 p->origin[2] + right[2]*scale);
	}

	qglEnd ();
	qglDisable( GL_BLEND );
	qglColor4f(colorWhite[0], colorWhite[1], colorWhite[2], colorWhite[3]);
	qglDepthMask( 1 );		// back to normal Z buffering
	GL_TexEnv(GL_TEXTURE0, GL_REPLACE);
}

/*
===============
R_DrawParticles
===============
*/
void R_DrawParticles (void)
{
		GL_DrawParticles( r_newrefdef.num_particles, r_newrefdef.particles );
}

/*
============
R_PolyBlend
============
*/
void R_PolyBlend (void)
{
	if (FLOAT_EQ_ZERO(gl_polyblend->value))
		return;

	if (FLOAT_EQ_ZERO(v_blend[3]))
		return;

	qglDisable (GL_ALPHA_TEST);
	qglEnable (GL_BLEND);
	qglDisable (GL_DEPTH_TEST);
	qglDisable (GL_TEXTURE_2D);

    qglLoadIdentity ();

	// FIXME: get rid of these
    qglRotatef (-90,  1, 0, 0);	    // put Z going up
    qglRotatef (90,  0, 0, 1);	    // put Z going up

	qglColor4f(v_blend[0], v_blend[1], v_blend[2], v_blend[3]);

	qglBegin (GL_TRIANGLES);

	qglVertex3f (10, 100, 100);
	qglVertex3f (10, -100, 100);
	qglVertex3f (10, -100, -100);

	qglVertex3f (10, 100, 100);
	qglVertex3f (10, -100, -100);
	qglVertex3f (10, 100, -100);
	qglEnd ();

	qglDisable (GL_BLEND);
	qglEnable (GL_TEXTURE_2D);
	qglEnable (GL_ALPHA_TEST);

	qglColor4f(colorWhite[0], colorWhite[1], colorWhite[2], colorWhite[3]);
}

//=======================================================================

int SignbitsForPlane (cplane_t *out)
{
	int	bits, j;

	// for fast box on planeside test

	bits = 0;
	for (j=0 ; j<3 ; j++)
	{
		if (FLOAT_LT_ZERO(out->normal[j]))
			bits |= 1<<j;
	}
	return bits;
}


void R_SetFrustum (void)
{
	int		i;

#if 0
	/*
	** this code is wrong, since it presume a 90 degree FOV both in the
	** horizontal and vertical plane
	*/
	// front side is visible
	VectorAdd (vpn, vright, frustum[0].normal);
	VectorSubtract (vpn, vright, frustum[1].normal);
	VectorAdd (vpn, vup, frustum[2].normal);
	VectorSubtract (vpn, vup, frustum[3].normal);

	// we theoretically don't need to normalize these vectors, but I do it
	// anyway so that debugging is a little easier
	VectorNormalize( frustum[0].normal );
	VectorNormalize( frustum[1].normal );
	VectorNormalize( frustum[2].normal );
	VectorNormalize( frustum[3].normal );
#else
	// rotate VPN right by FOV_X/2 degrees
	RotatePointAroundVector( frustum[0].normal, vup, vpn, -(90-r_newrefdef.fov_x / 2 ) );
	// rotate VPN left by FOV_X/2 degrees
	RotatePointAroundVector( frustum[1].normal, vup, vpn, 90-r_newrefdef.fov_x / 2 );
	// rotate VPN up by FOV_X/2 degrees
	RotatePointAroundVector( frustum[2].normal, vright, vpn, 90-r_newrefdef.fov_y / 2 );
	// rotate VPN down by FOV_X/2 degrees
	RotatePointAroundVector( frustum[3].normal, vright, vpn, -( 90 - r_newrefdef.fov_y / 2 ) );
#endif

	for (i=0 ; i<4 ; i++)
	{
		frustum[i].type = PLANE_ANYZ;
		frustum[i].dist = DotProduct (r_origin, frustum[i].normal);
		frustum[i].signbits = SignbitsForPlane (&frustum[i]);
	}
}

//=======================================================================

/*
===============
R_SetupFrame
===============
*/
void R_SetupFrame (void)
{
	mleaf_t	*leaf;

	r_framecount++;

// build the transformation matrix for the given view angles
	FastVectorCopy (r_newrefdef.vieworg, r_origin);

	AngleVectors (r_newrefdef.viewangles, vpn, vright, vup);

// current viewcluster
	if ( !( r_newrefdef.rdflags & RDF_NOWORLDMODEL ) )
	{
		r_oldviewcluster = r_viewcluster;
		r_oldviewcluster2 = r_viewcluster2;
		leaf = Mod_PointInLeaf (r_origin, r_worldmodel);
		r_viewcluster = r_viewcluster2 = leaf->cluster;

		// check above and below so crossing solid water doesn't draw wrong
		if (!leaf->contents)
		{	// look down a bit
			vec3_t	temp;

			FastVectorCopy (r_origin, temp);
			temp[2] -= 16;
			leaf = Mod_PointInLeaf (temp, r_worldmodel);
			if ( !(leaf->contents & CONTENTS_SOLID) &&
				(leaf->cluster != r_viewcluster2) )
				r_viewcluster2 = leaf->cluster;
		}
		else
		{	// look up a bit
			vec3_t	temp;

			FastVectorCopy (r_origin, temp);
			temp[2] += 16;
			leaf = Mod_PointInLeaf (temp, r_worldmodel);
			if ( !(leaf->contents & CONTENTS_SOLID) &&
				(leaf->cluster != r_viewcluster2) )
				r_viewcluster2 = leaf->cluster;
		}
	}

	v_blend[0] = r_newrefdef.blend[0];
	v_blend[1] = r_newrefdef.blend[1];
	v_blend[2] = r_newrefdef.blend[2];
	v_blend[3] = r_newrefdef.blend[3];

	c_brush_polys = 0;
	c_alias_polys = 0;

	// clear out the portion of the screen that the NOWORLDMODEL defines
	/*if ( r_newrefdef.rdflags & RDF_NOWORLDMODEL )
	{
		qglEnable( GL_SCISSOR_TEST );
		qglClearColor( 0.3f, 0.3f, 0.3f, 1 );
		
		qglScissor( r_newrefdef.x, vid.height - r_newrefdef.height - r_newrefdef.y, r_newrefdef.width, r_newrefdef.height );
		qglClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
		
		qglClearColor( 1, 0, 0.5f, 0.5f );

		qglDisable( GL_SCISSOR_TEST );
	}*/
	if (r_newrefdef.rdflags & RDF_NOWORLDMODEL)
	{
		qglEnable(GL_SCISSOR_TEST);
		qglClearColor(0.3f, 0.3f, 0.3f, 1);
		qglScissor(r_newrefdef.x, viddef.height - r_newrefdef.height - r_newrefdef.y, r_newrefdef.width,
			   r_newrefdef.height);
		qglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		qglClearColor(0, 0, 0, 1);
		qglDisable(GL_SCISSOR_TEST);
	}
}


void MYgluPerspective( GLdouble fovy, GLdouble aspect,
		     GLdouble zNear, GLdouble zFar )
{
   GLdouble xmin, xmax, ymin, ymax;

   ymax = zNear * tan( fovy * M_PI / 360.0 );
   ymin = -ymax;

   xmin = ymin * aspect;
   xmax = ymax * aspect;

#ifdef STERO_SUPPORT
   xmin += -( 2 * gl_state.camera_separation ) / zNear;
   xmax += -( 2 * gl_state.camera_separation ) / zNear;
#endif

   qglFrustum( xmin, xmax, ymin, ymax, zNear, zFar );
}


/*
=============
R_SetupGL
=============
*/
void R_SetupGL (void)
{
	float	screenaspect;
//	float	yfov;
	int		x, x2, y2, y, w, h;

	//
	// set up viewport
	//
	x = (int)floor(r_newrefdef.x * viddef.width / viddef.width);
	x2 = (int)ceil((r_newrefdef.x + r_newrefdef.width) * viddef.width / viddef.width);
	y = (int)floor(viddef.height - r_newrefdef.y * viddef.height / viddef.height);
	y2 = (int)ceil(viddef.height - (r_newrefdef.y + r_newrefdef.height) * viddef.height / viddef.height);

	w = x2 - x;
	h = y - y2;

	qglViewport (x, y2, w, h);

	//
	// set up projection matrix
	//
    screenaspect = (float)r_newrefdef.width/r_newrefdef.height;
//	yfov = 2*atan((float)r_newrefdef.height/r_newrefdef.width)*180/M_PI;
	qglMatrixMode(GL_PROJECTION);
    qglLoadIdentity ();
    MYgluPerspective (r_newrefdef.fov_y,  screenaspect,  4,  gl_zfar->value);

	qglCullFace(GL_FRONT);

	qglMatrixMode(GL_MODELVIEW);
    qglLoadIdentity ();

    qglRotatef (-90,  1, 0, 0);	    // put Z going up
    qglRotatef (90,  0, 0, 1);	    // put Z going up
    qglRotatef (-r_newrefdef.viewangles[2],  1, 0, 0);
    qglRotatef (-r_newrefdef.viewangles[0],  0, 1, 0);
    qglRotatef (-r_newrefdef.viewangles[1],  0, 0, 1);
    qglTranslatef (-r_newrefdef.vieworg[0],  -r_newrefdef.vieworg[1],  -r_newrefdef.vieworg[2]);

//	if ( gl_state.camera_separation != 0 && gl_state.stereo_enabled )
//		qglTranslatef ( gl_state.camera_separation, 0, 0 );

	qglGetFloatv (GL_MODELVIEW_MATRIX, r_world_matrix);

	//
	// set drawing parms
	//
	if (FLOAT_NE_ZERO(gl_cull->value))
		qglEnable(GL_CULL_FACE);
	else
		qglDisable(GL_CULL_FACE);

	//qglDisable(GL_BLEND);
	qglDisable(GL_ALPHA_TEST);
	//qglEnable(GL_ALPHA_TEST);
	qglEnable(GL_DEPTH_TEST);
}

/*
=============
R_Clear
=============
*/

float ref_frand(void)
{
	return (((rand()&32767)) * .0000305185094759971922971282082583086642f);
}

void R_Clear (void)
{
	gldepthmin = 0;
	gldepthmax = 1;
	qglDepthFunc (GL_LEQUAL);

	qglDepthRange (gldepthmin, gldepthmax);

	if (FLOAT_NE_ZERO(gl_clear->value) && gl_clear->value == 2)
	{
		qglClearColor (ref_frand(), ref_frand(), ref_frand(), 1.0);
	} else {
		qglClearColor(0.0f, 0.0f, 0.0f, 1.0);
	}

	qglClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

/*void R_Flash( void )
{
	R_PolyBlend ();
}*/

/*
================
R_RenderView

r_newrefdef must be set before the first call
================
*/
void R_RenderView (refdef_t *fd)
{
	if (FLOAT_NE_ZERO(r_norefresh->value))
		return;

	r_newrefdef = *fd;

	if (FLOAT_NE_ZERO(gl_hudscale->value))
	{
		r_newrefdef.width = (int)(r_newrefdef.width * gl_hudscale->value);
		r_newrefdef.height = (int)(r_newrefdef.height * gl_hudscale->value);
		r_newrefdef.x = (int)(r_newrefdef.x * gl_hudscale->value);
		r_newrefdef.y = (int)(r_newrefdef.y * gl_hudscale->value);
	}

	if (!r_worldmodel && !( r_newrefdef.rdflags & RDF_NOWORLDMODEL ) )
		ri.Sys_Error (ERR_DROP, "R_RenderView: NULL worldmodel");

	//if (FLOAT_NE_ZERO(r_speeds->value))
	//{
	c_brush_polys = 0;
	c_alias_polys = 0;
	//}

	R_PushDlights ();

	R_SetupFrame ();

	R_SetFrustum ();

	R_SetupGL ();

	R_MarkLeaves ();	// done here so we know if we're in water

	R_DrawWorld ();

	R_DrawEntitiesOnList ();

	R_RenderDlights ();

	R_DrawParticles ();

	R_DrawAlphaSurfaces ();

	R_PolyBlend();
	
	if (FLOAT_NE_ZERO(r_speeds->value))
	{
		ri.Con_Printf (PRINT_ALL, "%4i wpoly %4i epoly %i tex %i lmaps\n",
			c_brush_polys, 
			c_alias_polys, 
			c_visible_textures, 
			c_visible_lightmaps); 
	}
}


void	R_SetGL2D (void)
{
	// set 2D virtual screen size
	qglViewport (0,0, viddef.width, viddef.height);
	qglMatrixMode(GL_PROJECTION);
    qglLoadIdentity ();
	//qglOrtho  (0, vid.width, vid.height, 0, -99999, 99999);
	qglOrtho(0, vid_scaled_width, vid_scaled_height, 0, -99999, 99999);
	qglMatrixMode(GL_MODELVIEW);
    qglLoadIdentity ();
	qglDisable (GL_DEPTH_TEST);
	qglDisable (GL_CULL_FACE);
	//GLPROFqglDisable (GL_BLEND);
	qglEnable (GL_ALPHA_TEST);
	qglColor4f(colorWhite[0], colorWhite[1], colorWhite[2], colorWhite[3]);
}



/*
====================
R_SetLightLevel

====================
*/
void R_SetLightLevel (void)
{
	vec3_t		shadelight;

	if (r_newrefdef.rdflags & RDF_NOWORLDMODEL)
		return;

	// save off light value for server to look at (BIG HACK!)

	R_LightPoint (r_newrefdef.vieworg, shadelight);

	// pick the greatest component, which should be the same
	// as the mono value returned by software
	if (shadelight[0] > shadelight[1])
	{
		if (shadelight[0] > shadelight[2])
			r_lightlevel->value = 150*shadelight[0];
		else
			r_lightlevel->value = 150*shadelight[2];
	}
	else
	{
		if (shadelight[1] > shadelight[2])
			r_lightlevel->value = 150*shadelight[1];
		else
			r_lightlevel->value = 150*shadelight[2];
	}

}

/*
@@@@@@@@@@@@@@@@@@@@@
R_RenderFrame

@@@@@@@@@@@@@@@@@@@@@
*/
void R_RenderFrame (refdef_t *fd)
{
	R_RenderView( fd );
	R_SetLightLevel ();
	R_SetGL2D ();
}

void Cmd_HashStats_f (void);
void R_Register( void )
{
	r_lefthand = ri.Cvar_Get( "hand", "0", CVAR_USERINFO | CVAR_ARCHIVE );
	r_norefresh = ri.Cvar_Get ("r_norefresh", "0", 0);
	r_fullbright = ri.Cvar_Get ("r_fullbright", "0", 0);
	r_drawentities = ri.Cvar_Get ("r_drawentities", "1", 0);
	r_drawworld = ri.Cvar_Get ("r_drawworld", "1", 0);
	r_novis = ri.Cvar_Get ("r_novis", "0", 0);
	r_nocull = ri.Cvar_Get ("r_nocull", "0", 0);
	r_lerpmodels = ri.Cvar_Get ("r_lerpmodels", "1", 0);
	r_speeds = ri.Cvar_Get ("r_speeds", "0", 0);

	r_lightlevel = ri.Cvar_Get ("r_lightlevel", "0", CVAR_NOSET);

	//gl_nosubimage = ri.Cvar_Get( "gl_nosubimage", "0", 0 );
	gl_allow_software = ri.Cvar_Get( "gl_allow_software", "0", 0 );

	gl_particle_min_size = ri.Cvar_Get( "gl_particle_min_size", "2", CVAR_ARCHIVE );
	gl_particle_max_size = ri.Cvar_Get( "gl_particle_max_size", "40", CVAR_ARCHIVE );
	gl_particle_size = ri.Cvar_Get( "gl_particle_size", "40", CVAR_ARCHIVE );
	gl_particle_att_a = ri.Cvar_Get( "gl_particle_att_a", "0.01", CVAR_ARCHIVE );
	gl_particle_att_b = ri.Cvar_Get( "gl_particle_att_b", "0.0", CVAR_ARCHIVE );
	gl_particle_att_c = ri.Cvar_Get( "gl_particle_att_c", "0.01", CVAR_ARCHIVE );

	gl_modulate = ri.Cvar_Get ("gl_modulate", "2", CVAR_ARCHIVE );
	gl_bitdepth = ri.Cvar_Get( "gl_bitdepth", "0", 0 );
	gl_mode = ri.Cvar_Get( "gl_mode", "3", CVAR_ARCHIVE );
	//gl_lightmap = ri.Cvar_Get ("gl_lightmap", "0", 0);
	gl_shadows = ri.Cvar_Get ("gl_shadows", "0", CVAR_ARCHIVE );
	gl_dynamic = ri.Cvar_Get ("gl_dynamic", "1", 0);
	gl_nobind = ri.Cvar_Get ("gl_nobind", "0", 0);
	gl_round_down = ri.Cvar_Get ("gl_round_down", "0", 0);
	gl_picmip = ri.Cvar_Get ("gl_picmip", "0", 0);
	gl_skymip = ri.Cvar_Get ("gl_skymip", "0", 0);
	gl_showtris = ri.Cvar_Get ("gl_showtris", "0", 0);
	gl_clear = ri.Cvar_Get ("gl_clear", "0", 0);
	gl_cull = ri.Cvar_Get ("gl_cull", "1", 0);
	gl_polyblend = ri.Cvar_Get ("gl_polyblend", "1", 0);
	gl_flashblend = ri.Cvar_Get ("gl_flashblend", "0", 0);
	//gl_playermip = ri.Cvar_Get ("gl_playermip", "0", 0);
	//gl_monolightmap = ri.Cvar_Get( "gl_monolightmap", "0", 0 );
	gl_texturemode = ri.Cvar_Get( "gl_texturemode", "GL_LINEAR_MIPMAP_LINEAR", CVAR_ARCHIVE );
	gl_texturealphamode = ri.Cvar_Get( "gl_texturealphamode", "default", CVAR_ARCHIVE );
	gl_texturesolidmode = ri.Cvar_Get( "gl_texturesolidmode", "default", CVAR_ARCHIVE );
	gl_lockpvs = ri.Cvar_Get( "gl_lockpvs", "0", 0 );

	//gl_ext_swapinterval = ri.Cvar_Get( "gl_ext_swapinterval", "1", CVAR_ARCHIVE );
	gl_ext_multitexture = ri.Cvar_Get( "gl_ext_multitexture", "1", CVAR_ARCHIVE );
	
	//note, pointparams moved to init to handle defaults
	//gl_ext_compiled_vertex_array = ri.Cvar_Get( "gl_ext_compiled_vertex_array", "1", CVAR_ARCHIVE );

	//r1ch: my extensions
	//gl_ext_generate_mipmap = ri.Cvar_Get ("gl_ext_generate_mipmap", "0", 0);
	gl_ext_point_sprite = ri.Cvar_Get ("gl_ext_point_sprite", "0", 0);
	gl_ext_texture_filter_anisotropic = ri.Cvar_Get ("gl_ext_texture_filter_anisotropic", "0", 0);
	gl_ext_texture_non_power_of_two = ri.Cvar_Get ("gl_ext_texture_non_power_of_two", "0", 0);
	gl_ext_max_anisotropy = ri.Cvar_Get ("gl_ext_max_anisotropy", "2", 0);
	
	gl_ext_nv_multisample_filter_hint = ri.Cvar_Get ("gl_ext_nv_multisample_filter_hint", "fastest", 0);

	gl_colorbits = ri.Cvar_Get ("gl_colorbits", "0", 0);
	gl_stencilbits = ri.Cvar_Get ("gl_stencilbits", "", 0);
	gl_alphabits = ri.Cvar_Get ("gl_alphabits", "", 0);
	gl_depthbits = ri.Cvar_Get ("gl_depthbits", "", 0);

	gl_ext_multisample = ri.Cvar_Get ("gl_ext_multisample", "0", 0);
	gl_ext_samples = ri.Cvar_Get ("gl_ext_samples", "2", 0);
	
	gl_zfar = ri.Cvar_Get ("gl_zfar", "8192", 0);
	gl_hudscale = ri.Cvar_Get ("gl_hudscale", "1", 0);

	cl_version = ri.Cvar_Get ("cl_version", REF_VERSION, CVAR_NOSET); 
	
	gl_r1gl_test = ri.Cvar_Get ("gl_r1gl_test", "0", 0);
	gl_doublelight_entities = ri.Cvar_Get ("gl_doublelight_entities", "1", 0);
	gl_noscrap = ri.Cvar_Get ("gl_noscrap", "1", 0);
	gl_overbrights = ri.Cvar_Get ("gl_overbrights", "0", 0);
	gl_linear_mipmaps = ri.Cvar_Get ("gl_linear_mipmaps", "0", 0);

	vid_forcedrefresh = ri.Cvar_Get ("vid_forcedrefresh", "0", 0);
	vid_optimalrefresh = ri.Cvar_Get ("vid_optimalrefresh", "0", 0);
	vid_gamma_pics = ri.Cvar_Get ("vid_gamma_pics", "0", 0);
	vid_nowgl = ri.Cvar_Get ("vid_nowgl", "0", 0);
	vid_restore_on_switch = ri.Cvar_Get ("vid_flip_on_switch", "0", 0);

	gl_forcewidth = ri.Cvar_Get ("vid_forcewidth", "0", 0);
	gl_forceheight = ri.Cvar_Get ("vid_forceheight", "0", 0);

	vid_topmost = ri.Cvar_Get ("vid_topmost", "0", 0);

	gl_pic_scale = ri.Cvar_Get ("gl_pic_scale", "1", 0);
	//r1ch end my shit

	gl_swapinterval = ri.Cvar_Get( "gl_swapinterval", "1", CVAR_ARCHIVE );

	//gl_saturatelighting = ri.Cvar_Get( "gl_saturatelighting", "0", 0 );

	gl_jpg_quality = ri.Cvar_Get ("gl_jpg_quality", "90", 0);
	gl_coloredlightmaps = ri.Cvar_Get ("gl_coloredlightmaps", "1", 0);
	usingmodifiedlightmaps = (gl_coloredlightmaps->value != 1.0f);

	//gl_3dlabs_broken = ri.Cvar_Get( "gl_3dlabs_broken", "1", CVAR_ARCHIVE );

	vid_fullscreen = ri.Cvar_Get( "vid_fullscreen", "0", CVAR_ARCHIVE );
	vid_gamma = ri.Cvar_Get( "vid_gamma", "1.0", CVAR_ARCHIVE );
	vid_ref = ri.Cvar_Get( "vid_ref", "r1gl", CVAR_ARCHIVE );

	gl_texture_formats = ri.Cvar_Get ("gl_texture_formats", "png jpg tga", 0);
	gl_pic_formats = ri.Cvar_Get ("gl_pic_formats", "png jpg tga", 0);

	load_png_wals = strstr (gl_texture_formats->string, "png") ? true : false;
	load_jpg_wals = strstr (gl_texture_formats->string, "jpg") ? true : false;
	load_tga_wals = strstr (gl_texture_formats->string, "tga") ? true : false;

	load_png_pics = strstr (gl_pic_formats->string, "png") ? true : false;
	load_jpg_pics = strstr (gl_pic_formats->string, "jpg") ? true : false;
	load_tga_pics = strstr (gl_pic_formats->string, "tga") ? true : false;

	gl_dlight_falloff = ri.Cvar_Get ("gl_dlight_falloff", "0", 0);
	gl_alphaskins = ri.Cvar_Get ("gl_alphaskins", "0", 0);
	gl_defertext = ri.Cvar_Get ("gl_defertext", "0", 0);
	defer_drawing = (int)gl_defertext->value;

	//con_alpha = ri.Cvar_Get ("con_alpha", "1.0", 0);

	ri.Cmd_AddCommand( "imagelist", GL_ImageList_f );
	ri.Cmd_AddCommand( "screenshot", GL_ScreenShot_f );
	ri.Cmd_AddCommand( "modellist", Mod_Modellist_f );
	ri.Cmd_AddCommand( "gl_strings", GL_Strings_f );
	ri.Cmd_AddCommand( "hash_stats", Cmd_HashStats_f );
	

#ifdef R1GL_RELEASE
	ri.Cmd_AddCommand ("r1gl_version", GL_Version_f);
#endif
}

/*
==================
R_SetMode
==================
*/
int R_SetMode (void)
{
	int err;
	qboolean fullscreen;

	fullscreen = FLOAT_EQ_ZERO(vid_fullscreen->value) ? false : true;

	vid_fullscreen->modified = false;
	gl_mode->modified = false;

	if (FLOAT_NE_ZERO(gl_forcewidth->value))
		viddef.width = (int)gl_forcewidth->value;

	if (FLOAT_NE_ZERO(gl_forceheight->value))
		viddef.height = (int)gl_forceheight->value;

	if ( ( err = GLimp_SetMode( &viddef.width, &viddef.height, Q_ftol(gl_mode->value), fullscreen ) ) == VID_ERR_NONE )
	{
		gl_state.prev_mode = Q_ftol(gl_mode->value);
	}
	else
	{
		if ( err & VID_ERR_RETRY_QGL)
		{
			return err;
		}
		else if ( err & VID_ERR_FULLSCREEN_FAILED )
		{
			ri.Cvar_SetValue( "vid_fullscreen", 0);
			vid_fullscreen->modified = false;
			ri.Con_Printf( PRINT_ALL, "ref_gl::R_SetMode() - fullscreen unavailable in this mode\n" );
			if ( ( err = GLimp_SetMode( &viddef.width, &viddef.height, Q_ftol(gl_mode->value), false ) ) == VID_ERR_NONE )
				return VID_ERR_NONE;
		}
		else if ( err & VID_ERR_FAIL )
		{
			ri.Cvar_SetValue( "gl_mode", (float)gl_state.prev_mode );
			gl_mode->modified = false;
			ri.Con_Printf( PRINT_ALL, "ref_gl::R_SetMode() - invalid mode\n" );
		}

		// try setting it back to something safe
		if ( ( err = GLimp_SetMode( &viddef.width, &viddef.height, gl_state.prev_mode, false ) ) != VID_ERR_NONE )
		{
			ri.Con_Printf( PRINT_ALL, "ref_gl::R_SetMode() - could not revert to safe mode\n" );
			return VID_ERR_FAIL;
		}
	}
	return VID_ERR_NONE;
}

/*
===============
R_Init
===============
*/
int R_Init( void *hinstance, void *hWnd )
{	
	char renderer_buffer[1000];
	char vendor_buffer[1000];
	int		err;
	int		j;
	extern float r_turbsin[256];

	clearImageHash();

	memset(gl_state.lightmap_textures, 0, MAX_LIGHTMAPS * sizeof(GLuint));

	for ( j = 0; j < 256; j++ )
	{
		r_turbsin[j] *= 0.5;
	}

	ri.Cmd_ExecuteText (EXEC_NOW, "exec r1gl.cfg\n");

	ri.Con_Printf (PRINT_ALL, "ref_gl version: "REF_VERSION"\n");

	ri.Con_Printf (PRINT_DEVELOPER, "Draw_GetPalette()\n");
	Draw_GetPalette ();

	ri.Con_Printf (PRINT_DEVELOPER, "R_Register()\n");
	R_Register();

	gl_overbrights->modified = false;

retryQGL:

	// initialize our QGL dynamic bindings
	ri.Con_Printf (PRINT_DEVELOPER, "QGL_Init()\n");
	if ( !QGL_Init( "" ) )
	{
		QGL_Shutdown();

		ri.Con_Printf (PRINT_ALL, "ref_gl::R_Init(): QGL_Init() failed\n");

		return -1;
	}

	// initialize OS-specific parts of OpenGL
	ri.Con_Printf (PRINT_DEVELOPER, "GLimp_Init()\n");
	if ( !GLimp_Init( hinstance, hWnd ) )
	{
		ri.Con_Printf (PRINT_ALL, "ref_gl::R_Init(): GLimp_Init() failed\n");
		QGL_Shutdown();
		return -1;
	}

	// set our "safe" modes
	gl_state.prev_mode = 3;

	// create the window and set up the context
	ri.Con_Printf (PRINT_DEVELOPER, "R_SetMode()\n");
	err = R_SetMode ();
	if (err != VID_ERR_NONE)
	{
		QGL_Shutdown();
		if (err & VID_ERR_RETRY_QGL)
			goto retryQGL;

        ri.Con_Printf (PRINT_ALL, "ref_gl::R_Init() - could not R_SetMode()\n" );
		return -1;
	}

	ri.Con_Printf (PRINT_DEVELOPER, "Vid_MenuInit()\n");
	ri.Vid_MenuInit();

	/*
	** get our various GL strings
	*/
	gl_config.vendor_string = (const char *) qglGetString (GL_VENDOR);
	ri.Con_Printf (PRINT_ALL, "GL_VENDOR: %s\n", gl_config.vendor_string );
	gl_config.renderer_string = (const char *) qglGetString (GL_RENDERER);
	ri.Con_Printf (PRINT_ALL, "GL_RENDERER: %s\n", gl_config.renderer_string );
	gl_config.version_string = (const char *) qglGetString (GL_VERSION);
	ri.Con_Printf (PRINT_ALL, "GL_VERSION: %s\n", gl_config.version_string );
	gl_config.extensions_string = (const char *) qglGetString (GL_EXTENSIONS);
	//ri.Con_Printf (PRINT_ALL, "GL_EXTENSIONS: %s\n", gl_config.extensions_string );

	Q_strncpy( renderer_buffer, gl_config.renderer_string, sizeof(renderer_buffer)-1);
	Q_strlwr( renderer_buffer );

	Q_strncpy( vendor_buffer, gl_config.vendor_string, sizeof(vendor_buffer)-1);
	Q_strlwr( vendor_buffer );

	/*
	** grab extensions
	*/

#ifdef _WIN32
	if ( strstr( gl_config.extensions_string, "WGL_EXT_swap_control" ) )
	{
		//qwglSwapIntervalEXT = ( BOOL (WINAPI *)(int)) qwglGetProcAddress( "wglSwapIntervalEXT" );
		ri.Con_Printf( PRINT_ALL, "...enabling WGL_EXT_swap_control\n" );
	}
	else
	{
		ri.Con_Printf( PRINT_ALL, "...WGL_EXT_swap_control not found\n" );
	}
#endif

	ri.Con_Printf( PRINT_ALL, "Initializing r1gl extensions:\n" );

	/*gl_config.r1gl_GL_SGIS_generate_mipmap = false;
	if ( strstr( gl_config.extensions_string, "GL_SGIS_generate_mipmap" ) ) {
		if ( gl_ext_generate_mipmap->value ) {
			ri.Con_Printf( PRINT_ALL, "...using GL_SGIS_generate_mipmap\n" );
			gl_config.r1gl_GL_SGIS_generate_mipmap = true;
		} else {
			ri.Con_Printf( PRINT_ALL, "...ignoring GL_SGIS_generate_mipmap\n" );		
		}
	} else {
		ri.Con_Printf( PRINT_ALL, "...GL_SGIS_generate_mipmap not found\n" );
	}*/

	gl_config.r1gl_GL_EXT_texture_filter_anisotropic = false;
	if ( strstr( gl_config.extensions_string, "GL_EXT_texture_filter_anisotropic" ) )
	{
		if ( gl_ext_texture_filter_anisotropic->value ) {
			ri.Con_Printf( PRINT_ALL, "...using GL_EXT_texture_filter_anisotropic\n" );
			gl_config.r1gl_GL_EXT_texture_filter_anisotropic = true;
		} else {
			ri.Con_Printf( PRINT_ALL, "...ignoring GL_EXT_texture_filter_anisotropic\n" );		
		}
	} else {
		ri.Con_Printf( PRINT_ALL, "...GL_EXT_texture_filter_anisotropic not found\n" );
	}
	gl_ext_texture_filter_anisotropic->modified = false;

	gl_config.r1gl_GL_ARB_texture_non_power_of_two = false;
	if ( strstr( gl_config.extensions_string, "GL_ARB_texture_non_power_of_two" ) ) {
		if (FLOAT_NE_ZERO (gl_ext_texture_non_power_of_two->value) ) {
			ri.Con_Printf( PRINT_ALL, "...using GL_ARB_texture_non_power_of_two\n" );
			gl_config.r1gl_GL_ARB_texture_non_power_of_two = true;
		} else {
			ri.Con_Printf( PRINT_ALL, "...ignoring GL_ARB_texture_non_power_of_two\n" );		
		}
	} else {
		ri.Con_Printf( PRINT_ALL, "...GL_ARB_texture_non_power_of_two not found\n" );
	}

	ri.Con_Printf( PRINT_ALL, "Initializing r1gl NVIDIA-only extensions:\n" );
	gl_config.r1gl_GL_EXT_nv_multisample_filter_hint = false;
	if ( strstr( gl_config.extensions_string, "GL_NV_multisample_filter_hint" ) ) {
		gl_config.r1gl_GL_EXT_nv_multisample_filter_hint = true;	
		ri.Con_Printf( PRINT_ALL, "...allowing GL_NV_multisample_filter_hint\n" );
	} else {
		ri.Con_Printf( PRINT_ALL, "...GL_NV_multisample_filter_hint not found\n" );
	}

	ri.Con_Printf( PRINT_DEVELOPER, "GL_SetDefaultState()\n" );
	GL_SetDefaultState();

	//r1: setup cached screensizes
	vid_scaled_width = viddef.width / gl_hudscale->value;
	vid_scaled_height = viddef.height / gl_hudscale->value;

	/*
	** draw our stereo patterns
	*/
#if 0 // commented out until H3D pays us the money they owe us
	GL_DrawStereoPattern();
#endif

	ri.Con_Printf( PRINT_DEVELOPER, "GL_InitImages()\n" );
	GL_InitImages ();

	ri.Con_Printf( PRINT_DEVELOPER, "Mod_Init()\n" );
	Mod_Init ();

	ri.Con_Printf( PRINT_DEVELOPER, "R_InitParticleTexture()\n" );
	R_InitParticleTexture ();

	ri.Con_Printf( PRINT_DEVELOPER, "Draw_InitLocal()\n" );
	Draw_InitLocal ();

	err = qglGetError();
	if ( err != GL_NO_ERROR )
		ri.Con_Printf (PRINT_ALL, "glGetError() = 0x%x\n", err);

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);
	glEnableVertexAttribArray(3);

	ri.Con_Printf( PRINT_DEVELOPER, "R_Init() complete.\n" );
	return 0;
}

/*
===============
R_Shutdown
===============
*/
void R_Shutdown (void)
{
	if (gl_state.lightmap_textures[0] != 0) {
		glDeleteTextures(MAX_LIGHTMAPS, gl_state.lightmap_textures);
		memset(gl_state.lightmap_textures, 0, MAX_LIGHTMAPS * sizeof(GLuint));
	}

	ri.Cmd_RemoveCommand ("modellist");
	ri.Cmd_RemoveCommand ("screenshot");
	ri.Cmd_RemoveCommand ("imagelist");
	ri.Cmd_RemoveCommand ("gl_strings");
	ri.Cmd_RemoveCommand ("hash_stats");

#ifdef R1GL_RELEASE
	ri.Cmd_RemoveCommand ("r1gl_version");
#endif

	Mod_FreeAll ();

	GL_ShutdownImages ();

	/*
	** shutdown our QGL subsystem
	*/
	QGL_Shutdown();

	/*
	** shut down OS specific OpenGL stuff like contexts, etc.
	*/
	GLimp_Shutdown();
}

void GL_UpdateAnisotropy (void)
{
	int		i;
	image_t	*glt;
	float	value;

	if (!gl_config.r1gl_GL_EXT_texture_filter_anisotropic)
		value = 1;
	else
		value = gl_ext_max_anisotropy->value;

	for (i=0, glt=gltextures ; i<numgltextures ; i++, glt++)
	{
		if (glt->type != it_pic && glt->type != it_sky)
		{
			GL_MBind(GL_TEXTURE0, glt->texnum);
			qglTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, value);
		}
	}
}


/*
@@@@@@@@@@@@@@@@@@@@@
R_BeginFrame
@@@@@@@@@@@@@@@@@@@@@
*/
void R_BeginFrame( float camera_separation )
{

	/*
	** change modes if necessary
	*/
	if ( gl_mode->modified || vid_fullscreen->modified )
	{	// FIXME: only restart if CDS is required
		cvar_t	*ref;

		ref = ri.Cvar_Get ("vid_ref", "r1gl", 0);
		ref->modified = true;
	}

	if (gl_ext_nv_multisample_filter_hint->modified)
	{
		gl_ext_nv_multisample_filter_hint->modified = false;

		if (gl_config.r1gl_GL_EXT_nv_multisample_filter_hint)
		{
			if (!strcmp (gl_ext_nv_multisample_filter_hint->string, "nicest"))
				qglHint(GL_MULTISAMPLE_FILTER_HINT_NV, GL_NICEST);
			else
				qglHint(GL_MULTISAMPLE_FILTER_HINT_NV, GL_FASTEST);
		}
	}

	if (gl_contrast->modified)
	{
		if (gl_contrast->value < 0.5f)
			ri.Cvar_SetValue ("gl_contrast", 0.5f);
		else if (gl_contrast->value > 1.5f)
			ri.Cvar_SetValue ("gl_contrast", 1.5f);

		gl_contrast->modified = false;
	}



	/*
	** update 3Dfx gamma -- it is expected that a user will do a vid_restart
	** after tweaking this value
	*/
#if 0
	if ( vid_gamma->modified )
	{
		vid_gamma->modified = false;

		if ( gl_config.renderer & ( GL_RENDERER_VOODOO ) )
		{
			char envbuffer[1024];
			float g;

			g = 2.00 * ( 0.8 - ( vid_gamma->value - 0.5 ) ) + 1.0F;
			Com_sprintf( envbuffer, sizeof(envbuffer), "SSTV2_GAMMA=%f", g );
			putenv( envbuffer );
			Com_sprintf( envbuffer, sizeof(envbuffer), "SST_GAMMA=%f", g );
			putenv( envbuffer );
		}
	}
#endif

	GLimp_BeginFrame ();

	/*
	** go into 2D mode
	*/
	qglViewport (0,0, viddef.width, viddef.height);
	qglMatrixMode(GL_PROJECTION);
    qglLoadIdentity ();
	//qglOrtho  (0, vid.width, vid.height, 0, -99999, 99999);
	qglOrtho(0, vid_scaled_width, vid_scaled_height, 0, -99999, 99999);
	qglMatrixMode(GL_MODELVIEW);
    qglLoadIdentity ();
	//qglDisable (GL_DEPTH_TEST);
	//GLPROFqglDisable (GL_CULL_FACE);
	//GLPROFqglDisable (GL_BLEND);
	//GLPROFqglEnable (GL_ALPHA_TEST);
	qglColor4f(colorWhite[0], colorWhite[1], colorWhite[2], colorWhite[3]);

	//qglEnable(GL_MULTISAMPLE_ARB);

	/*
	** texturemode stuff
	*/
	if ( gl_texturemode->modified )
	{
		GL_TextureMode( gl_texturemode->string );
		gl_texturemode->modified = false;
	}

	if (gl_ext_max_anisotropy->modified && gl_config.r1gl_GL_EXT_texture_filter_anisotropic)
	{
		GL_UpdateAnisotropy ();
		gl_ext_max_anisotropy->modified = false;
	}

	if (gl_ext_texture_filter_anisotropic->modified)
	{
		gl_config.r1gl_GL_EXT_texture_filter_anisotropic = false;
		if ( strstr( gl_config.extensions_string, "GL_EXT_texture_filter_anisotropic" ) )
		{
			if ( gl_ext_texture_filter_anisotropic->value )
			{
				ri.Con_Printf( PRINT_ALL, "...using GL_EXT_texture_filter_anisotropic\n" );
				gl_config.r1gl_GL_EXT_texture_filter_anisotropic = true;
				GL_UpdateAnisotropy ();
			}
			else
			{
				ri.Con_Printf( PRINT_ALL, "...ignoring GL_EXT_texture_filter_anisotropic\n" );
				GL_UpdateAnisotropy ();
			}
		}
		else
		{
			ri.Con_Printf( PRINT_ALL, "...GL_EXT_texture_filter_anisotropic not found\n" );
		}
		
		gl_ext_texture_filter_anisotropic->modified = false;
	}

	if (gl_hudscale->modified)
	{
		int width, height;

		gl_hudscale->modified = false;

		if (gl_hudscale->value < 1.0f)
		{
			ri.Cvar_Set ("gl_hudscale", "1.0");
		}
		else
		{
			//r1: hudscaling
			width = (int)ceilf((float)viddef.width / gl_hudscale->value);
			height = (int)ceilf((float)viddef.height / gl_hudscale->value);

			//round to powers of 8/2 to avoid blackbars
			width = (width+7)&~7;
			height = (height+1)&~1;

			gl_hudscale->modified = false;

			vid_scaled_width = viddef.width / gl_hudscale->value;
			vid_scaled_height = viddef.height / gl_hudscale->value;

			// let the sound and input subsystems know about the new window
			ri.Vid_NewWindow (width, height);
		}
	}

#if 0
	if ( gl_texturealphamode->modified )
	{
		GL_TextureAlphaMode( gl_texturealphamode->string );
		gl_texturealphamode->modified = false;
	}

	if ( gl_texturesolidmode->modified )
	{
		GL_TextureSolidMode( gl_texturesolidmode->string );
		gl_texturesolidmode->modified = false;
	}
#endif

	if (gl_texture_formats->modified)
	{
		load_png_wals = strstr (gl_texture_formats->string, "png") ? true : false;
		load_jpg_wals = strstr (gl_texture_formats->string, "jpg") ? true : false;
		load_tga_wals = strstr (gl_texture_formats->string, "tga") ? true : false;
		gl_texture_formats->modified = false;
	}

	if (gl_pic_formats->modified)
	{
		load_png_pics = strstr (gl_pic_formats->string, "png") ? true : false;
		load_jpg_pics = strstr (gl_pic_formats->string, "jpg") ? true : false;
		load_tga_pics = strstr (gl_pic_formats->string, "tga") ? true : false;
		gl_pic_formats->modified = false;
	}

	/*
	** swapinterval stuff
	*/
	GL_UpdateSwapInterval();

	//
	// clear screen if desired
	//
	R_Clear ();
}

/*
=============
R_SetPalette
=============
*/
unsigned r_rawpalette[256];

void R_SetPalette ( const unsigned char *palette)
{
	/*int		i;

	byte *rp = ( byte * ) r_rawpalette;

	if ( palette )
	{
		for ( i = 0; i < 256; i++ )
		{
			rp[i*4+0] = palette[i*3+0];
			rp[i*4+1] = palette[i*3+1];
			rp[i*4+2] = palette[i*3+2];
			rp[i*4+3] = 0xff;
		}
	}
	else
	{
		for ( i = 0; i < 256; i++ )
		{
			rp[i*4+0] = d_8to24table[i] & 0xff;
			rp[i*4+1] = ( d_8to24table[i] >> 8 ) & 0xff;
			rp[i*4+2] = ( d_8to24table[i] >> 16 ) & 0xff;
			rp[i*4+3] = 0xff;
		}
	}
	GL_SetTexturePalette( r_rawpalette );*/

	qglClearColor (0,0,0,0);
	qglClear (GL_COLOR_BUFFER_BIT);
	qglClearColor (1,0, 0.5 , 0.5);
}

/*
** R_DrawBeam
*/
void R_DrawBeam( entity_t *e )
{
#define NUM_BEAM_SEGS 6

	int	i;
	float r, g, b;

	vec3_t perpvec;
	vec3_t direction, normalized_direction;
	vec3_t	start_points[NUM_BEAM_SEGS], end_points[NUM_BEAM_SEGS];
	vec3_t oldorigin, origin;

	oldorigin[0] = e->oldorigin[0];
	oldorigin[1] = e->oldorigin[1];
	oldorigin[2] = e->oldorigin[2];

	origin[0] = e->origin[0];
	origin[1] = e->origin[1];
	origin[2] = e->origin[2];

	normalized_direction[0] = direction[0] = oldorigin[0] - origin[0];
	normalized_direction[1] = direction[1] = oldorigin[1] - origin[1];
	normalized_direction[2] = direction[2] = oldorigin[2] - origin[2];

	if ( VectorNormalize( normalized_direction ) == 0 )
		return;

	PerpendicularVector( perpvec, normalized_direction );
	VectorScale( perpvec, e->frame / 2, perpvec );

	for ( i = 0; i < 6; i++ )
	{
		RotatePointAroundVector( start_points[i], normalized_direction, perpvec, (360.0f/NUM_BEAM_SEGS)*i );
		VectorAdd( start_points[i], origin, start_points[i] );
		VectorAdd( start_points[i], direction, end_points[i] );
	}

	qglDisable( GL_TEXTURE_2D );
	qglEnable( GL_BLEND );
	qglDepthMask( GL_FALSE );

	r = (float)(( d_8to24table[e->skinnum & 0xFF] ) & 0xFF);
	g = (float)(( d_8to24table[e->skinnum & 0xFF] >> 8 ) & 0xFF);
	b = (float)(( d_8to24table[e->skinnum & 0xFF] >> 16 ) & 0xFF);

	r *= 1/255.0F;
	g *= 1/255.0F;
	b *= 1/255.0F;

	qglColor4f( r, g, b, e->alpha );

	qglBegin( GL_TRIANGLE_STRIP );
	for ( i = 0; i < NUM_BEAM_SEGS; i++ )
	{
		qglVertex3f(start_points[i][0], start_points[i][1], start_points[i][2]);
		qglVertex3f(end_points[i][0], end_points[i][1], end_points[i][2]);
		int lastPt = (i + 1) % NUM_BEAM_SEGS;
		qglVertex3f(start_points[lastPt][0], start_points[lastPt][1], start_points[lastPt][2]);
		qglVertex3f(end_points[lastPt][0], end_points[lastPt][1], end_points[lastPt][2]);
	}
	qglEnd();

	qglEnable( GL_TEXTURE_2D );
	qglDisable( GL_BLEND );
	qglDepthMask( GL_TRUE );
}

//===================================================================


void	R_BeginRegistration (char *map);
struct model_s	* R_RegisterModel (char *name);
struct image_s	* R_RegisterSkin (char *name);
void R_SetSky (char *name, float rotate, vec3_t axis);
void EXPORT	R_EndRegistration (void);

void EXPORT	R_RenderFrame (refdef_t *fd);

struct image_s	* Draw_FindPic (char *name);

void	Draw_Pic (int x, int y, char *name);
void	Draw_Char (int x, int y, int c);
void	Draw_TileClear (int x, int y, int w, int h, char *name);
void	Draw_Fill (int x, int y, int w, int h, int c);
void	Draw_FadeScreen (void);

/*
@@@@@@@@@@@@@@@@@@@@@
GetRefAPI

@@@@@@@@@@@@@@@@@@@@@
*/
void GetExtraAPI (refimportnew_t rimp )
{
	if (rimp.APIVersion != EXTENDED_API_VERSION)
	{
		ri.Con_Printf (PRINT_ALL, "R1GL: ExtraAPI version number mismatch, expected version %d, got version %d.\n", EXTENDED_API_VERSION, rimp.APIVersion);
		return;
	}

	memcpy (&rx, &rimp, sizeof(rx));
}

refexport_t GetRefAPI (refimport_t rimp )
{
	refexport_t	re;

	ri = rimp;

	re.api_version = API_VERSION;

	re.BeginRegistration = R_BeginRegistration;
	re.RegisterModel = R_RegisterModel;
	re.RegisterSkin = R_RegisterSkin;
	re.RegisterPic = Draw_FindPic;
	re.SetSky = R_SetSky;
	re.EndRegistration = R_EndRegistration;

	re.RenderFrame = R_RenderFrame;

	re.DrawGetPicSize = Draw_GetPicSize;
	re.DrawPic = Draw_Pic;
	re.DrawStretchPic = Draw_StretchPic;
	re.DrawChar = Draw_Char;
	re.DrawTileClear = Draw_TileClear;
	re.DrawFill = Draw_Fill;
	re.DrawFadeScreen= Draw_FadeScreen;

	re.DrawStretchRaw = Draw_StretchRaw;

	re.Init = R_Init;
	re.Shutdown = R_Shutdown;

	re.CinematicSetPalette = R_SetPalette;
	re.BeginFrame = R_BeginFrame;
	re.EndFrame = GLimp_EndFrame;

	re.AppActivate = GLimp_AppActivate;

	Swap_Init ();

	return re;
}



/* $Id: gl_sdl.c,v 1.3 2007/05/12 20:09:36 r1ch Exp $
 *
 * all os-specific SDL refresher code
 *
 * Copyright (c) 2002 The QuakeForge Project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  
 * 
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

/*
** RW_SDL.C
**
** This file contains ALL Linux specific stuff having to do with the
** software refresh.  When a port is being made the following functions
** must be implemented by the port:
**
** GLimp_EndFrame
** GLimp_Init
** GLimp_InitGraphics
** GLimp_Shutdown
** GLimp_SwitchFullscreen
*/

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <SDL.h>

#include "../client/client.h"
#include "../ref_gl/gl_local.h"
#include "../linux/glw.h"

#include "../client/keys.h"

/*****************************************************************************/

#define JOY_AXIS_X			0
#define JOY_AXIS_Y			1
#define JOY_AXIS_Z			2
#define JOY_AXIS_R			3
#define JOY_AXIS_U			4
#define JOY_AXIS_V			5

static qboolean                 X11_active = false;


static SDL_Window *window;
static SDL_GLContext glcontext;


struct
{
	int key;
	int down;
} keyq[64];
int keyq_head=0;
int keyq_tail=0;

glwstate_t glw_state;
						      
// Console variables that we need to access from this module

/*****************************************************************************/
/* MOUSE                                                                     */
/*****************************************************************************/

// this is inside the renderer shared lib, so these are called from vid_so

static qboolean        mouse_avail;
static int     mouse_buttonstate;
static int     mouse_oldbuttonstate;
static int   mouse_x, mouse_y;
static int	old_mouse_x, old_mouse_y;
static int		mx, my;
static float old_windowed_mouse;
static qboolean mouse_active;

static cvar_t	*_windowed_mouse;
static cvar_t	*m_filter;
static cvar_t	*in_mouse;

static qboolean	mlooking = true;

/* stencilbuffer shadows */
qboolean have_stencil = false;

// state struct passed in Init
static in_state_t	*in_state;

cvar_t *sensitivity;
static cvar_t *my_lookstrafe;
cvar_t *m_side;
cvar_t *m_yaw;
cvar_t *m_pitch;
cvar_t *m_forward;
static cvar_t *my_freelook;

#ifdef HAVE_JOYSTICK
/************************
 * Joystick
 ************************/
static cvar_t   *in_joystick;
static cvar_t   *j_invert_y;
static qboolean joystick_avail;
static SDL_Joystick *joy;
static int joy_oldbuttonstate;
static int joy_numbuttons;
static int jx, jy, jt;
static int lr_axis, ud_axis, throttle_axis;
#endif /* HAVE_JOYSTICK */

static void Force_CenterView_f (void)
{
	in_state->viewangles[PITCH] = 0;
}

static void RW_IN_MLookDown (void) 
{ 
	mlooking = true; 
}

static void RW_IN_MLookUp (void) 
{
	mlooking = false;
	in_state->IN_CenterView_fp ();
}

void RW_IN_Init(in_state_t *in_state_p)
{
	in_state = in_state_p;

	// mouse variables
	_windowed_mouse = ri.Cvar_Get ("_windowed_mouse", "0", CVAR_ARCHIVE);
	m_filter = ri.Cvar_Get ("m_filter", "0", 0);
	in_mouse = ri.Cvar_Get ("in_mouse", "1", CVAR_ARCHIVE);
#ifdef HAVE_JOYSTICK
	in_joystick = ri.Cvar_Get("in_joystick", "0", CVAR_ARCHIVE);
	j_invert_y = ri.Cvar_Get("j_invert_y", "1", 0);
	lr_axis = (int) ri.Cvar_Get("j_lr_axis", "0", CVAR_ARCHIVE)->value;
	ud_axis = (int) ri.Cvar_Get("j_ud_axis", "1", CVAR_ARCHIVE)->value;
	throttle_axis = (int) ri.Cvar_Get("j_throttle", "3", CVAR_ARCHIVE)->value;
#endif
	my_freelook = ri.Cvar_Get( "freelook", "0", 0);
	my_lookstrafe = ri.Cvar_Get ("lookstrafe", "0", 0);
	
	sensitivity = ri.Cvar_Get ("sensitivity", "3", 0);
	m_pitch = ri.Cvar_Get ("m_pitch", "0.022", 0);
	m_yaw = ri.Cvar_Get ("m_yaw", "0.022", 0);
	m_forward = ri.Cvar_Get ("m_forward", "1", 0);
	m_side = ri.Cvar_Get ("m_side", "0.8", 0);

	ri.Cmd_AddCommand ("+mlook", RW_IN_MLookDown);
	ri.Cmd_AddCommand ("-mlook", RW_IN_MLookUp);

	ri.Cmd_AddCommand ("force_centerview", Force_CenterView_f);

	mouse_x = mouse_y = 0.0;
	mouse_avail = true;
}

void RW_IN_Shutdown(void) {
    if (mouse_avail) {
	mouse_avail = false;

	ri.Cmd_RemoveCommand ("+mlook");
	ri.Cmd_RemoveCommand ("-mlook");

	ri.Cmd_RemoveCommand ("force_centerview");
    }

#ifdef HAVE_JOYSTICK
    if (joy) {
	SDL_JoystickClose(joy);
	joy = NULL;
    }
#endif
}

/*
===========
IN_Commands
===========
*/
void RW_IN_Commands (void)
{
    int i;
#ifdef HAVE_JOYSTICK
    int key_index;
#endif
   
    if (mouse_avail) {
	for (i = 0; i < 3; i++) {
	    if ( (mouse_buttonstate & (1<<i)) && !(mouse_oldbuttonstate & (1<<i)) )
		in_state->Key_Event_fp (K_MOUSE1 + i, true);
	    
	    if ( !(mouse_buttonstate & (1<<i)) && (mouse_oldbuttonstate & (1<<i)) )
		in_state->Key_Event_fp (K_MOUSE1 + i, false);
	}
	/* can't put in loop because K_MOUSE4 doesn't come after K_MOUSE3 */
	if ((mouse_buttonstate & (1<<3)) && !(mouse_oldbuttonstate & (1<<3)))
	    in_state->Key_Event_fp(K_MOUSE4, true);
	if (!(mouse_buttonstate * (1<<3)) && (mouse_oldbuttonstate & (1<<3)))
	    in_state->Key_Event_fp(K_MOUSE4, false);
	
	if ((mouse_buttonstate & (1<<4)) && !(mouse_oldbuttonstate & (1<<4)))
	    in_state->Key_Event_fp(K_MOUSE5, true);
	if (!(mouse_buttonstate * (1<<4)) && (mouse_oldbuttonstate & (1<<4)))
	    in_state->Key_Event_fp(K_MOUSE5, false);
	
	mouse_oldbuttonstate = mouse_buttonstate;
    }
#ifdef HAVE_JOYSTICK
    if (joystick_avail && joy) {
	for (i = 0; i < joy_numbuttons; i++) {
	    if (SDL_JoystickGetButton(joy, i) && joy_oldbuttonstate != i) {
		key_index = (i < 4) ? K_JOY1 : K_AUX1;
		in_state->Key_Event_fp(key_index + i, true);
		joy_oldbuttonstate = i;
	    }
	    if (!SDL_JoystickGetButton(joy, i) && joy_oldbuttonstate != i) {
		key_index = (i < 4) ? K_JOY1 : K_AUX1;
		in_state->Key_Event_fp(key_index + i, false);
		joy_oldbuttonstate = i;
	    }
	}
    }
#endif
}

/*
===========
IN_Move
===========
*/
void RW_IN_Move (usercmd_t *cmd)
{
  /*** FIXME 
   *   You can accelerate while in the air, this doesn't
   *   make physical sense.  Try falling off something and then moving
   *   forward.
   ***/
  
  if (mouse_avail) {
    if (m_filter->value)
      {
	mouse_x = (mx + old_mouse_x) * 0.5;
	mouse_y = (my + old_mouse_y) * 0.5;
      } else {
	mouse_x = mx;
	mouse_y = my;
      }
    
    old_mouse_x = mx;
    old_mouse_y = my;
    
    if (mouse_x || mouse_y) {    
      mouse_x *= sensitivity->value;
      mouse_y *= sensitivity->value;
      
      /* add mouse X/Y movement to cmd */
      if ( (*in_state->in_strafe_state & 1) || 
	   (my_lookstrafe->value && mlooking ))
	cmd->sidemove += m_side->value * mouse_x;
      else
	in_state->viewangles[YAW] -= m_yaw->value * mouse_x;
      
      
      if ( (mlooking || my_freelook->value) && 
	   !(*in_state->in_strafe_state & 1))
	{
	  in_state->viewangles[PITCH] += m_pitch->value * mouse_y;
	}
      else
	{
	  cmd->forwardmove -= m_forward->value * mouse_y;
	}
      mx = my = 0;
    }
  }
#ifdef HAVE_JOYSTICK
  if (joystick_avail && joy) {
      /* add joy X/Y movement to cmd */
    if ( (*in_state->in_strafe_state & 1) || 
	 (my_lookstrafe->value && mlooking ))
      cmd->sidemove += m_side->value * (jx/100);
    else
      in_state->viewangles[YAW] -= m_yaw->value * (jx/100);

    if ((mlooking || my_freelook->value) && !(*in_state->in_strafe_state & 1)) {
	if (j_invert_y)
	    in_state->viewangles[PITCH] -= m_pitch->value * (jy/100);
	else
	    in_state->viewangles[PITCH] += m_pitch->value * (jy/100);
	cmd->forwardmove -= m_forward->value * (jt/100);
    } else {
	cmd->forwardmove -= m_forward->value * (jy/100);
    }
    jt = jx = jy = 0;
  }
#endif
}

void IN_DeactivateMouse( void ) 
{ 
	if (!mouse_avail)
		return;

	if (mouse_active) {
		/* uninstall_grabs(); */
		mouse_active = false;
	}
}

static void IN_ActivateMouse( void ) 
{
	if (!mouse_avail)
		return;

	if (!mouse_active) {
		mx = my = 0; // don't spazz
		/* install_grabs(); */
		mouse_active = true;
	}
}

void RW_IN_Frame (void)
{
}

void RW_IN_Activate(qboolean active)
{
    /*	if (active || vidmode_active) */
    	if (active)
		IN_ActivateMouse();
	else
		IN_DeactivateMouse();
}

/*****************************************************************************/

#if 0 /* SDL parachute should catch everything... */
// ========================================================================
// Tragic death handler
// ========================================================================

void TragicDeath(int signal_num)
{
	/* SDL_Quit(); */
	Sys_Error("This death brought to you by the number %d\n", signal_num);
}
#endif

int XLateKey(unsigned int keysym)
{
	int key;
	
	key = 0;
	switch(keysym) {

		case SDLK_KP_0:			key = K_KP_INS; break;
		case SDLK_KP_1:			key = K_KP_END; break;
		case SDLK_KP_2:			key = K_KP_DOWNARROW; break;
		case SDLK_KP_3:			key = K_KP_PGDN; break;
		case SDLK_KP_4:			key = K_KP_LEFTARROW; break;
		case SDLK_KP_5:			key = K_KP_5; break;
		case SDLK_KP_6:			key = K_KP_RIGHTARROW; break;
		case SDLK_KP_7:			key = K_KP_HOME; break;
		case SDLK_KP_8:			key = K_KP_UPARROW; break;
		case SDLK_KP_9:			key = K_KP_PGUP; break;
		
		/* suggestions on how to handle this better would be appreciated */
		case SDLK_BACKQUOTE:	key = '`'; break;

		// WTF: the above should get this but doesn't
		case 167			:	key = '`'; break;

		case SDLK_PAGEUP:		key = K_PGUP; break;
		
		case SDLK_PAGEDOWN:		key = K_PGDN; break;
		
		case SDLK_HOME:			key = K_HOME; break;
		
		case SDLK_END:			key = K_END; break;
		
		case SDLK_LEFT:			key = K_LEFTARROW; break;
		
		case SDLK_RIGHT:		key = K_RIGHTARROW; break;
		
		case SDLK_DOWN:			key = K_DOWNARROW; break;
		
		case SDLK_UP:			key = K_UPARROW; break;
		
		case SDLK_ESCAPE:		key = K_ESCAPE; break;
		
		case SDLK_KP_ENTER:		key = K_KP_ENTER; break;
		case SDLK_RETURN:		key = K_ENTER; break;
		
		case SDLK_TAB:			key = K_TAB; break;
		
		case SDLK_F1:			key = K_F1; break;
		case SDLK_F2:			key = K_F2; break;
		case SDLK_F3:			key = K_F3; break;
		case SDLK_F4:			key = K_F4; break;
		case SDLK_F5:			key = K_F5; break;
		case SDLK_F6:			key = K_F6; break;
		case SDLK_F7:			key = K_F7; break;
		case SDLK_F8:			key = K_F8; break;
		case SDLK_F9:			key = K_F9; break;
		case SDLK_F10:			key = K_F10; break;
		case SDLK_F11:			key = K_F11; break;
		case SDLK_F12:			key = K_F12; break;
		
		case SDLK_BACKSPACE:		key = K_BACKSPACE; break;
		
		case SDLK_KP_PERIOD:		key = K_KP_DEL; break;
		case SDLK_DELETE:		key = K_DEL; break;
		
		case SDLK_PAUSE:		key = K_PAUSE; break;
		
		case SDLK_LSHIFT:
		case SDLK_RSHIFT:		key = K_SHIFT; break;
		
		case SDLK_LCTRL:
		case SDLK_RCTRL:		key = K_CTRL; break;
		
		case SDLK_LALT:
		case SDLK_RALT:			key = K_ALT; break;

		case SDLK_INSERT:		key = K_INS; break;
		
		case SDLK_KP_MULTIPLY:		key = '*'; break;
		case SDLK_KP_PLUS:		key = K_KP_PLUS; break;
		case SDLK_KP_MINUS:		key = K_KP_MINUS; break;
		case SDLK_KP_DIVIDE:		key = K_KP_SLASH; break;
		
		default: /* assuming that the other sdl keys are mapped to ascii */
			if (keysym < 128)
				key = keysym;
			break;
	}

	return key;		
}


static unsigned char KeyStates[SDL_NUM_SCANCODES];


void GetEvent(SDL_Event *event)
{
	unsigned int key;
	
	switch(event->type) {
	case SDL_MOUSEMOTION:
		mx += event->motion.xrel; my += event->motion.yrel;
		break;

	case SDL_MOUSEWHEEL:
		if (event->wheel.y > 0) {
			keyq[keyq_head].key = K_MWHEELUP;
			keyq[keyq_head].down = true;
			keyq_head = (keyq_head + 1) & 63;
			keyq[keyq_head].key = K_MWHEELUP;
			keyq[keyq_head].down = false;
			keyq_head = (keyq_head + 1) & 63;
		} else {
			keyq[keyq_head].key = K_MWHEELDOWN;
			keyq[keyq_head].down = true;
			keyq_head = (keyq_head + 1) & 63;
			keyq[keyq_head].key = K_MWHEELDOWN;
			keyq[keyq_head].down = false;
			keyq_head = (keyq_head + 1) & 63;
		}
		break;
	case SDL_MOUSEBUTTONUP:
		break;
#ifdef HAVE_JOYSTICK
	case SDL_JOYBUTTONDOWN:
	  keyq[keyq_head].key = 
	    ((((SDL_JoyButtonEvent*)event)->button < 4)?K_JOY1:K_AUX1)+
	    ((SDL_JoyButtonEvent*)event)->button;
	  keyq[keyq_head].down = true;
	  keyq_head = (keyq_head+1)&63;
	  break;
	case SDL_JOYBUTTONUP:
	  keyq[keyq_head].key = 
	    ((((SDL_JoyButtonEvent*)event)->button < 4)?K_JOY1:K_AUX1)+
	    ((SDL_JoyButtonEvent*)event)->button;
	  keyq[keyq_head].down = false;
	  keyq_head = (keyq_head+1)&63;
	  break;
#endif
	case SDL_KEYDOWN:
		if ( (KeyStates[SDL_SCANCODE_LALT] || KeyStates[SDL_SCANCODE_RALT]) &&
			(event->key.keysym.sym == SDLK_RETURN) ) {
			cvar_t	*fullscreen;

			int flags = SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN;

			// this is AFTER the change
			int fs = flags ? 0 : 1;
			// TODO: support SDL_WINDOW_FULLSCREEN_DESKTOP
			flags = fs ? SDL_WINDOW_FULLSCREEN : 0;

			int retval = SDL_SetWindowFullscreen(window, flags);
			if (retval != 0) {
				ri.Con_Printf(PRINT_ALL, "SDL_SetWindowFullscreen failed: \"%s\"\n", SDL_GetError());
				// not fatal but since fullscreen failed...
				fs = flags ? 1 : 0;
			}


			ri.Cvar_SetValue( "vid_fullscreen", fs );

			fullscreen = ri.Cvar_Get( "vid_fullscreen", "0", 0 );
			fullscreen->modified = false;	// we just changed it with SDL.

			break; /* ignore this key */
		}
		
		if ( (KeyStates[SDL_SCANCODE_LCTRL] || KeyStates[SDL_SCANCODE_RCTRL]) &&
			(event->key.keysym.sym == SDLK_g) ) {

			int newValue = (SDL_GetWindowGrab(window) == SDL_TRUE) ? /*1*/ 0 : /*0*/ 1;
			/*	
			SDL_WM_GrabInput((gm == SDL_GRAB_ON) ? SDL_GRAB_OFF : SDL_GRAB_ON);
			gm = SDL_WM_GrabInput(SDL_GRAB_QUERY);
			*/	
			ri.Cvar_SetValue( "_windowed_mouse", newValue );
			
			break; /* ignore this key */
		}

		KeyStates[event->key.keysym.scancode] = 1;
		
		key = XLateKey(event->key.keysym.sym);
		if (key) {
			keyq[keyq_head].key = key;
			keyq[keyq_head].down = true;
			keyq_head = (keyq_head + 1) & 63;
		}
		break;
	case SDL_KEYUP:
		if (KeyStates[event->key.keysym.scancode]) {
			KeyStates[event->key.keysym.scancode] = 0;
		
			key = XLateKey(event->key.keysym.sym);
			if (key) {
				keyq[keyq_head].key = key;
				keyq[keyq_head].down = false;
				keyq_head = (keyq_head + 1) & 63;
			}
		}
		break;

	case SDL_WINDOWEVENT:
		switch (event->window.event) {
		case SDL_WINDOWEVENT_RESIZED:
			viddef.width = event->window.data1;
			viddef.height = event->window.data2;
			vid_scaled_width = viddef.width / gl_hudscale->value;
			vid_scaled_height = viddef.height / gl_hudscale->value;
		}
		break;

	case SDL_QUIT:
		ri.Cmd_ExecuteText(EXEC_NOW, "quit");
		break;
	}

}

void init_joystick() {
#ifdef HAVE_JOYSTICK
    int num_joysticks, i;
    joy = NULL;

    if (!(SDL_INIT_JOYSTICK&SDL_WasInit(SDL_INIT_JOYSTICK))) {
	ri.Con_Printf(PRINT_ALL, "SDL Joystick not initialized, trying to init...\n");
	SDL_Init(SDL_INIT_JOYSTICK);
    }
    if (in_joystick) {
	ri.Con_Printf(PRINT_ALL, "Trying to start-up joystick...\n");
	if ((num_joysticks=SDL_NumJoysticks())) {
	    for(i=0;i<num_joysticks;i++) {
		ri.Con_Printf(PRINT_ALL, "Trying joystick [%s]\n", 
			      SDL_JoystickName(i));
		if (!SDL_JoystickOpened(i)) {
		    joy = SDL_JoystickOpen(i);
		    if (joy) {
			ri.Con_Printf(PRINT_ALL, "Joytick activated.\n");
			joystick_avail = true;
			joy_numbuttons = SDL_JoystickNumButtons(joy);
			break;
		    }
		}
	    }
	    if (!joy) {
		ri.Con_Printf(PRINT_ALL, "Failed to open any joysticks\n");
		joystick_avail = false;
	    }
	}
	else {
	    ri.Con_Printf(PRINT_ALL, "No joysticks available\n");
	    joystick_avail = false;
	}
    }
    else {
	ri.Con_Printf(PRINT_ALL, "Joystick Inactive\n");
	joystick_avail = false;
    }
#endif
}

void InitJoystick() {
#ifdef HAVE_JOYSTICK
  int num_joysticks, i;
  joy = NULL;

  if (!(SDL_INIT_JOYSTICK&SDL_WasInit(SDL_INIT_JOYSTICK))) {
    ri.Con_Printf(PRINT_ALL, "SDL Joystick not initialized, trying to init...\n");
    SDL_Init(SDL_INIT_JOYSTICK);
  }
  if (in_joystick) {
    ri.Con_Printf(PRINT_ALL, "Trying to start-up joystick...\n");
    if ((num_joysticks=SDL_NumJoysticks())) {
      for(i=0;i<num_joysticks;i++) {
	ri.Con_Printf(PRINT_ALL, "Trying joystick [%s]\n", 
		      SDL_JoystickName(i));
	if (!SDL_JoystickOpened(i)) {
	  joy = SDL_JoystickOpen(0);
	  if (joy) {
	    ri.Con_Printf(PRINT_ALL, "Joytick activated.\n");
	    joystick_avail = true;
	    joy_numbuttons = SDL_JoystickNumButtons(joy);
	    break;
	  }
	}
      }
      if (!joy) {
	ri.Con_Printf(PRINT_ALL, "Failed to open any joysticks\n");
	joystick_avail = false;
      }
    }
    else {
      ri.Con_Printf(PRINT_ALL, "No joysticks available\n");
      joystick_avail = false;
    }
  }
  else {
    ri.Con_Printf(PRINT_ALL, "Joystick Inactive\n");
    joystick_avail = false;
  }
#endif
}

/*****************************************************************************/

/*
** SWimp_Init
**
** This routine is responsible for initializing the implementation
** specific stuff in a software rendering subsystem.
*/
int SWimp_Init( void *hInstance, void *wndProc )
{
	if (SDL_WasInit(SDL_INIT_AUDIO | SDL_INIT_VIDEO) == 0) {
		if (SDL_Init(SDL_INIT_VIDEO) < 0) {
			Sys_Error("SDL Init failed: %s\n", SDL_GetError());
			return false;
		}
	} else if (SDL_WasInit(SDL_INIT_VIDEO) == 0) {
		if (SDL_InitSubSystem(SDL_INIT_VIDEO) < 0) {
			Sys_Error("SDL Init failed: %s\n", SDL_GetError());
			return false;
		}
	}

// catch signals so i can turn on auto-repeat
#if 0
 	{
		struct sigaction sa;
		sigaction(SIGINT, 0, &sa);
		sa.sa_handler = TragicDeath;
		sigaction(SIGINT, &sa, 0);
		sigaction(SIGTERM, &sa, 0);
	}
#endif
#ifdef HAVE_JOYSTICK
	init_joystick();
#endif
	return true;
}


int GLimp_Init( void *hInstance, void *wndProc )
{
	return SWimp_Init(hInstance, wndProc);
}

static void SetSDLIcon(void) {
#include "q2icon.xbm"
    SDL_Surface * icon;
    SDL_Color color;
    Uint8 * ptr;
    int i, mask;

    icon = SDL_CreateRGBSurface(SDL_SWSURFACE, q2icon_width, q2icon_height, 8,
				0, 0, 0, 0);
    if (icon == NULL)
	return; /* oh well... */

    SDL_SetColorKey(icon, SDL_TRUE, 0);

    color.r = 255;
    color.g = 255;
    color.b = 255;
	icon->format->palette->colors[0] = color;
    color.r = 0;
    color.g = 16;
    color.b = 0;
	icon->format->palette->colors[1] = color;

    ptr = (Uint8 *)icon->pixels;
    for (i = 0; i < sizeof(q2icon_bits); i++) {
	for (mask = 1; mask != 0x100; mask <<= 1) {
	    *ptr = (q2icon_bits[i] & mask) ? 1 : 0;
	    ptr++;
	}               
    }

	SDL_SetWindowIcon(window, icon);
    SDL_FreeSurface(icon);
}


#ifdef EPOXY


static const char *errorSource(GLenum source)
{
	switch (source)
	{
	case GL_DEBUG_SOURCE_API:
		return "API";
		break;

	case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
		return "window system";
		break;

	case GL_DEBUG_SOURCE_SHADER_COMPILER:
		return "shader compiler";
		break;

	case GL_DEBUG_SOURCE_THIRD_PARTY:
		return "third party";
		break;

	case GL_DEBUG_SOURCE_APPLICATION:
		return "application";
		break;

	case GL_DEBUG_SOURCE_OTHER:
		return "other";
		break;

	default:
		break;
	}

	return "unknown source";
}


static const char *errorType(GLenum type)
{
	switch (type)
	{
	case GL_DEBUG_TYPE_ERROR:
	case GL_DEBUG_CATEGORY_API_ERROR_AMD:
		return "error";
		break;

	case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
	case GL_DEBUG_CATEGORY_DEPRECATION_AMD:
		return "deprecated behavior";
		break;

	case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
	case GL_DEBUG_CATEGORY_UNDEFINED_BEHAVIOR_AMD:
		return "undefined behavior";
		break;

	case GL_DEBUG_TYPE_PORTABILITY:
		return "portability";
		break;

	case GL_DEBUG_TYPE_PERFORMANCE:
	case GL_DEBUG_CATEGORY_PERFORMANCE_AMD:
		return "performance";
		break;

	case GL_DEBUG_TYPE_OTHER:
	case GL_DEBUG_CATEGORY_OTHER_AMD:
		return "other";
		break;

	case GL_DEBUG_CATEGORY_WINDOW_SYSTEM_AMD:
		return "window system error";
		break;

	case GL_DEBUG_CATEGORY_SHADER_COMPILER_AMD:
		return "shader compiler error";
		break;

	case GL_DEBUG_CATEGORY_APPLICATION_AMD:
		return "application error";
		break;

	default:
		break;

	}

	return "unknown type";
}


#ifndef CALLBACK
#define CALLBACK
#endif


void CALLBACK glDebugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam)
{
	switch (severity)
	{
	case GL_DEBUG_SEVERITY_HIGH_ARB:
		ri.Con_Printf( PRINT_ALL, "GL error from %s type %s: (%d) %s\n", errorSource(source), errorType(type), id, message);
		break;

	case GL_DEBUG_SEVERITY_MEDIUM_ARB:
		ri.Con_Printf( PRINT_ALL, "GL warning from %s type %s: (%d) %s\n", errorSource(source), errorType(type), id, message);
		break;

	case GL_DEBUG_SEVERITY_LOW_ARB:
		ri.Con_Printf( PRINT_ALL, "GL debug from %s type %s: (%d) %s\n", errorSource(source), errorType(type), id, message);
		break;

	default:
		ri.Con_Printf( PRINT_ALL, "GL error of unknown severity %x from %s type %s: (%d) %s\n", severity, errorSource(source), errorType(type), id, message);
		break;
	}
}


#endif  // EPOXY


/*
** SWimp_InitGraphics
**
** This initializes the software refresh's implementation specific
** graphics subsystem.  In the case of Windows it creates DIB or
** DDRAW surfaces.
**
** The necessary width and height parameters are grabbed from
** vid.width and vid.height.
*/
static qboolean GLimp_InitGraphics( qboolean fullscreen )
{
	/* Just toggle fullscreen if that's all that has been changed */
	int oldW = -1, oldH = -1;
	if (window) {
		SDL_GetWindowSize(window, &oldW, &oldH);
	}

	if ((oldW == viddef.width) && (oldH == viddef.height)) {
		int isfullscreen = (SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN) ? 1 : 0;
		if (fullscreen != isfullscreen) {
			// TODO: support SDL_WINDOW_FULLSCREEN_DESKTOP
			SDL_SetWindowFullscreen(window, fullscreen ? SDL_WINDOW_FULLSCREEN : 0);
		}

		isfullscreen = (SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN) ? 1 : 0;
		if (fullscreen == isfullscreen)
			return true;
	}
	
	srand(getpid());

#if 0
	// for testing on desktop
	// nvidia lets us create GLES contexts
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);

#if 0
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

#else

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif

#endif  // 0

	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 5);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 5);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 5);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	
	if (window) {
		SDL_SetWindowSize(window, viddef.width, viddef.height);
	} else {
		// TODO: SDL_WINDOW_RESIZABLE
		// TODO: SDL_WINDOW_INPUT_GRABBED?
		// TODO: SDL_WINDOW_ALLOW_HIGHDPI?
		uint32_t flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;
		if (fullscreen) {
			// TODO: SDL_WINDOW_FULLSCREEN_DESKTOP
			flags |= SDL_WINDOW_FULLSCREEN;
		}

		window = SDL_CreateWindow("Quake II", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, viddef.width, viddef.height, flags);
		if (!window) {
			Sys_Error("(SDLGL) SDL CreateWindow failed: %s\n", SDL_GetError());
			return false;
		}
		SetSDLIcon();
		glcontext = SDL_GL_CreateContext(window);

#ifdef EPOXY
		if (epoxy_has_gl_extension("GL_KHR_debug")) {
			ri.Con_Printf( PRINT_ALL, "KHR_debug found\n" );

			glDebugMessageCallback(glDebugCallback, NULL);
			glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, GL_TRUE);

			glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);

			// nvidia warning about inconsistent texture base level
			// which fires on all glClear operations
			// tell the driver to STFU
			{
				const unsigned int ids[] = { 131204 };
				glDebugMessageControl(GL_DEBUG_SOURCE_API, GL_DEBUG_TYPE_OTHER, GL_DONT_CARE, 1, ids, GL_FALSE);
			}

		} else {
			ri.Con_Printf( PRINT_ALL, "No KHR_debug\n" );
		}
#endif  // EPOXY

	}

	// let the sound and input subsystems know about the new window
	ri.Vid_NewWindow(viddef.width, viddef.height);

	X11_active = true;

	return true;
}

void GLimp_BeginFrame(void)
{
}

/*
** GLimp_EndFrame
**
** This does an implementation specific copy from the backbuffer to the
** front buffer.  In the Win32 case it uses BitBlt or BltFast depending
** on whether we're using DIB sections/GDI or DDRAW.
*/

void GLimp_EndFrame (void)
{
	SDL_GL_SwapWindow(window);
}

/*
** GLimp_SetMode
*/
int GLimp_SetMode( unsigned int *pwidth, unsigned int *pheight, int mode, qboolean fullscreen )
{
	ri.Con_Printf (PRINT_ALL, "setting mode %d:", mode );

	if ( !ri.Vid_GetModeInfo( pwidth, pheight, mode ) )
	{
		ri.Con_Printf( PRINT_ALL, " invalid mode\n" );
		return VID_ERR_INVALID_MODE;
	}

	ri.Con_Printf( PRINT_ALL, " %d %d\n", *pwidth, *pheight);

	if ( !GLimp_InitGraphics( fullscreen ) ) {
		// failed to set a valid mode in windowed mode
		return VID_ERR_INVALID_MODE;
	}

	return VID_ERR_NONE;
}


/*
** SWimp_Shutdown
**
** System specific graphics subsystem shutdown routine.  Destroys
** DIBs or DDRAW surfaces as appropriate.
*/

void SWimp_Shutdown( void )
{
	if (glcontext) {
		SDL_GL_DeleteContext(glcontext);
		glcontext = NULL;
	}

	if (window) {
		SDL_DestroyWindow(window);
		window = NULL;
	}
	
	if (SDL_WasInit(SDL_INIT_EVERYTHING) == SDL_INIT_VIDEO)
		SDL_Quit();
	else
		SDL_QuitSubSystem(SDL_INIT_VIDEO);
		
	X11_active = false;
}

void GLimp_Shutdown( void )
{
	SWimp_Shutdown();
}


/*****************************************************************************/
/* KEYBOARD                                                                  */
/*****************************************************************************/

Key_Event_fp_t Key_Event_fp;

void KBD_Init(Key_Event_fp_t fp)
{
	Key_Event_fp = fp;
}

void KBD_Update(void)
{
	SDL_Event event;
	static int KBD_Update_Flag;
	
	if (KBD_Update_Flag == 1)
		return;
	
	KBD_Update_Flag = 1;
	
// get events from x server
	if (X11_active)
	{
		mx = 0; my = 0;
		int bstate;
		
		while (SDL_PollEvent(&event))
			GetEvent(&event);

#ifdef HAVE_JOYSTICK
	if (joystick_avail && joy) {
	  jx = SDL_JoystickGetAxis(joy, lr_axis);
	  jy = SDL_JoystickGetAxis(joy, ud_axis);
	  jt = SDL_JoystickGetAxis(joy, throttle_axis);
	}
#endif	
	mouse_buttonstate = 0;
	bstate = SDL_GetMouseState(NULL, NULL);
	if (SDL_BUTTON(1) & bstate)
		mouse_buttonstate |= (1 << 0);
	if (SDL_BUTTON(3) & bstate) /* quake2 has the right button be mouse2 */
		mouse_buttonstate |= (1 << 1);
	if (SDL_BUTTON(2) & bstate) /* quake2 has the middle button be mouse3 */
		mouse_buttonstate |= (1 << 2);
	if (SDL_BUTTON(6) & bstate)
		mouse_buttonstate |= (1 << 3);

	if (SDL_BUTTON(7) & bstate)
		mouse_buttonstate |= (1 << 4);
	
	if (old_windowed_mouse != _windowed_mouse->value) {
		old_windowed_mouse = _windowed_mouse->value;

		// TODO: should refactor all this grab stuff to one place
		SDL_SetWindowGrab(window, _windowed_mouse->value ? SDL_TRUE : SDL_FALSE);
		SDL_SetRelativeMouseMode(_windowed_mouse->value ? SDL_TRUE : SDL_FALSE);
	}			
		while (keyq_head != keyq_tail)
		{
			Key_Event_fp(keyq[keyq_tail].key, keyq[keyq_tail].down);
			keyq_tail = (keyq_tail + 1) & 63;
		}
	}
	
	KBD_Update_Flag = 0;
}

void KBD_Close(void)
{
	keyq_head = 0;
	keyq_tail = 0;
	
	memset(keyq, 0, sizeof(keyq));
}

#include "server.h"


void (*KBD_Update_fp)(void) = NULL;


qboolean send_packet_now = false;


void CL_Shutdown(void)
{
}


void CL_Drop (qboolean skipdisconnect, qboolean nonerror)
{
}


void CL_Init (void)
{
}


void CL_Frame (int msec)
{
}


const char *CL_Get_Loc_Here (void)
{
	return NULL;
}


const char *CL_Get_Loc_There (void)
{
}


void Cmd_ForwardToServer (void)
{
}


void Con_Print (const char *txt)
{
}


void Key_Init (void)
{
}


void SCR_BeginLoadingPlaque (void)
{
}


void EXPORT SCR_DebugGraph (float value, int color)
{
}


void SCR_EndLoadingPlaque (void)
{
}

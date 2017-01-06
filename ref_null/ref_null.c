#include "../client/ref.h"
#include "../client/client.h"
#include "../client/keys.h"


#ifdef NDEBUG


#define STUBBED(x)


#else  // NDEBUG


#define STUBBED(x) do { \
	static bool seen_this = false; \
	if (!seen_this) { \
		seen_this = true; \
		Com_Printf ("STUBBED: %s at %s (%s:%d)\n", LOG_NET, \
		x, __FUNCTION__, __FILE__, __LINE__); \
	} \
	} while (0)


#endif  // NDEBUG


void KBD_Close(void) {
	STUBBED("");
}


void KBD_Update(void) {
	STUBBED("");
}


void R_BeginRegistration (char *map) {
	STUBBED("");
}


struct model_s *R_RegisterModel(const char *name) {
	STUBBED("");
	return NULL;
}


struct image_s *R_RegisterSkin(const char *name) {
	STUBBED("");
	return NULL;
}


struct image_s *Draw_FindPic (char *name) {
	STUBBED("");
	return NULL;
}


void R_DrawString(int x, int y, const char *s, int xorVal, unsigned int maxLen) {
	STUBBED("");
}


void R_DrawChar (int x, int y, int c) {
	STUBBED("");
}


void Draw_FadeScreen (void) {
	STUBBED("");
}


void Draw_Fill (int x, int y, int w, int h, int c) {
	STUBBED("");
}


void Draw_TileClear (int x, int y, int w, int h, char *name) {
	STUBBED("");
}


void Draw_StretchPic (int x, int y, int w, int h, char *name) {
	STUBBED("");
}


void Draw_Pic (int x, int y, char *name) {
	STUBBED("");
}


void Draw_GetPicSize (int *w, int *h, char *name) {
	STUBBED("");
}


void R_SetSky(char *name, float rotate, vec3_t axis) {
	STUBBED("");
}


void R_EndRegistration (void) {
	STUBBED("");
}


int R_Init( void *hinstance, void *hWnd ) {
	STUBBED("");
	return 0;
}


void R_Shutdown( void ) {
	STUBBED("");
}


void R_SetPalette ( const unsigned char *palette) {
	STUBBED("");
}


void R_BeginFrame(void) {
	STUBBED("");
}


void R_RenderFrame (refdef_t *fd) {
	STUBBED("");
}


void GLimp_EndFrame(void) {
	STUBBED("");
}


void IN_Move (usercmd_t *cmd) {
	STUBBED("");
}


void RW_IN_Init(in_state_t *in_state_p) {
	STUBBED("");
}


void RW_IN_Shutdown(void) {
	STUBBED("");
}


void RW_IN_Commands(void) {
	STUBBED("");
}


void RW_IN_Frame(void) {
	STUBBED("");
}

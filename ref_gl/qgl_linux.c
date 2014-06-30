/*
** QGL_WIN.C
**
** This file implements the operating system binding of GL to QGL function
** pointers.  When doing a port of Quake2 you must implement the following
** two functions:
**
** QGL_Init() - loads libraries, assigns function pointers, etc.
** QGL_Shutdown() - unloads libraries, NULLs function pointers
*/
#define QGL
#include "../ref_gl/gl_local.h"

#include <SDL.h>


typedef struct Vertex {
	float pos[3];
	uint32_t color;
	float tex0[2];
	float tex1[2];
} Vertex;


typedef struct QGLState {
	Vertex *vertices;
	unsigned int numVertices;
	unsigned int usedVertices;

	Vertex currentVertex;
} QGLState;


static QGLState *qglState = NULL;


void * qwglGetProcAddress(const char *procname)
{
	return SDL_GL_GetProcAddress(procname);
}

void ( APIENTRY * qglActiveTextureARB) ( GLenum );
void ( APIENTRY * qglClientActiveTextureARB) ( GLenum );

void ( APIENTRY * qglAlphaFunc )(GLenum func, GLclampf ref);
void ( APIENTRY * qglBegin )(GLenum mode);
void ( APIENTRY * qglBindTexture )(GLenum target, GLuint texture);
void ( APIENTRY * qglBlendFunc )(GLenum sfactor, GLenum dfactor);
void ( APIENTRY * qglClear )(GLbitfield mask);
void ( APIENTRY * qglClearColor )(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
void ( APIENTRY * qglClearDepth )(GLclampd depth);
void ( APIENTRY * qglClearStencil )(GLint s);
void ( APIENTRY * qglColorMask )(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha);
void ( APIENTRY * qglColorPointer )(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
void ( APIENTRY * qglCullFace )(GLenum mode);
void ( APIENTRY * qglDeleteTextures )(GLsizei n, const GLuint *textures);
void ( APIENTRY * qglDepthFunc )(GLenum func);
void ( APIENTRY * qglDepthMask )(GLboolean flag);
void ( APIENTRY * qglDepthRange )(GLclampd zNear, GLclampd zFar);
void ( APIENTRY * qglDisable )(GLenum cap);
void ( APIENTRY * qglDrawArrays )(GLenum mode, GLint first, GLsizei count);
void ( APIENTRY * qglDrawElements )(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices);
void ( APIENTRY * qglEnable )(GLenum cap);
void ( APIENTRY * qglEnd )(void);
void ( APIENTRY * qglFinish )(void);
void ( APIENTRY * qglFlush )(void);
void ( APIENTRY * qglFrontFace )(GLenum mode);
void ( APIENTRY * qglFrustum )(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
void ( APIENTRY * qglGenTextures )(GLsizei n, GLuint *textures);
void ( APIENTRY * qglGetBooleanv )(GLenum pname, GLboolean *params);
GLenum ( APIENTRY * qglGetError )(void);
void ( APIENTRY * qglGetFloatv )(GLenum pname, GLfloat *params);
void ( APIENTRY * qglGetIntegerv )(GLenum pname, GLint *params);
const GLubyte * ( APIENTRY * qglGetString )(GLenum name);
void ( APIENTRY * qglHint )(GLenum target, GLenum mode);
GLboolean ( APIENTRY * qglIsEnabled )(GLenum cap);
GLboolean ( APIENTRY * qglIsTexture )(GLuint texture);
void ( APIENTRY * qglLineWidth )(GLfloat width);
void ( APIENTRY * qglLoadIdentity )(void);
void ( APIENTRY * qglLoadMatrixf )(const GLfloat *m);
void ( APIENTRY * qglMatrixMode )(GLenum mode);
void ( APIENTRY * qglMultMatrixf )(const GLfloat *m);
void ( APIENTRY * qglOrtho )(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
void ( APIENTRY * qglPointSize )(GLfloat size);
void ( APIENTRY * qglPolygonMode )(GLenum face, GLenum mode);
void ( APIENTRY * qglPolygonOffset )(GLfloat factor, GLfloat units);
void ( APIENTRY * qglPopMatrix )(void);
void ( APIENTRY * qglPushMatrix )(void);
void ( APIENTRY * qglReadPixels )(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels);
void ( APIENTRY * qglRotatef )(GLfloat angle, GLfloat x, GLfloat y, GLfloat z);
void ( APIENTRY * qglScalef )(GLfloat x, GLfloat y, GLfloat z);
void ( APIENTRY * qglScissor )(GLint x, GLint y, GLsizei width, GLsizei height);
void ( APIENTRY * qglShadeModel )(GLenum mode);
void ( APIENTRY * qglStencilFunc )(GLenum func, GLint ref, GLuint mask);
void ( APIENTRY * qglStencilMask )(GLuint mask);
void ( APIENTRY * qglStencilOp )(GLenum fail, GLenum zfail, GLenum zpass);
void ( APIENTRY * qglTexCoordPointer )(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
void ( APIENTRY * qglTexEnvf )(GLenum target, GLenum pname, GLfloat param);
void ( APIENTRY * qglTexEnvi )(GLenum target, GLenum pname, GLint param);
void ( APIENTRY * qglTexImage2D )(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
void ( APIENTRY * qglTexParameterf )(GLenum target, GLenum pname, GLfloat param);
void ( APIENTRY * qglTexParameteri )(GLenum target, GLenum pname, GLint param);
void ( APIENTRY * qglTexSubImage2D )(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);
void ( APIENTRY * qglTranslatef )(GLfloat x, GLfloat y, GLfloat z);
void ( APIENTRY * qglVertexPointer )(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
void ( APIENTRY * qglViewport )(GLint x, GLint y, GLsizei width, GLsizei height);

void ( APIENTRY * qglPointParameterfEXT)( GLenum param, GLfloat value );
void ( APIENTRY * qglPointParameterfvEXT)( GLenum param, const GLfloat *value );
void ( APIENTRY * qglPointParameterfARB) (GLenum, GLfloat);
void ( APIENTRY * qglPointParameterfvARB) (GLenum, const GLfloat *);

void ( APIENTRY * qglColorTableEXT)( int, int, int, int, int, const void * );

static void ( APIENTRY * dllAlphaFunc )(GLenum func, GLclampf ref);
static void ( APIENTRY * dllBegin )(GLenum mode);
static void ( APIENTRY * dllBindTexture )(GLenum target, GLuint texture);
static void ( APIENTRY * dllBlendFunc )(GLenum sfactor, GLenum dfactor);
static void ( APIENTRY * dllClear )(GLbitfield mask);
static void ( APIENTRY * dllClearColor )(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
static void ( APIENTRY * dllClearDepth )(GLclampd depth);
static void ( APIENTRY * dllClearStencil )(GLint s);
static void ( APIENTRY * dllColorMask )(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha);
static void ( APIENTRY * dllColorPointer )(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
static void ( APIENTRY * dllCullFace )(GLenum mode);
static void ( APIENTRY * dllDeleteTextures )(GLsizei n, const GLuint *textures);
static void ( APIENTRY * dllDepthFunc )(GLenum func);
static void ( APIENTRY * dllDepthMask )(GLboolean flag);
static void ( APIENTRY * dllDepthRange )(GLclampd zNear, GLclampd zFar);
static void ( APIENTRY * dllDisable )(GLenum cap);
static void ( APIENTRY * dllDrawArrays )(GLenum mode, GLint first, GLsizei count);
static void ( APIENTRY * dllDrawElements )(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices);
static void ( APIENTRY * dllEnable )(GLenum cap);
static void ( APIENTRY * dllEnd )(void);
static void ( APIENTRY * dllFinish )(void);
static void ( APIENTRY * dllFlush )(void);
static void ( APIENTRY * dllFrontFace )(GLenum mode);
static void ( APIENTRY * dllFrustum )(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
static void ( APIENTRY * dllGenTextures )(GLsizei n, GLuint *textures);
static void ( APIENTRY * dllGetBooleanv )(GLenum pname, GLboolean *params);
GLenum ( APIENTRY * dllGetError )(void);
static void ( APIENTRY * dllGetFloatv )(GLenum pname, GLfloat *params);
static void ( APIENTRY * dllGetIntegerv )(GLenum pname, GLint *params);
const GLubyte * ( APIENTRY * dllGetString )(GLenum name);
static void ( APIENTRY * dllHint )(GLenum target, GLenum mode);
GLboolean ( APIENTRY * dllIsEnabled )(GLenum cap);
GLboolean ( APIENTRY * dllIsTexture )(GLuint texture);
static void ( APIENTRY * dllLineWidth )(GLfloat width);
static void ( APIENTRY * dllLoadIdentity )(void);
static void ( APIENTRY * dllLoadMatrixf )(const GLfloat *m);
static void ( APIENTRY * dllMatrixMode )(GLenum mode);
static void ( APIENTRY * dllMultMatrixf )(const GLfloat *m);
static void ( APIENTRY * dllOrtho )(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
static void ( APIENTRY * dllPointSize )(GLfloat size);
static void ( APIENTRY * dllPolygonMode )(GLenum face, GLenum mode);
static void ( APIENTRY * dllPolygonOffset )(GLfloat factor, GLfloat units);
static void ( APIENTRY * dllPopMatrix )(void);
static void ( APIENTRY * dllPushMatrix )(void);
static void ( APIENTRY * dllReadPixels )(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels);
GLint ( APIENTRY * dllRenderMode )(GLenum mode);
static void ( APIENTRY * dllRotated )(GLdouble angle, GLdouble x, GLdouble y, GLdouble z);
static void ( APIENTRY * dllRotatef )(GLfloat angle, GLfloat x, GLfloat y, GLfloat z);
static void ( APIENTRY * dllScaled )(GLdouble x, GLdouble y, GLdouble z);
static void ( APIENTRY * dllScalef )(GLfloat x, GLfloat y, GLfloat z);
static void ( APIENTRY * dllScissor )(GLint x, GLint y, GLsizei width, GLsizei height);
static void ( APIENTRY * dllShadeModel )(GLenum mode);
static void ( APIENTRY * dllStencilFunc )(GLenum func, GLint ref, GLuint mask);
static void ( APIENTRY * dllStencilMask )(GLuint mask);
static void ( APIENTRY * dllStencilOp )(GLenum fail, GLenum zfail, GLenum zpass);
static void ( APIENTRY * dllTexCoordPointer )(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
static void ( APIENTRY * dllTexEnvf )(GLenum target, GLenum pname, GLfloat param);
static void ( APIENTRY * dllTexEnvi )(GLenum target, GLenum pname, GLint param);
static void ( APIENTRY * dllTexImage2D )(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
static void ( APIENTRY * dllTexParameterf )(GLenum target, GLenum pname, GLfloat param);
static void ( APIENTRY * dllTexParameteri )(GLenum target, GLenum pname, GLint param);
static void ( APIENTRY * dllTexSubImage2D )(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);
static void ( APIENTRY * dllTranslatef )(GLfloat x, GLfloat y, GLfloat z);
static void ( APIENTRY * dllVertexPointer )(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
static void ( APIENTRY * dllViewport )(GLint x, GLint y, GLsizei width, GLsizei height);


/*
** QGL_Shutdown
**
** Unloads the specified DLL then nulls out all the proc pointers.
*/
void QGL_Shutdown( void )
{
	qglAlphaFunc                 = NULL;
	qglBegin                     = NULL;
	qglBindTexture               = NULL;
	qglBlendFunc                 = NULL;
	qglClear                     = NULL;
	qglClearColor                = NULL;
	qglClearDepth                = NULL;
	qglClearStencil              = NULL;
	qglColorMask                 = NULL;
	qglColorPointer              = NULL;
	qglCullFace                  = NULL;
	qglDeleteTextures            = NULL;
	qglDepthFunc                 = NULL;
	qglDepthMask                 = NULL;
	qglDepthRange                = NULL;
	qglDisable                   = NULL;
	qglDrawArrays                = NULL;
	qglDrawElements              = NULL;
	qglEnable                    = NULL;
	qglEnd                       = NULL;
	qglFinish                    = NULL;
	qglFlush                     = NULL;
	qglFrontFace                 = NULL;
	qglFrustum                   = NULL;
	qglGenTextures               = NULL;
	qglGetBooleanv               = NULL;
	qglGetError                  = NULL;
	qglGetFloatv                 = NULL;
	qglGetIntegerv               = NULL;
	qglGetString                 = NULL;
	qglHint                      = NULL;
	qglIsEnabled                 = NULL;
	qglIsTexture                 = NULL;
	qglLineWidth                 = NULL;
	qglLoadIdentity              = NULL;
	qglLoadMatrixf               = NULL;
	qglMatrixMode                = NULL;
	qglMultMatrixf               = NULL;
	qglOrtho                     = NULL;
	qglPointSize                 = NULL;
	qglPolygonMode               = NULL;
	qglPolygonOffset             = NULL;
	qglPopMatrix                 = NULL;
	qglPushMatrix                = NULL;
	qglReadPixels                = NULL;
	qglRotatef                   = NULL;
	qglScalef                    = NULL;
	qglScissor                   = NULL;
	qglShadeModel                = NULL;
	qglStencilFunc               = NULL;
	qglStencilMask               = NULL;
	qglStencilOp                 = NULL;
	qglTexCoordPointer           = NULL;
	qglTexEnvf                   = NULL;
	qglTexEnvi                   = NULL;
	qglTexImage2D                = NULL;
	qglTexParameterf             = NULL;
	qglTexParameteri             = NULL;
	qglTexSubImage2D             = NULL;
	qglTranslatef                = NULL;
	qglVertexPointer             = NULL;
	qglViewport                  = NULL;
}

/*
** QGL_Init
**
** This is responsible for binding our qgl function pointers to 
** the appropriate GL stuff.  In Windows this means doing a 
** LoadLibrary and a bunch of calls to GetProcAddress.  On other
** operating systems we need to do the right thing, whatever that
** might be.
** 
*/
qboolean QGL_Init( const char *dllname )
{
	qglState = (QGLState *) malloc(sizeof(QGLState));
	// qglState = new QGLState;   ... oh shit, not C++. sigh ...
	memset(qglState, 0, sizeof(QGLState));
	qglState->numVertices = 1024;
	qglState->vertices = (Vertex *) malloc(qglState->numVertices * sizeof(Vertex));
	memset(qglState->vertices, 0, qglState->numVertices * sizeof(Vertex));

	qglAlphaFunc                 = dllAlphaFunc = glAlphaFunc;
	qglBegin                     = dllBegin = glBegin;
	qglBindTexture               = dllBindTexture = glBindTexture;
	qglBlendFunc                 = dllBlendFunc = glBlendFunc;
	qglClear                     = dllClear = glClear;
	qglClearColor                = dllClearColor = glClearColor;
	qglClearDepth                = dllClearDepth = glClearDepth;
	qglClearStencil              = dllClearStencil = glClearStencil;
	qglColorMask                 = dllColorMask = glColorMask;
	qglColorPointer              = dllColorPointer = glColorPointer;
	qglCullFace                  = dllCullFace = glCullFace;
	qglDeleteTextures            = dllDeleteTextures = glDeleteTextures;
	qglDepthFunc                 = dllDepthFunc = glDepthFunc;
	qglDepthMask                 = dllDepthMask = glDepthMask;
	qglDepthRange                = dllDepthRange = glDepthRange;
	qglDisable                   = dllDisable = glDisable;
	qglDrawArrays                = dllDrawArrays = glDrawArrays;
	qglDrawElements              = dllDrawElements = glDrawElements;
	qglEnable                    = 	dllEnable                    = glEnable;
	qglEnd                       = 	dllEnd                       = glEnd;
	qglFinish                    = 	dllFinish                    = glFinish;
	qglFlush                     = 	dllFlush                     = glFlush;
	qglFrontFace                 = 	dllFrontFace                 = glFrontFace;
	qglFrustum                   = 	dllFrustum                   = glFrustum;
	qglGenTextures               = 	dllGenTextures               = glGenTextures;
	qglGetBooleanv               = 	dllGetBooleanv               = glGetBooleanv;
	qglGetError                  = 	dllGetError                  = glGetError;
	qglGetFloatv                 = 	dllGetFloatv                 = glGetFloatv;
	qglGetIntegerv               = 	dllGetIntegerv               = glGetIntegerv;
	qglGetString                 = 	dllGetString                 = glGetString;
	qglHint                      = 	dllHint                      = glHint;
	qglIsEnabled                 = 	dllIsEnabled                 = glIsEnabled;
	qglIsTexture                 = 	dllIsTexture                 = glIsTexture;
	qglLineWidth                 = 	dllLineWidth                 = glLineWidth;
	qglLoadIdentity              = 	dllLoadIdentity              = glLoadIdentity;
	qglLoadMatrixf               = 	dllLoadMatrixf               = glLoadMatrixf;
	qglMatrixMode                = 	dllMatrixMode                = glMatrixMode;
	qglMultMatrixf               = 	dllMultMatrixf               = glMultMatrixf;
	qglOrtho                     = 	dllOrtho                     = glOrtho;
	qglPointSize                 = 	dllPointSize                 = glPointSize;
	qglPolygonMode               = 	dllPolygonMode               = glPolygonMode;
	qglPolygonOffset             = 	dllPolygonOffset             = glPolygonOffset;
	qglPopMatrix                 = 	dllPopMatrix                 = glPopMatrix;
	qglPushMatrix                = 	dllPushMatrix                = glPushMatrix;
	qglReadPixels                = 	dllReadPixels                = glReadPixels;
	qglRotatef                   = 	dllRotatef                   = glRotatef;
	qglScalef                    = 	dllScalef                    = glScalef;
	qglScissor                   = 	dllScissor                   = glScissor;
	qglShadeModel                = 	dllShadeModel                = glShadeModel;
	qglStencilFunc               = 	dllStencilFunc               = glStencilFunc;
	qglStencilMask               = 	dllStencilMask               = glStencilMask;
	qglStencilOp                 = 	dllStencilOp                 = glStencilOp;
	qglTexCoordPointer           = 	dllTexCoordPointer           = glTexCoordPointer;
	qglTexEnvf                   = 	dllTexEnvf                   = glTexEnvf;
	qglTexEnvi                   = 	dllTexEnvi                   = glTexEnvi;
	qglTexImage2D                = 	dllTexImage2D                = glTexImage2D;
	qglTexParameterf             = 	dllTexParameterf             = glTexParameterf;
	qglTexParameteri             = 	dllTexParameteri             = glTexParameteri;
	qglTexSubImage2D             = 	dllTexSubImage2D             = glTexSubImage2D;
	qglTranslatef                = 	dllTranslatef                = glTranslatef;
	qglVertexPointer             = 	dllVertexPointer             = glVertexPointer;
	qglViewport                  = 	dllViewport                  = glViewport;

	qglPointParameterfEXT = NULL;
	qglPointParameterfvEXT = NULL;
	qglColorTableEXT = NULL;

	return true;
}

void GLimp_EnableLogging( qboolean enable )
{
		qglAlphaFunc                 = dllAlphaFunc;
		qglBegin                     = dllBegin;
		qglBindTexture               = dllBindTexture;
		qglBlendFunc                 = dllBlendFunc;
		qglClear                     = dllClear;
		qglClearColor                = dllClearColor;
		qglClearDepth                = dllClearDepth;
		qglClearStencil              = dllClearStencil;
		qglColorMask                 = dllColorMask;
		qglColorPointer              = dllColorPointer;
		qglCullFace                  = dllCullFace;
		qglDeleteTextures            = dllDeleteTextures ;
		qglDepthFunc                 = dllDepthFunc ;
		qglDepthMask                 = dllDepthMask ;
		qglDepthRange                = dllDepthRange ;
		qglDisable                   = dllDisable ;
		qglDrawArrays                = dllDrawArrays ;
		qglDrawElements              = dllDrawElements ;
		qglEnable                    = 	dllEnable                    ;
		qglEnd                       = 	dllEnd                       ;
		qglFinish                    = 	dllFinish                    ;
		qglFlush                     = 	dllFlush                     ;
		qglFrontFace                 = 	dllFrontFace                 ;
		qglFrustum                   = 	dllFrustum                   ;
		qglGenTextures               = 	dllGenTextures               ;
		qglGetBooleanv               = 	dllGetBooleanv               ;
		qglGetError                  = 	dllGetError                  ;
		qglGetFloatv                 = 	dllGetFloatv                 ;
		qglGetIntegerv               = 	dllGetIntegerv               ;
		qglGetString                 = 	dllGetString                 ;
		qglHint                      = 	dllHint                      ;
		qglIsEnabled                 = 	dllIsEnabled                 ;
		qglIsTexture                 = 	dllIsTexture                 ;
		qglLineWidth                 = 	dllLineWidth                 ;
		qglLoadIdentity              = 	dllLoadIdentity              ;
		qglLoadMatrixf               = 	dllLoadMatrixf               ;
		qglMatrixMode                = 	dllMatrixMode                ;
		qglMultMatrixf               = 	dllMultMatrixf               ;
		qglOrtho                     = 	dllOrtho                     ;
		qglPointSize                 = 	dllPointSize                 ;
		qglPolygonMode               = 	dllPolygonMode               ;
		qglPolygonOffset             = 	dllPolygonOffset             ;
		qglPopMatrix                 = 	dllPopMatrix                 ;
		qglPushMatrix                = 	dllPushMatrix                ;
		qglReadPixels                = 	dllReadPixels                ;
		qglRotatef                   = 	dllRotatef                   ;
		qglScalef                    = 	dllScalef                    ;
		qglScissor                   = 	dllScissor                   ;
		qglShadeModel                = 	dllShadeModel                ;
		qglStencilFunc               = 	dllStencilFunc               ;
		qglStencilMask               = 	dllStencilMask               ;
		qglStencilOp                 = 	dllStencilOp                 ;
		qglTexCoordPointer           = 	dllTexCoordPointer           ;
		qglTexEnvf                   = 	dllTexEnvf                   ;
		qglTexEnvi                   = 	dllTexEnvi                   ;
		qglTexImage2D                = 	dllTexImage2D                ;
		qglTexParameterf             = 	dllTexParameterf             ;
		qglTexParameteri             = 	dllTexParameteri             ;
		qglTexSubImage2D             = 	dllTexSubImage2D             ;
		qglTranslatef                = 	dllTranslatef                ;
		qglVertexPointer             = 	dllVertexPointer             ;
		qglViewport                  = 	dllViewport                  ;
}


// add a new vertex to vertices array
// resize if necessary
// return pointer to new vertex
static Vertex *pushVertex() {
	if (qglState->numVertices == qglState->usedVertices) {
		// resize needed
		size_t oldVerticesSize = qglState->numVertices * sizeof(Vertex);

		qglState->numVertices *= 2;
		Vertex *newVertices = (Vertex *) malloc(qglState->numVertices * sizeof(Vertex));
		memset(newVertices, 0, qglState->numVertices * sizeof(Vertex));
		memcpy(newVertices, qglState->vertices, oldVerticesSize);

		free(qglState->vertices);
		qglState->vertices = newVertices;
	}

	return qglState->vertices + (qglState->usedVertices++);
}


void qglColor3f(GLfloat red, GLfloat green, GLfloat blue) {
	qglColor4f(red, green, blue, 1.0f);
}


void qglColor4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) {
	uint32_t r = red * 255, g = green * 255, b = blue * 255, a = alpha * 255;

	// TODO: big-endian, if anyone cares
	uint32_t c =
		  (r      )
		| (g << 8 )
		| (b << 16)
		| (a << 24);
	qglState->currentVertex.color = c;

	glColor4f(red, green, blue, alpha);
}


void qglVertex2f(GLfloat x, GLfloat y) {
	qglVertex3f(x, y, 0.0f);
}


void qglVertex3f(GLfloat x, GLfloat y, GLfloat z) {
	glVertex3f(x, y, z);

	qglState->currentVertex.pos[0] = x;
	qglState->currentVertex.pos[1] = y;
	qglState->currentVertex.pos[2] = z;

	// pushVertex(qglState->currentVertex);
}


void qglMTexCoord2f(GLenum tex, GLfloat s, GLfloat t) {
	glMultiTexCoord2f(tex, s, t);

	if (tex == GL_TEXTURE0) {
		qglState->currentVertex.tex0[0] = s;
		qglState->currentVertex.tex0[1] = t;
	} else {
		qglState->currentVertex.tex1[0] = s;
		qglState->currentVertex.tex1[1] = t;
	}
}

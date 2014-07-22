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

#include <float.h>
#include <stdbool.h>
#include <assert.h>

#include <SDL.h>


typedef struct Vertex {
	float pos[3];
	uint32_t color;
	float tex0[2];
	float tex1[2];
} Vertex;


#define NUMMATRICES 32


typedef struct ShaderTexState {
	bool texEnable;
	GLenum texMode;
} ShaderTexState;


typedef struct ShaderState {
	bool alphaTest;
	GLenum alphaFunc;
	float alphaRef;

	ShaderTexState texState[2];
} ShaderState;


const char *vertexShaderSrc =
"attribute vec3 pos;\n"
"attribute vec4 col;\n"
"attribute vec2 texcoord0;\n"
"attribute vec2 texcoord1;\n"

"varying vec4 vColor;\n"
"varying vec2 vTex0;\n"
"varying vec2 vTex1;\n"

"uniform mat4 mvp;\n"

"void main(void) {\n"
"	vColor = col;\n"
"	vTex0 = texcoord0;\n"
"	vTex1 = texcoord1;\n"

"	gl_Position = mvp * vec4(pos, 1.0);\n"
"}\n"
;


const char *fragmentShaderSrc =
"uniform sampler2D tex0;\n"
"uniform sampler2D tex1;\n"

"precision highp float;\n"

"#ifdef ALPHA\n"
"uniform float alphaRef;\n"
"#endif  // ALPHA\n"

"varying vec4 vColor;\n"
"varying vec2 vTex0;\n"
"varying vec2 vTex1;\n"

"void main(void) {\n"
"	vec4 temp = vColor;\n"

"#ifdef TEX0\n"
"	vec4 t0col = texture2D(tex0, vTex0);\n"
"#ifdef TEX0_MODULATE\n"
"	temp = temp * t0col;\n"
"#elif TEX0_REPLACE\n"
"	temp = t0col;\n"
"#else\n"
"#error Unknown tex0 mode\n"
"#endif  // TEX0 mode\n"
"#endif  // TEX0\n"

"#ifdef TEX1\n"
"	vec4 t1col = texture2D(tex1, vTex1);\n"
"#ifdef TEX1_MODULATE\n"
"	temp = temp * t1col;\n"
"#elif TEX1_REPLACE\n"
"	temp = t1col;\n"
"#else\n"
"#error Unknown tex1 mode\n"
"#endif  // TEX1 mode\n"
"#endif  // TEX1\n"

"#ifdef ALPHA\n"
	// alpha function shows the fragment if test passes and discard if fails
	// so for GREATER we check <=
"	if (temp.a <= alphaRef) {\n"
"		discard;\n"
"	}\n"
"#endif  // ALPHA\n"

"	gl_FragColor = temp;\n"
"}\n"
;


struct Shader;

typedef struct Shader {
	ShaderState key;
	GLuint program;

	struct Shader *next;
} Shader;


typedef struct DrawCall {
	GLenum primitive;
	unsigned int firstVert;
	unsigned int numVertices;
} DrawCall;


#define NUMVBOS 1024


typedef struct QGLState {
	Vertex *vertices;
	unsigned int numVertices;
	unsigned int usedVertices;

	GLenum primitive;

	Vertex currentVertex;

	unsigned int clientActiveTexture;
	unsigned int wantActiveTexture, activeTexture;

	GLenum matrixMode;

	float mvMatrices[NUMMATRICES][16];
	float projMatrices[NUMMATRICES][16];

	int mvMatrixTop, projMatrixTop;
	bool mvMatrixDirty, projMatrixDirty;

	float zNear, zFar;

	GLuint vbos[NUMVBOS];

	// this is index into the array, not a VBO id
	unsigned int currentVBOidx;

	ShaderState wantShader;
	Shader *activeShader;

	Shader *shaders;

	DrawCall *drawCalls;
	unsigned int numDrawCalls, maxDrawCalls;

	unsigned int currentDrawFirstVertex;
} QGLState;


static QGLState *qglState = NULL;


void * qwglGetProcAddress(const char *procname)
{
	return SDL_GL_GetProcAddress(procname);
}

void ( APIENTRY * qglBlendFunc )(GLenum sfactor, GLenum dfactor);
void ( APIENTRY * qglClear )(GLbitfield mask);
void ( APIENTRY * qglClearColor )(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
void ( APIENTRY * qglClearDepth )(GLclampd depth);
void ( APIENTRY * qglClearStencil )(GLint s);
void ( APIENTRY * qglColorMask )(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha);
void ( APIENTRY * qglCullFace )(GLenum mode);
void ( APIENTRY * qglDeleteTextures )(GLsizei n, const GLuint *textures);
void ( APIENTRY * qglDepthFunc )(GLenum func);
void ( APIENTRY * qglDepthMask )(GLboolean flag);
void ( APIENTRY * qglDrawArrays )(GLenum mode, GLint first, GLsizei count);
void ( APIENTRY * qglDrawElements )(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices);
void ( APIENTRY * qglFrontFace )(GLenum mode);
void ( APIENTRY * qglGenTextures )(GLsizei n, GLuint *textures);
GLenum ( APIENTRY * qglGetError )(void);
void qglGetFloatv(GLenum pname, GLfloat *params);
const GLubyte * ( APIENTRY * qglGetString )(GLenum name);
void ( APIENTRY * qglHint )(GLenum target, GLenum mode);
GLboolean ( APIENTRY * qglIsEnabled )(GLenum cap);
GLboolean ( APIENTRY * qglIsTexture )(GLuint texture);
void ( APIENTRY * qglLineWidth )(GLfloat width);
void ( APIENTRY * qglPolygonOffset )(GLfloat factor, GLfloat units);
void ( APIENTRY * qglReadPixels )(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels);
void ( APIENTRY * qglScissor )(GLint x, GLint y, GLsizei width, GLsizei height);
void ( APIENTRY * qglStencilFunc )(GLenum func, GLint ref, GLuint mask);
void ( APIENTRY * qglStencilMask )(GLuint mask);
void ( APIENTRY * qglStencilOp )(GLenum fail, GLenum zfail, GLenum zpass);
void ( APIENTRY * qglTexImage2D )(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
void ( APIENTRY * qglTexParameterf )(GLenum target, GLenum pname, GLfloat param);
void ( APIENTRY * qglTexParameteri )(GLenum target, GLenum pname, GLint param);
void ( APIENTRY * qglTexSubImage2D )(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);
void ( APIENTRY * qglViewport )(GLint x, GLint y, GLsizei width, GLsizei height);


/*
** QGL_Shutdown
**
** Unloads the specified DLL then nulls out all the proc pointers.
*/
void QGL_Shutdown( void )
{
	if (qglState->vbos[0] != 0) {
		glDeleteBuffers(NUMVBOS, qglState->vbos);
		memset(qglState->vbos, 0, sizeof(GLuint) * NUMVBOS);
	}

	// this pointer is not unique
	qglState->activeShader = NULL;

	// these are
	while (qglState->shaders != NULL) {
		glDeleteProgram(qglState->shaders->program);
		Shader *old = qglState->shaders;
		qglState->shaders = old->next;
		free(old);
	}

	free(qglState); qglState = NULL;

	qglBlendFunc                 = NULL;
	qglClear                     = NULL;
	qglClearColor                = NULL;
	qglClearDepth                = NULL;
	qglClearStencil              = NULL;
	qglColorMask                 = NULL;
	qglCullFace                  = NULL;
	qglDeleteTextures            = NULL;
	qglDepthFunc                 = NULL;
	qglDepthMask                 = NULL;
	qglDrawArrays                = NULL;
	qglDrawElements              = NULL;
	qglFrontFace                 = NULL;
	qglGenTextures               = NULL;
	qglGetError                  = NULL;
	qglGetString                 = NULL;
	qglHint                      = NULL;
	qglIsEnabled                 = NULL;
	qglIsTexture                 = NULL;
	qglLineWidth                 = NULL;
	qglPolygonOffset             = NULL;
	qglReadPixels                = NULL;
	qglScissor                   = NULL;
	qglStencilFunc               = NULL;
	qglStencilMask               = NULL;
	qglStencilOp                 = NULL;
	qglTexImage2D                = NULL;
	qglTexParameterf             = NULL;
	qglTexParameteri             = NULL;
	qglTexSubImage2D             = NULL;
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

	qglState->maxDrawCalls = 128;
	qglState->drawCalls = (DrawCall *) malloc(qglState->maxDrawCalls * sizeof(DrawCall));

	qglState->zFar = 1.0f;

	qglBlendFunc                 = glBlendFunc;
	qglClear                     = glClear;
	qglClearColor                = glClearColor;
	qglClearDepth                = glClearDepth;
	qglClearStencil              = glClearStencil;
	qglColorMask                 = glColorMask;
	qglCullFace                  = glCullFace;
	qglDeleteTextures            = glDeleteTextures;
	qglDepthFunc                 = glDepthFunc;
	qglDepthMask                 = glDepthMask;
	qglDrawArrays                = glDrawArrays;
	qglDrawElements              = glDrawElements;
	qglFrontFace                 = glFrontFace;
	qglGenTextures               = glGenTextures;
	qglGetError                  = glGetError;
	qglGetString                 = glGetString;
	qglHint                      = glHint;
	qglIsEnabled                 = glIsEnabled;
	qglIsTexture                 = glIsTexture;
	qglLineWidth                 = glLineWidth;
	qglPolygonOffset             = glPolygonOffset;
	qglReadPixels                = glReadPixels;
	qglScissor                   = glScissor;
	qglStencilFunc               = glStencilFunc;
	qglStencilMask               = glStencilMask;
	qglStencilOp                 = glStencilOp;
	qglTexImage2D                = glTexImage2D;
	qglTexParameterf             = glTexParameterf;
	qglTexParameteri             = glTexParameteri;
	qglTexSubImage2D             = glTexSubImage2D;
	qglViewport                  = glViewport;

	return true;
}


// add a new vertex to vertices array
// resize if necessary
static void pushVertex(const Vertex *v) {
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

	memcpy(&qglState->vertices[qglState->usedVertices++], v, sizeof(Vertex));
}


void qglColor3f(GLfloat red, GLfloat green, GLfloat blue) {
	qglColor4f(red, green, blue, 1.0f);
}


#define CLAMP(x) if ((x) > 1.0f) { (x) = 1.0f; } else if ((x) < 0.0f) { (x) = 0.0f; }


void qglColor4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) {
	CLAMP(red)
	CLAMP(green)
	CLAMP(blue)
	CLAMP(alpha)

	uint32_t r = red * 255, g = green * 255, b = blue * 255, a = alpha * 255;

	// TODO: big-endian, if anyone cares
	uint32_t c =
		  (r <<  0)
		| (g <<  8)
		| (b << 16)
		| (a << 24);
	qglState->currentVertex.color = c;
}


#undef CLAMP


void qglVertex2f(GLfloat x, GLfloat y) {
	qglVertex3f(x, y, 0.0f);
}


void qglVertex3f(GLfloat x, GLfloat y, GLfloat z) {
	qglState->currentVertex.pos[0] = x;
	qglState->currentVertex.pos[1] = y;
	qglState->currentVertex.pos[2] = z;

	pushVertex(&qglState->currentVertex);
}


void qglMTexCoord2f(GLenum tex, GLfloat s, GLfloat t) {
	if (tex == GL_TEXTURE0) {
		qglState->currentVertex.tex0[0] = s;
		qglState->currentVertex.tex0[1] = t;
	} else {
		qglState->currentVertex.tex1[0] = s;
		qglState->currentVertex.tex1[1] = t;
	}
}


static void pushDraw(GLenum primitive, unsigned int firstVert, unsigned int numVertices) {
	if (qglState->numDrawCalls == qglState->maxDrawCalls) {
		// add more space
		qglState->maxDrawCalls *= 2;
		qglState->drawCalls = realloc(qglState->drawCalls, qglState->maxDrawCalls * sizeof(DrawCall));
	}

	qglState->drawCalls[qglState->numDrawCalls].primitive = primitive;
	qglState->drawCalls[qglState->numDrawCalls].firstVert = firstVert;
	qglState->drawCalls[qglState->numDrawCalls].numVertices = numVertices;

	qglState->numDrawCalls++;
}


static void commitShaderState();


static void bindVBO() {
	if (qglState->vbos[qglState->currentVBOidx] == 0) {
		// can't be called in QGL_Init, GL context doesn't exist there
		glGenBuffers(NUMVBOS, qglState->vbos);
	}

	qglState->currentVBOidx = (qglState->currentVBOidx + 1) % NUMVBOS;

	glBindBuffer(GL_ARRAY_BUFFER, qglState->vbos[qglState->currentVBOidx]);
}


static void flushDraws() {
	if (qglState->numDrawCalls == 0) {
		// nothing to do
		return;
	}

	commitShaderState();

	bindVBO();

	glBufferData(GL_ARRAY_BUFFER, qglState->usedVertices * sizeof(Vertex), &qglState->vertices[0], GL_DYNAMIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const GLvoid *) offsetof(Vertex, pos));
	glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Vertex), (const GLvoid *) offsetof(Vertex, color));
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const GLvoid *) offsetof(Vertex, tex0));
	glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const GLvoid *) offsetof(Vertex, tex1));

	for (unsigned int i = 0; i < qglState->numDrawCalls; i++) {
		DrawCall *d = qglState->drawCalls + i;
		glDrawArrays(d->primitive, d->firstVert, d->numVertices);
	}

	qglState->numDrawCalls = 0;
	qglState->usedVertices = 0;
}


void qglBegin(GLenum mode) {
	qglState->currentDrawFirstVertex = qglState->usedVertices;
	qglState->primitive = mode;
}


static Shader *createShader(const ShaderState *state) {
#define BUFSIZE 512
#define EMITDEF(x) defSize += snprintf(defineBuf + defSize, BUFSIZE - defSize, "#define %s 1\n", x)
	char defineBuf[BUFSIZE];
	unsigned int defSize = 0;
	memset(defineBuf, '\0', BUFSIZE);

	if (state->alphaTest) {
		EMITDEF("ALPHA");

		// the only one currently used
		assert(state->alphaFunc == GL_GREATER);
	}

	if (state->texState[0].texEnable) {
		EMITDEF("TEX0");

		switch(state->texState[0].texMode) {
		case GL_MODULATE:
			EMITDEF("TEX0_MODULATE");
			break;

		case GL_REPLACE:
			EMITDEF("TEX0_REPLACE");
			break;

		default:
			assert(false);
			break;
		}
	}

	if (state->texState[1].texEnable) {
		EMITDEF("TEX1");

		switch(state->texState[1].texMode) {
		case GL_MODULATE:
			EMITDEF("TEX1_MODULATE");
			break;

		case GL_REPLACE:
			EMITDEF("TEX1_REPLACE");
			break;

		default:
			assert(false);
			break;
		}
	}

	const char *srcArray[3];
	srcArray[0] = "#version 100\n";
	srcArray[1] = (const char *) &defineBuf;
	GLuint program = glCreateProgram();
	GLint temp = 0;

	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	srcArray[2] = vertexShaderSrc;
	glShaderSource(vertexShader, 3, srcArray, NULL);
	glCompileShader(vertexShader);
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &temp);
	if (temp != GL_TRUE) {
		printf("vertex shader compile failed:\n");
		glGetShaderiv(vertexShader, GL_INFO_LOG_LENGTH, &temp);
		char buf[temp + 1];
		memset(buf, '\0', temp + 1);
		glGetShaderInfoLog(vertexShader, temp, NULL, buf);

		printf("%s\n", buf);
		abort();
	}
	glAttachShader(program, vertexShader);
	glDeleteShader(vertexShader);

	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	srcArray[2] = fragmentShaderSrc;
	glShaderSource(fragmentShader, 3, srcArray, NULL);
	glCompileShader(fragmentShader);
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &temp);
	if (temp != GL_TRUE) {
		printf("fragment shader compile failed:\n");
		glGetShaderiv(fragmentShader, GL_INFO_LOG_LENGTH, &temp);
		char buf[temp + 1];
		memset(buf, '\0', temp + 1);
		glGetShaderInfoLog(fragmentShader, temp, NULL, buf);

		printf("%s\n", buf);
		abort();
	}
	glAttachShader(program, fragmentShader);
	glDeleteShader(fragmentShader);

	glBindAttribLocation(program, 0, "pos");
	glBindAttribLocation(program, 1, "col");
	glBindAttribLocation(program, 2, "texcoord0");
	glBindAttribLocation(program, 3, "texcoord1");

	glLinkProgram(program);
	glUseProgram(program);
	glUniform1i(glGetUniformLocation(program, "tex0"), 0);
	glUniform1i(glGetUniformLocation(program, "tex1"), 1);

	Shader *shader = (Shader *) malloc(sizeof(Shader));
	shader->program = program;
	memcpy(&shader->key, state, sizeof(ShaderState));
	shader->next = NULL;

	return shader;
}


static Shader *findShader(const ShaderState *state) {
	Shader *shader = qglState->shaders;
	while (shader != NULL) {
		if (memcmp(&shader->key, state, sizeof(ShaderState)) == 0) {
			return shader;
		}

		shader = shader->next;
	}

	// not found, create
	shader = createShader(state);
	shader->next = qglState->shaders;
	qglState->shaders = shader;

	return shader;

}


static void identityMatrix(float *matrix) {
	memset(matrix, 0, sizeof(float) * 16);
	matrix[0 * 4 + 0] = 1.0f;
	matrix[1 * 4 + 1] = 1.0f;
	matrix[2 * 4 + 2] = 1.0f;
	matrix[3 * 4 + 3] = 1.0f;
}


static void multMatrices(float *target, const float *left, const float *right) {
	// target and left/right must not alias
	// should probably put some __restrict__ on this
	assert(target != left);
	assert(target != right);

	target[0 * 4 + 0] = right[0 * 4 + 0] * left[0 * 4 + 0] + right[0 * 4 + 1] * left[1 * 4 + 0] + right[0 * 4 + 2] * left[2 * 4 + 0] + right[0 * 4 + 3] * left[3 * 4 + 0];
	target[0 * 4 + 1] = right[0 * 4 + 0] * left[0 * 4 + 1] + right[0 * 4 + 1] * left[1 * 4 + 1] + right[0 * 4 + 2] * left[2 * 4 + 1] + right[0 * 4 + 3] * left[3 * 4 + 1];
	target[0 * 4 + 2] = right[0 * 4 + 0] * left[0 * 4 + 2] + right[0 * 4 + 1] * left[1 * 4 + 2] + right[0 * 4 + 2] * left[2 * 4 + 2] + right[0 * 4 + 3] * left[3 * 4 + 2];
	target[0 * 4 + 3] = right[0 * 4 + 0] * left[0 * 4 + 3] + right[0 * 4 + 1] * left[1 * 4 + 3] + right[0 * 4 + 2] * left[2 * 4 + 3] + right[0 * 4 + 3] * left[3 * 4 + 3];

	target[1 * 4 + 0] = right[1 * 4 + 0] * left[0 * 4 + 0] + right[1 * 4 + 1] * left[1 * 4 + 0] + right[1 * 4 + 2] * left[2 * 4 + 0] + right[1 * 4 + 3] * left[3 * 4 + 0];
	target[1 * 4 + 1] = right[1 * 4 + 0] * left[0 * 4 + 1] + right[1 * 4 + 1] * left[1 * 4 + 1] + right[1 * 4 + 2] * left[2 * 4 + 1] + right[1 * 4 + 3] * left[3 * 4 + 1];
	target[1 * 4 + 2] = right[1 * 4 + 0] * left[0 * 4 + 2] + right[1 * 4 + 1] * left[1 * 4 + 2] + right[1 * 4 + 2] * left[2 * 4 + 2] + right[1 * 4 + 3] * left[3 * 4 + 2];
	target[1 * 4 + 3] = right[1 * 4 + 0] * left[0 * 4 + 3] + right[1 * 4 + 1] * left[1 * 4 + 3] + right[1 * 4 + 2] * left[2 * 4 + 3] + right[1 * 4 + 3] * left[3 * 4 + 3];
	
	target[2 * 4 + 0] = right[2 * 4 + 0] * left[0 * 4 + 0] + right[2 * 4 + 1] * left[1 * 4 + 0] + right[2 * 4 + 2] * left[2 * 4 + 0] + right[2 * 4 + 3] * left[3 * 4 + 0];
	target[2 * 4 + 1] = right[2 * 4 + 0] * left[0 * 4 + 1] + right[2 * 4 + 1] * left[1 * 4 + 1] + right[2 * 4 + 2] * left[2 * 4 + 1] + right[2 * 4 + 3] * left[3 * 4 + 1];
	target[2 * 4 + 2] = right[2 * 4 + 0] * left[0 * 4 + 2] + right[2 * 4 + 1] * left[1 * 4 + 2] + right[2 * 4 + 2] * left[2 * 4 + 2] + right[2 * 4 + 3] * left[3 * 4 + 2];
	target[2 * 4 + 3] = right[2 * 4 + 0] * left[0 * 4 + 3] + right[2 * 4 + 1] * left[1 * 4 + 3] + right[2 * 4 + 2] * left[2 * 4 + 3] + right[2 * 4 + 3] * left[3 * 4 + 3];

	target[3 * 4 + 0] = right[3 * 4 + 0] * left[0 * 4 + 0] + right[3 * 4 + 1] * left[1 * 4 + 0] + right[3 * 4 + 2] * left[2 * 4 + 0] + right[3 * 4 + 3] * left[3 * 4 + 0];
	target[3 * 4 + 1] = right[3 * 4 + 0] * left[0 * 4 + 1] + right[3 * 4 + 1] * left[1 * 4 + 1] + right[3 * 4 + 2] * left[2 * 4 + 1] + right[3 * 4 + 3] * left[3 * 4 + 1];
	target[3 * 4 + 2] = right[3 * 4 + 0] * left[0 * 4 + 2] + right[3 * 4 + 1] * left[1 * 4 + 2] + right[3 * 4 + 2] * left[2 * 4 + 2] + right[3 * 4 + 3] * left[3 * 4 + 2];
	target[3 * 4 + 3] = right[3 * 4 + 0] * left[0 * 4 + 3] + right[3 * 4 + 1] * left[1 * 4 + 3] + right[3 * 4 + 2] * left[2 * 4 + 3] + right[3 * 4 + 3] * left[3 * 4 + 3];
}


static void commitShaderState() {
	Shader *newShader = findShader(&qglState->wantShader);
	bool shaderChanged = false;
	if (qglState->activeShader != newShader) {
		qglState->activeShader = newShader;
		glUseProgram(qglState->activeShader->program);
		shaderChanged = true;
	}

	if (shaderChanged || qglState->mvMatrixDirty || qglState->projMatrixDirty) {
		float mvp[16];

		float depthAdjust[16];
		identityMatrix(depthAdjust);
		depthAdjust[2 * 4 + 2] = qglState->zFar - qglState->zNear;
		depthAdjust[3 * 4 + 2] = qglState->zNear;

		float temp[16];
		multMatrices(temp, depthAdjust, qglState->projMatrices[qglState->projMatrixTop]);
		multMatrices(mvp, temp, qglState->mvMatrices[qglState->mvMatrixTop]);

		glUniformMatrix4fv(glGetUniformLocation(qglState->activeShader->program, "mvp"), 1, GL_FALSE, &mvp[0]);
		qglState->mvMatrixDirty = false;
		qglState->projMatrixDirty = false;
	}

	if (qglState->wantShader.alphaTest) {
		glUniform1f(glGetUniformLocation(newShader->program, "alphaRef"), qglState->wantShader.alphaRef);
	}
}


void qglEnd(void) {
	pushDraw(qglState->primitive, qglState->currentDrawFirstVertex, qglState->usedVertices - qglState->currentDrawFirstVertex);

	qglState->primitive = GL_NONE;
}


void qglMatrixMode(GLenum mode) {
	qglState->matrixMode = mode;
}


void qglLoadIdentity(void) {
	float idM[16];
	identityMatrix(idM);

	qglLoadMatrixf(idM);
}


void qglFrustum(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar) {
	float mat[16];
	memset(mat, 0, sizeof(float) * 16);

	mat[0 * 4 + 0] = 2 * zNear / (right - left);

	mat[1 * 4 + 1] = 2 * zNear / (top - bottom);

	mat[2 * 4 + 0] = (right + left) / (right - left);
	mat[2 * 4 + 1] = (top + bottom) / (top - bottom);
	mat[2 * 4 + 2] = -(zFar + zNear) / (zFar - zNear);
	mat[2 * 4 + 3] = -1.0f;

	mat[3 * 4 + 2] = -(2 * zFar * zNear) / (zFar - zNear);

	// should really be MultMatrix but since we always load identity before...
	qglLoadMatrixf(mat);
}


void qglOrtho(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar) {
	float mat[16];
	memset(mat, 0, sizeof(float) * 16);

	mat[0 * 4 + 0] = 2  / (right - left);

	mat[1 * 4 + 1] = 2  / (top - bottom);

	mat[2 * 4 + 2] = -2 / (zFar - zNear);

	mat[3 * 4 + 0] = -(right + left) / (right - left);
	mat[3 * 4 + 1] = -(top + bottom) / (top - bottom);
	mat[3 * 4 + 2] = -(zFar + zNear) / (zFar - zNear);
	mat[3 * 4 + 3] = 1.0f;

	// should really be MultMatrix but since we always load identity before...
	qglLoadMatrixf(mat);
}


void qglLoadMatrixf(const GLfloat *m) {
	flushDraws();

	float *targetMat = NULL;
	if (qglState->matrixMode == GL_MODELVIEW) {
		targetMat = qglState->mvMatrices[qglState->mvMatrixTop];
		qglState->mvMatrixDirty = true;
	} else if (qglState->matrixMode == GL_PROJECTION) {
		targetMat = qglState->projMatrices[qglState->projMatrixTop];
		qglState->projMatrixDirty = true;
	} else {
		assert(false);
	}

	memcpy(targetMat, m, sizeof(float) * 16);
}


void qglMultMatrixf(const GLfloat *m) {
	flushDraws();

	float *targetMat = NULL;
	if (qglState->matrixMode == GL_MODELVIEW) {
		targetMat = qglState->mvMatrices[qglState->mvMatrixTop];
		qglState->mvMatrixDirty = true;
	} else if (qglState->matrixMode == GL_PROJECTION) {
		targetMat = qglState->projMatrices[qglState->projMatrixTop];
		qglState->projMatrixDirty = true;
	} else {
		assert(false);
	}

	float mat[16];
	multMatrices(mat, targetMat, m);
	memcpy(targetMat, mat, sizeof(float) * 16);
}


void qglRotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z) {
	float mat[16];
	memset(mat, 0, sizeof(float) * 16);

	// someone calls this with zero vector
	// add FLT_MIN to prevent division by zero
	float norm = 1.0f / (sqrtf(x*x + y*y + z*z) + FLT_MIN);
	x *= norm;
	y *= norm;
	z *= norm;

	float c = cosf(angle * M_PI / 180.0f);
	float s = sinf(angle * M_PI / 180.0f);

	mat[0 * 4 + 0] = x * x * (1 - c) + c;
	mat[0 * 4 + 1] = y * x * (1 - c) + z * s;
	mat[0 * 4 + 2] = x * z * (1 - c) - y * s;

	mat[1 * 4 + 0] = x * y * (1 - c) - z * s;
	mat[1 * 4 + 1] = y * y * (1 - c) + c;
	mat[1 * 4 + 2] = y * z * (1 - c) + x * s;

	mat[2 * 4 + 0] = x * z * (1 - c) + y * s;
	mat[2 * 4 + 1] = y * z * (1 - c) - x * s;
	mat[2 * 4 + 2] = z * z * (1 - c) + c;

	mat[3 * 4 + 3] = 1.0f;

	qglMultMatrixf(mat);
}


void qglScalef(GLfloat x, GLfloat y, GLfloat z) {
	float mat[16];
	memset(mat, 0, sizeof(float) * 16);

	mat[0 * 4 + 0] = x;

	mat[1 * 4 + 1] = y;

	mat[2 * 4 + 2] = z;

	mat[3 * 4 + 3] = 1.0f;

	qglMultMatrixf(mat);
}


void qglTranslatef(GLfloat x, GLfloat y, GLfloat z) {
	float mat[16];
	identityMatrix(mat);

	mat[3 * 4 + 0] = x;
	mat[3 * 4 + 1] = y;
	mat[3 * 4 + 2] = z;

	qglMultMatrixf(mat);
}


void qglPopMatrix(void) {
	flushDraws();

	float *targetMat = NULL;
	if (qglState->matrixMode == GL_MODELVIEW) {
		qglState->mvMatrixTop--;

		assert(qglState->mvMatrixTop >= 0 && qglState->mvMatrixTop <= NUMMATRICES);
		targetMat = qglState->mvMatrices[qglState->mvMatrixTop];
		qglState->mvMatrixDirty = true;
	} else if (qglState->matrixMode == GL_PROJECTION) {
		qglState->projMatrixTop--;

		assert(qglState->projMatrixTop >= 0 && qglState->projMatrixTop <= NUMMATRICES);
		targetMat = qglState->projMatrices[qglState->projMatrixTop];
		qglState->projMatrixDirty = true;
	} else {
		assert(false);
	}
}


void qglPushMatrix(void) {
	if (qglState->matrixMode == GL_MODELVIEW) {
		qglState->mvMatrixTop++;

		assert(qglState->mvMatrixTop >= 0 && qglState->mvMatrixTop <= NUMMATRICES);

		memcpy(qglState->mvMatrices[qglState->mvMatrixTop], qglState->mvMatrices[qglState->mvMatrixTop - 1], sizeof(float) * 16);
	} else if (qglState->matrixMode == GL_PROJECTION) {
		qglState->projMatrixTop++;

		assert(qglState->projMatrixTop >= 0 && qglState->projMatrixTop <= NUMMATRICES);

		memcpy(qglState->projMatrices[qglState->projMatrixTop], qglState->projMatrices[qglState->projMatrixTop - 1], sizeof(float) * 16);
	} else {
		assert(false);
	}
}


void qglGetFloatv(GLenum pname, GLfloat *params) {
	assert(pname == GL_MODELVIEW_MATRIX);

	memcpy(params, qglState->mvMatrices[qglState->mvMatrixTop], sizeof(float) * 16);
}


void qglActiveTexture(GLenum tex) {
	qglState->wantActiveTexture = tex - GL_TEXTURE0;
}


void qglAlphaFunc(GLenum func, GLclampf ref) {
	flushDraws();

	qglState->wantShader.alphaFunc = func;
	qglState->wantShader.alphaRef = ref;
}


void qglDisable(GLenum cap) {
	flushDraws();

	if (cap == GL_ALPHA_TEST) {
		qglState->wantShader.alphaTest = false;
	} else if (cap == GL_TEXTURE_2D) {
		qglState->wantShader.texState[qglState->wantActiveTexture].texEnable = false;
	} else {
		glDisable(cap);
	}
}


void qglEnable(GLenum cap) {
	flushDraws();

	if (cap == GL_ALPHA_TEST) {
		qglState->wantShader.alphaTest = true;
	} else if (cap == GL_TEXTURE_2D) {
		qglState->wantShader.texState[qglState->wantActiveTexture].texEnable = true;
	} else {
		glEnable(cap);
	}
}


void qglTexEnvi(GLenum target, GLenum pname, GLint param) {
	assert(target == GL_TEXTURE_ENV);

	// we only use one combine mode
	if (pname == GL_TEXTURE_ENV_MODE) {
		flushDraws();

		assert(param == GL_COMBINE_ARB
			  || param == GL_MODULATE
			  || param == GL_REPLACE);
		qglState->wantShader.texState[qglState->wantActiveTexture].texMode = param;
	} else if (pname == GL_COMBINE_RGB_ARB) {
		assert(param == GL_MODULATE);
	} else if (pname == GL_COMBINE_ALPHA_ARB) {
		assert(param == GL_MODULATE);
	} else if (pname == GL_RGB_SCALE_ARB) {
		assert(param == 2);
	} else {
		assert(false);
	}
}


void qglBindTexture(GLenum target, GLuint texture) {
	flushDraws();

	if (qglState->activeTexture != qglState->wantActiveTexture) {
		glActiveTexture(GL_TEXTURE0);
		qglState->activeTexture = qglState->wantActiveTexture;
	}

	glBindTexture(target, texture);
}


void qglDepthRange(GLclampd zNear, GLclampd zFar) {
	qglState->zNear = zNear;
	qglState->zFar = zFar;
}

# example of local configuration
# copy to local.mk


# location of source
TOPDIR?=..


# default to debugging disabled
DEBUG?=n


USE_JPEG:=n
USE_GLEW:=y
USE_LIBWEBSOCKETS:=y
USE_OPENAL:=n
USE_PNG:=n

STATIC_SDL2:=y
PURE_CLIENT:=n

BUILD_SERVER:=y
BUILD_UTILS:=n

# compiler options etc
CXX:=msvgcc
CC:=msvgcc


# /GR[-] enable C++ RTTI
# /WX treat warnings as errors
# /TP treat all source as C++
# /W<n> set warning level (default n=1)
# /Wp64 enable 64 bit porting warnings  ERROR: msvc includes not clean for this!
CFLAGS:=/WX /TP
CFLAGS+=-I$(TOPDIR)
CFLAGS+=-D_CRT_SECURE_NO_DEPRECATE
CFLAGS+=-I$(TOPDIR)/foreign/SDL2/include
CFLAGS+=-DLWS_LIBRARY_VERSION=\"1.4\"


# lazy assignment because CFLAGS is changed later
CXXFLAGS=$(CFLAGS)
CXXFLAGS+=/EHsc

LDLIBS:=winmm.lib wsock32.lib
LDLIBS_client:=dsound.lib
LDLIBS_ref_gl:=opengl32.lib
LDLIBS_sdl2:=imm32.lib ole32.lib oleaut32.lib uuid.lib version.lib
LDLIBS_libwebsockets:=ws2_32.lib

LDFLAGS:=/nodefaultlib:libcmt.lib /subsystem:windows /WX
LDLIBS+=dsound.lib dxerr.lib gdi32.lib imm32.lib ole32.lib oleaut32.lib
LDLIBS+=user32.lib shell32.lib version.lib winmm.lib

OPTFLAGS:=

SOCFLAGS:=

OBJSUFFIX:=.obj
EXESUFFIX:=.exe
SOSUFFIX:=x86.dll


# the fucker requires different libs for debugging...
ifeq ($(DEBUG),y)

CFLAGS+=/MDd -D_DEBUG
# /RTC1 Enable fast checks (/RTCsu)
# /Z7 enable old-style debug info
OPTFLAGS+=/RTC1 /Z7
LDFLAGS+=/nodefaultlib:msvcrt.lib
LDLIBS+=msvcrtd.lib

else

CFLAGS+=/MD -DNDEBUG
# /O2 maximize speed
# /Oi enable intrinsic functions
# /GA optimize for Windows Application
# /GF enable read-only string pooling
# /fp:fast "fast" floating-point model; results are less predictable
# /arch:<SSE|SSE2>
OPTFLAGS+=/O2 /Oi /GA /GF /fp:fast /arch:SSE2
LDLIBS+=msvcrt.lib

endif

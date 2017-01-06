# example of local configuration
# copy to local.mk


# location of source
TOPDIR?=..


LTO:=n
ASAN:=n
TSAN:=n
UBSAN:=n


USE_JPEG:=n
USE_GLEW:=y
USE_LIBWEBSOCKETS:=y
USE_OPENAL:=n
USE_PNG:=n

STATIC_SDL2:=y
PURE_CLIENT:=n
RENDERER:=opengl

BUILD_SERVER:=y
BUILD_UTILS:=n

# compiler options etc
CC:=i686-w64-mingw32-gcc
CXX:=i686-w64-mingw32-g++
CFLAGS:=-gstabs -DNDEBUG -D__CRT__NO_INLINE=1 -mwindows
CFLAGS+=-w
CFLAGS+=-I$(TOPDIR)/foreign/SDL2/include
CFLAGS+=-DLWS_LIBRARY_VERSION=\"1.4\"
OPTFLAGS:=-O2 -mtune=generic -fno-strict-aliasing -ffloat-store


# lazy assignment because CFLAGS is changed later
CXXFLAGS=$(CFLAGS) -std=c++11 -fno-exceptions -fno-rtti


LDFLAGS:=-gstabs -mwindows -static-libstdc++ -static-libgcc
LDLIBS:=-lwinmm -lwsock32
LDLIBS_client:=-ldsound
LDLIBS_ref_gl:=-lopengl32
LDLIBS_sdl2:=-limm32 -lole32 -loleaut32 -luuid -lversion
LDLIBS_libwebsockets:=-lws2_32


SOCFLAGS:=


LTOCFLAGS:=-flto
LTOLDFLAGS:=-flto


OBJSUFFIX:=.o
EXESUFFIX:=.exe
SOSUFFIX:=x86.dll

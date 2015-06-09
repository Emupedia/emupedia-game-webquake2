# example of local configuration
# copy to local.mk


# location of source
TOPDIR:=..


LTO:=n
ASAN:=n
TSAN:=n
UBSAN:=n


USE_JPEG:=n
USE_GLEW:=y
USE_OPENAL:=n
USE_PNG:=n

BUILD_SERVER:=y

# compiler options etc
CC:=i686-w64-mingw32-gcc
CXX:=i686-w64-mingw32-g++
CFLAGS:=-g -DNDEBUG -D__CRT__NO_INLINE=1 -mwindows
CFLAGS+=-w
CFLAGS+=-I$(TOPDIR)/foreign/SDL2/include
OPTFLAGS:=-O2 -march=native -fno-strict-aliasing -ffloat-store


# lazy assignment because CFLAGS is changed later
CXXFLAGS=$(CFLAGS) -std=c++11 -fno-exceptions -fno-rtti


LDFLAGS:=-g -mwindows
LDFLAGS+=-L$(TOPDIR)/foreign/SDL2/lib
LDLIBS:=-lwinmm -lwsock32
LDLIBS_ref_gl:=-lopengl32 -lSDL2
LDLIBS_client:=-ldsound


SOCFLAGS:=


LTOCFLAGS:=-flto
LTOLDFLAGS:=-flto


OBJSUFFIX:=.o
EXESUFFIX:=.exe
SOSUFFIX:=x86.dll

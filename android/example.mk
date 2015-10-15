# example of local configuration
# copy to local.mk


# location of source
TOPDIR?=..


LTO:=n
ASAN:=n
TSAN:=n
UBSAN:=n
AFL:=n


USE_JPEG:=y
USE_GLEW:=n
USE_LIBWEBSOCKETS:=n
USE_OPENAL:=n
USE_PNG:=y

BUILD_SERVER:=n
BUILD_UTILS:=n
PURE_CLIENT:=n

STATIC_SDL2:=n

BUILTIN_GAME:=baseq2


# compiler options etc
CC:=arm-linux-androideabi-gcc
CXX:=arm-linux-androideabi-g++
CFLAGS:=-g -DNDEBUG -DLINUX
CFLAGS+=-w
CFLAGS+=-I$(TOPDIR)/foreign/SDL2/include
OPTFLAGS:=-O2 -fno-strict-aliasing


# lazy assignment because CFLAGS is changed later
CXXFLAGS=$(CFLAGS) -std=c++11 -fno-exceptions -fno-rtti


LDFLAGS:=-g
LDLIBS:=
LDLIBS_ref_gl:=
LDLIBS_client:=


SOCFLAGS:=-fPIC -DPIC


LTOCFLAGS:=-flto -fuse-linker-plugin -fno-fat-lto-objects
LTOLDFLAGS:=-flto -fuse-linker-plugin


OBJSUFFIX:=.o
EXESUFFIX:=.so
SOSUFFIX:=.so

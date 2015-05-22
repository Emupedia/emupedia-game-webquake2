# example of local configuration
# copy to local.mk


# location of source
TOPDIR:=..


LTO:=n
ASAN:=n
TSAN:=n
UBSAN:=n


USE_JPEG:=y
USE_PNG:=y

BUILD_SERVER:=y

# compiler options etc
CC:=i686-w64-mingw32-gcc
CXX:=i686-w64-mingw32-g++
CFLAGS:=-g -DNDEBUG
CFLAGS+=-Wall -Wextra
CFLAGS+=-Wno-sign-compare -Wno-unused-parameter
OPTFLAGS:=-O2 -march=native -fno-strict-aliasing -ffloat-store


# lazy assignment because CFLAGS is changed later
CXXFLAGS=$(CFLAGS) -std=c++11 -fno-exceptions -fno-rtti


LDFLAGS:=-g
LDLIBS:=-lwinmm -lkernel32
LDLIBS_ref_gl:=
LDLIBS_client:=


SOCFLAGS:=


LTOCFLAGS:=-flto -fuse-linker-plugin -fno-fat-lto-objects
LTOLDFLAGS:=-flto -fuse-linker-plugin


OBJSUFFIX:=.o
EXESUFFIX:=.exe
SOSUFFIX:=.so

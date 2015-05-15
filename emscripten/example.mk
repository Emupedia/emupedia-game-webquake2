# example of local configuration
# copy to local.mk


# location of source
TOPDIR:=..


LTO:=y
ASAN:=n
TSAN:=n
UBSAN:=n


USE_JPEG:=y


BUILTIN_GAME:=baseq2

# compiler options etc
CC:=emcc
CXX:=em++
CFLAGS:=-g -DNDEBUG -DLINUX -D_GNU_SOURCE=1
CFLAGS+=-I$(TOPDIR)/foreign/SDL2/include
OPTFLAGS:=-O3


# lazy assignment because CFLAGS is changed later
CXXFLAGS=$(CFLAGS) -std=c++11 -fno-exceptions -fno-rtti


LDFLAGS:=-g --preload-file baseq2
LDFLAGS+=-s TOTAL_MEMORY=268435456
LDFLAGS+=-s OUTLINING_LIMIT=5000
LDFLAGS+=-s FORCE_ALIGNED_MEMORY=1
LDLIBS:=
LDLIBS_ref_gl:=
LDLIBS_client:=$(TOPDIR)/foreign/SDL2/build/.libs/libSDL2.a


LTOCFLAGS:=--llvm-lto 3
LTOLDFLAGS:=--llvm-lto 3


OBJSUFFIX:=.o
EXESUFFIX:=.html

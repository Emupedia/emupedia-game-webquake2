# example of local configuration
# copy to local.mk


# location of source
TOPDIR:=..


LTO:=n
ASAN:=n
TSAN:=n
UBSAN:=n

BUILTIN_GAME:=baseq2

# compiler options etc
CC:=emcc
CXX:=em++
CFLAGS:=-g -DNDEBUG -DLINUX# -std=c99
CFLAGS+=-DREF_HARD_LINKED
OPTFLAGS:=-fno-strict-aliasing


LDFLAGS:=-g -s ALLOW_MEMORY_GROWTH=1 --preload-file baseq2
LDLIBS:=
LDLIBS_ref_gl:=
LDLIBS_client:=


LTOCFLAGS:=-flto -fuse-linker-plugin -fno-fat-lto-objects
LTOLDFLAGS:=-flto -fuse-linker-plugin


OBJSUFFIX:=.o
EXESUFFIX:=.html

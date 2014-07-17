# example of local configuration
# copy to local.mk


# location of source
TOPDIR:=..


LTO:=y
ASAN:=n
TSAN:=n
UBSAN:=n

BUILTIN_GAME:=baseq2

# compiler options etc
CC:=emcc
CXX:=em++
CFLAGS:=-g -DNDEBUG -DLINUX -std=c99 -D_GNU_SOURCE=1
CFLAGS+=-DREF_HARD_LINKED
OPTFLAGS:=-O3


LDFLAGS:=-g --preload-file baseq2
LDFLAGS+=-s ALLOW_MEMORY_GROWTH=1
LDFLAGS+=-s OUTLINING_LIMIT=5000
LDFLAGS+=-s FORCE_ALIGNED_MEMORY=1
LDLIBS:=
LDLIBS_ref_gl:=
LDLIBS_client:=


LTOCFLAGS:=--llvm-lto 3
LTOLDFLAGS:=--llvm-lto 3


OBJSUFFIX:=.o
EXESUFFIX:=.html

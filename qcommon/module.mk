sp             := $(sp).x
dirstack_$(sp) := $(d)
d              := $(dir)


SUBDIRS:= \
	# empty line

DIRS:=$(addprefix $(d)/,$(SUBDIRS))

$(eval $(foreach directory, $(DIRS), $(call directory-module,$(directory)) ))


FILES:= \
	cmd.c \
	cmodel.c \
	common.c \
	crc.c \
	cvar.c \
	files.c \
	ioapi.c \
	md4.c \
	mersennetwister.c \
	net_chan.c \
	net_websockets.cpp \
	pmove.c \
	redblack.c \
	unzip.c \
	# empty line


SRC_$(d):=$(addprefix $(d)/,$(FILES))


ifeq ($(USE_LIBWEBSOCKETS),y)
CFLAGS+=-isystem$(TOPDIR)/foreign/libwebsockets/lib
CFLAGS+=-DUSE_LIBWEBSOCKETS
endif  # USE_LIBWEBSOCKETS


d  := $(dirstack_$(sp))
sp := $(basename $(sp))

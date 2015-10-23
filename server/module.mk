sp             := $(sp).x
dirstack_$(sp) := $(d)
d              := $(dir)


SUBDIRS:= \
	# empty line

DIRS:=$(addprefix $(d)/,$(SUBDIRS))

$(eval $(foreach directory, $(DIRS), $(call directory-module,$(directory)) ))


FILES:= \
	sv_ccmds.c \
	sv_ents.c \
	sv_game.c \
	sv_init.c \
	sv_main.c \
	sv_send.c \
	sv_user.c \
	sv_world.c \
	# empty line


q2ded_MODULES:=linux qcommon qshared server shlinux shwin zlib win32
q2ded_SRC:=$(d)/q2ded.c


ifneq ($(BUILTIN_GAME),)
q2ded_MODULES+=$(BUILTIN_GAME)
endif


ifeq ($(USE_LIBWEBSOCKETS),y)
q2ded_MODULES+=libwebsockets
endif  # USE_LIBWEBSOCKETS


ifeq ($(BUILD_SERVER),y)

PROGRAMS+= \
	q2ded \
	#empty line

endif  # BUILD_SERVER


SRC_$(d):=$(addprefix $(d)/,$(FILES))


d  := $(dirstack_$(sp))
sp := $(basename $(sp))

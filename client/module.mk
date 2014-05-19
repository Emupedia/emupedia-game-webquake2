sp             := $(sp).x
dirstack_$(sp) := $(d)
d              := $(dir)


SUBDIRS:= \
	# empty line

DIRS:=$(addprefix $(d)/,$(SUBDIRS))

$(eval $(foreach directory, $(DIRS), $(call directory-module,$(directory)) ))


FILES:= \
	cl_cin.c \
	cl_ents.c \
	cl_fx.c \
	cl_input.c \
	cl_inv.c \
	cl_main.c \
	cl_parse.c \
	cl_pred.c \
	cl_tent.c \
	cl_scrn.c \
	cl_view.c \
	cl_newfx.c \
	console.c \
	keys.c \
	le_physics.c \
	le_util.c \
	menu.c \
	qmenu.c \
	snd_dma.c \
	snd_mem.c \
	snd_mix.c \
	# empty line


quake2_MODULES:=client libjpeg linux qcommon qshared ref_gl server shlinux
quake2_SRC:=


PROGRAMS+= \
	quake2 \
	#empty line


SRC_$(d):=$(addprefix $(d)/,$(FILES))


d  := $(dirstack_$(sp))
sp := $(basename $(sp))

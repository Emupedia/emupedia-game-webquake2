sp             := $(sp).x
dirstack_$(sp) := $(d)
d              := $(dir)


SUBDIRS:= \
	# empty line

DIRS:=$(addprefix $(d)/,$(SUBDIRS))

$(eval $(foreach directory, $(DIRS), $(call directory-module,$(directory)) ))


FILES:= \
	# empty line


unpak_MODULES:=
unpak_SRC:=$(d)/unpak.cpp


pak_MODULES:=
pak_SRC:=$(d)/pak.cpp


ifeq ($(BUILD_UTILS),y)

PROGRAMS+= \
	pak \
	unpak \
	#empty line

endif  # BUILD_SERVER


SRC_$(d):=$(addprefix $(d)/,$(FILES))


d  := $(dirstack_$(sp))
sp := $(basename $(sp))

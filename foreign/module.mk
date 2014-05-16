sp             := $(sp).x
dirstack_$(sp) := $(d)
d              := $(dir)


SUBDIRS:= \
	libjpeg-turbo-1.3.1 \
	# empty line

DIRS:=$(addprefix $(d)/,$(SUBDIRS))

$(eval $(foreach directory, $(DIRS), $(call directory-module,$(directory)) ))


FILES:= \
	# empty line


SRC_$(d):=$(addprefix $(d)/,$(FILES))


d  := $(dirstack_$(sp))
sp := $(basename $(sp))

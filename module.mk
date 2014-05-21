.PHONY: default all bindirs clean distclean


.SUFFIXES:

#initialize these
LIBRARIES:=
PROGRAMS:=
ALLSRC:=
# directories which might contain object files
# used for both clean and bindirs
ALLDIRS:=

default: all


ifeq ($(ASAN),y)

OPTFLAGS+=-fsanitize=address
LDFLAGS_asan?=-fsanitize=address
LDFLAGS+=$(LDFLAGS_asan)

endif


ifeq ($(TSAN),y)

CFLAGS+=-DPIC
OPTFLAGS+=-fsanitize=thread -fpic
LDFLAGS_tsan?=-fsanitize=thread -pie
LDFLAGS+=$(LDFLAGS_tsan)

endif


ifeq ($(UBSAN),y)

OPTFLAGS+=-fsanitize=undefined -fno-sanitize-recover
LDFLAGS+=-fsanitize=undefined -fno-sanitize-recover

endif


ifeq ($(LTO),y)

CFLAGS+=$(LTOCFLAGS)
LDFLAGS+=$(LTOLDFLAGS) $(OPTFLAGS)

endif


CFLAGS+=$(OPTFLAGS)
CFLAGS+=-isystem$(TOPDIR)/foreign/libjpeg-turbo-1.3.1
CFLAGS+=-isystem$(TOPDIR)/foreign/libpng-1.2.51


ifneq ($(BUILTIN_GAME),)
CFLAGS+=-DGAME_HARD_LINKED=$(BUILTIN_GAME)
endif


# (call directory-module, dirname)
define directory-module

# save old
DIRS_$1:=$$(DIRS)

dir:=$1
include $(TOPDIR)/$1/module.mk

ALLDIRS+=$1

ALLSRC+=$$(SRC_$1)

# restore saved
DIRS:=$$(DIRS_$1)

endef  # directory-module

DIRS:= \
	client \
	foreign \
	game \
	linux \
	qcommon \
	ref_gl \
	server \
	# empty line
$(eval $(foreach directory, $(DIRS), $(call directory-module,$(directory)) ))


TARGETS:=$(foreach PROG,$(PROGRAMS),$(PROG)$(EXESUFFIX))
TARGETS+=$(foreach LIB,$(LIBRARIES),$(LIB)$(SOSUFFIX))

all: $(TARGETS)


# check if a directory needs to be created
# can't use targets with the same name as directory because unfortunate
# interaction with VPATH (targets always exists because source dir)
#  $(call missingdir, progname)
define missingdir

ifneq ($$(shell test -d $1 && echo n),n)
MISSINGDIRS+=$1
endif

endef # missingdir

MISSINGDIRS:=
$(eval $(foreach d, $(ALLDIRS), $(call missingdir,$(d)) ))


# create directories which might contain object files
bindirs:
ifneq ($(MISSINGDIRS),)
	mkdir -p $(MISSINGDIRS)
endif


clean:
	rm -f $(TARGETS) $(foreach dir,$(ALLDIRS),$(dir)/*$(OBJSUFFIX))

distclean: clean
	rm -f $(foreach dir,$(ALLDIRS),$(dir)/*.d)
	-rmdir -p --ignore-fail-on-non-empty $(ALLDIRS)


# rules here

%$(OBJSUFFIX): %.c | bindirs
	$(CC) -c -MF $*.d -MP -MMD $(CFLAGS) -o $@ $<


%.sh$(OBJSUFFIX): %.c | bindirs
	$(CC) -c -MF $*.d -MP -MMD $(SOCFLAGS) $(CFLAGS) -o $@ $<


# $(call program-target, progname)
define program-target

ALLSRC+=$$(filter %.c,$$($1_SRC))

$1_SRC+=$$(foreach module, $$($1_MODULES), $$(SRC_$$(module)))
$1_OBJ:=$$($1_SRC:.c=$(OBJSUFFIX))
$1$(EXESUFFIX): $$($1_OBJ) | bindirs
	$(CC) $(LDFLAGS) -o $$@ $$^ $$(foreach module, $$($1_MODULES), $$(LDLIBS_$$(module))) $$($1_LIBS) $(LDLIBS)

endef  # program-target


$(eval $(foreach PROGRAM,$(PROGRAMS), $(call program-target,$(PROGRAM)) ) )


# $(call library-target, progname)
define library-target

ALLSRC+=$$(filter %.c,$$($1_SRC))

$1_SRC+=$$(foreach module, $$($1_MODULES), $$(SRC_$$(module)))
$1_OBJ:=$$($1_SRC:.c=.sh$(OBJSUFFIX))
$1$(SOSUFFIX): $$($1_OBJ) | bindirs
	$(CC) $(LDFLAGS) -shared -o $$@ $$^ $$(foreach module, $$($1_MODULES), $$(LDLIBS_$$(module))) $$($1_LIBS) $(LDLIBS)

endef  # library-target


$(eval $(foreach LIBRARY,$(LIBRARIES), $(call library-target,$(LIBRARY)) ) )


-include $(foreach FILE,$(ALLSRC),$(patsubst %.c,%.d,$(FILE)))

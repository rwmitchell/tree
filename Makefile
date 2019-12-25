# $Copyright: $
# Copyright (c) 1996 - 2018 by Steve Baker
# Original Code from: http://mama.indstate.edu/users/ice/tree/src/
# Copyright (c) 2019 - 2020 by Richard Mitchell for changes to version 1.8.0
# All Rights reserved
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

include make.$(OSTYPE)

# Uncomment for HP/UX:
#CC=cc
#CFLAGS=-Ae +O2 +DAportable -Wall
#LDFLAGS=

RLS  = release
DBG  = debug
PTH := $(RLS)

SRC = ./Source
OBJ = ./obj
DST = ./bin

list:

.PHONY: debug
debug:  CFLAGS += $(CC_DEBUG_FLAGS)
debug:  PTH    := $(DBG)
debug:  make_it

.PHONY: release
release:  CFLAGS += $(CC_RELEASE_FLAGS)
release:  PTH    := $(RLS)
release:  make_it

column = sed 's/ / 	/g' | tr ' |' '\n\n'

DIR = $(shell basename $(CURDIR))
BLD = $(HOME)/Build
MCH = $(BLD)/$(MACHTYPE)
BAS = $(MCH)/$(PTH)/$(DIR)
DEP = $(MCH)/.dep/$(DIR)

DST = $(BAS)/bin
OBJ = $(BAS)/obj
LIB = $(BAS)/lib

# Override this on the cmdline with: make prefix=/some/where/else
bin_prefix = $(BLD)
SRC = Source
NST = $(bin_prefix)/bin

VERSION=1.8.0
TREE_DEST=tree
MAN=tree.1
MANDIR=${prefix}/man/man1

# Programs
DST_PROGS =          \
	$(DST)/tree        \

ALL_PROGS =          \
	$(DST_PROGS)       \
	$(MTH_PROGS)       \

DIRS =   \
	$(DEP) \
	$(OBJ) \
	$(DST) \
	$(NST) \

# Additional object files
OBJ_FILES =           \
	$(OBJ)/color.o      \
	$(OBJ)/file.o       \
	$(OBJ)/hash.o       \
	$(OBJ)/html.o       \
	$(OBJ)/json.o       \
	$(OBJ)/strverscmp.o \
	$(OBJ)/unix.o       \
	$(OBJ)/xml.o        \

$(NST)/%: $(DST)/%
	install -m ugo+rx $< $@

list:
	@echo "all install"
	@echo debug release
	@echo $(DST_PROGS)
	@echo $(NST_PROGS)

all:	          \
	$(DIRS)       \
	$(OBJ_PROGS)  \
	$(DST_PROGS)  \
	tags          \
	show_install  \

$(DST_PROGS):	$(DST)/%:	$(OBJ)/%.o $(OBJ_FILES)
	$(CC) $(LD_FLAGS) -o $@ $^

# 2019-12-25: WARNING: requiring $(DEP)/%d on next line breaks fresh compiles
# Comment out for initial compile
# then restore for .h file changes  to require a recompile
$(OBJ)/%.o:	$(SRC)/%.c # $(DEP)/%.d
	@echo "|Making_OBJ:$@ $^ " | $(column)
	$(CC) -c $(CFLAGS) -I$(SRC) -o $@ $<

NST_PROGS = $(subst $(DST),$(NST),$(DST_PROGS))

.PHONY: install help_install

install:       \
	$(NST)       \
	$(NST_PROGS) \

$(DIRS):
	mkdir -p $@


help_install:
	@printf "These programs are made:"
	@printf "\nDST:\t"; printf "$(DST_PROGS)" | $(column)
	@printf "\nTry: make install\n"

	@printf "These programs are installed:"
	@printf "\nPROGS:\t"; printf "$(NST_PROGS)" | $(column)

rebuild: clean all

reinstall: rebuild install

uninstall:
	$(RM) $(NST_PROGS)

clean:
	$(RM) $(DEP)/*.d $(OBJ)/*.o $(DST_PROGS)
	rmdir $(OBJ) $(DST)

var:
	@ printf "\nDST :\t%s" $(DST)
	@ printf "\nNST :\t%s" $(NST)
	@ printf "\nOBJ :\t%s" $(OBJ)
	@ printf "\nSRC :\t%s" $(SRC)
	@ printf "\nobj :\t"; printf "$(OBJ_FILES)" | $(column)
	@ printf "\nDST :\t"; printf "$(DST_PROGS)" | $(column)
	@ printf "\nNST :\t"; printf "$(NST_PROGS)" | $(column)

show_install:
	@ echo "make install:"
	@ make -sn install
	@ echo

make_it:
	make PTH=$(PTH) CFLAGS="$(CFLAGS)" $(LDFLAGS) all

#We don't need to clean up when we're making these targets
NODEPS:=clean tags svn install
#Find all C files in the $(SRC)/ directory
SOURCES:=$(shell find $(SRC)  -name "*.c")
#These are the dependency files, which make will clean up after it creates them
DEPFILES:=$(patsubst %.c,%.d,$(patsubst $(SRC)/%,$(DEP)/%, $(SOURCES)))

#Don't create dependencies when we're cleaning, for instance
ifeq (0, $(words $(findstring $(MAKECMDGOALS), $(NODEPS))))
    #Chances are, these files don't exist.  GMake will create them and
    #clean up automatically afterwards
    -include $(DEPFILES)
endif

#This is the rule for creating the dependency files
$(DEP)/%.d: $(SRC)/%.c $(DEP)
	@echo "START DEP: $@"
	@echo $(CC) $(CFLAGS) -MM -MT '$(patsubst $(SRC)/%,$(OBJ)/%, $(patsubst %.c,%.o,$<))' $<
	$(CC) $(CFLAGS) -MG -MM -MT '$(patsubst $(SRC)/%,$(OBJ)/%, $(patsubst %.c,%.o,$<))' $< > $@
	@echo "END   DEP: $@"
# End of - Dependency code added here

# Make a highlight file for types.  Requires Exuberant ctags and awk
types: $(SRC)/.types.vim
$(SRC)/.types.vim: $(SRC)/*.[ch]
	ctags --c-kinds=gstu -o- $(SRC)/*.[ch] |\
		awk 'BEGIN{printf("syntax keyword Type\t")}\
		{printf("%s ", $$1)}END{print ""}' > $@
	ctags --c-kinds=d -o- $(SRC)/*.h |\
		awk 'BEGIN{printf("syntax keyword Debug\t")}\
		{printf("%s ", $$1)}END{print ""}' >> $@
# End types

tags: $(SRC)/*
	ctags --fields=+l --langmap=c:.c.h $(SRC)/*

#
# ##########################################################
#
# $(DST)/tree:	$(OBJ)/tree.o
# 	$(CC) $(LDFLAGS) -o  $@ $(OBJ)/tree.o

# $(OBJ):
# 	mkdir $@

# $(DST):
# 	mkdir $@

# clean:
# 	if [ -x $(TREE_DEST) ]; then rm $(TREE_DEST); fi
# 	if [ -f tree.o ]; then rm *.o; fi
# 	rm -f *~

# install:
# 	install -d $(BINDIR)
# 	install -d $(MANDIR)
# 	if [ -e $(TREE_DEST) ]; then \
# 		install -s $(TREE_DEST) $(BINDIR)/$(TREE_DEST); \
# 	fi
# 	install $(MAN) $(MANDIR)/$(MAN)

# distclean:
# 	if [ -f tree.o ]; then rm *.o; fi
# 	rm -f *~
	

# dist:	distclean
# 	tar zcf ../tree-$(VERSION).tgz -C .. `cat .tarball`

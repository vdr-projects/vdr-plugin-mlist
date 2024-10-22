#
# Makefile for a Video Disk Recorder plugin
#
# $Id$

# The official name of this plugin.
# This name will be used in the '-P...' option of VDR to load the plugin.
# By default the main source file also carries this name.
#
PLUGIN = mlist

### Set this to '0' if you don't want mlist to auto-configure itself
AUTOCONFIG=1

### If AUTOCONFIG is not active,you can manually enable the
### optional modules or patches for other plugins:

ifeq ($(AUTOCONFIG),0)
	# If you want to use Perl compatible regular expressions (PCRE) or libtre for
	# unlimited fuzzy searching, uncomment this and set the value to pcre/pcre2
	# or tre; also have a look at INSTALL for further notes on this.
	#REGEXLIB = pcre2
endif

### The version number of this plugin (taken from the main source file):

VERSION = $(shell grep 'static const char \*VERSION *=' $(PLUGIN).c | awk '{ print $$6 }' | sed -e 's/[";]//g')

# Use package data if installed...otherwise assume we're under the VDR source directory:
PKG_CONFIG ?= pkg-config
PKGCFG = $(if $(VDRDIR),$(shell $(PKG_CONFIG) --variable=$(1) $(VDRDIR)/vdr.pc),$(shell $(PKG_CONFIG) --variable=$(1) vdr || $(PKG_CONFIG) --variable=$(1) ../../../vdr.pc))
LIBDIR = $(DESTDIR)$(call PKGCFG,libdir)
LOCDIR = $(DESTDIR)$(call PKGCFG,locdir)
PLGCFG = $(call PKGCFG,plgcfg)
#
TMPDIR = /tmp

### The compiler options:

export CFLAGS   = $(call PKGCFG,cflags)
export CXXFLAGS = $(call PKGCFG,cxxflags)

### The module configuration:

ifeq ($(AUTOCONFIG),1)
	ifeq (exists, $(shell $(PKG_CONFIG) libpcre2-posix && echo exists))
		REGEXLIB = pcre2
	else ifeq (exists, $(shell $(PKG_CONFIG) libpcre && echo exists))
		REGEXLIB = pcre
	else ifeq (exists, $(shell $(PKG_CONFIG) tre && echo exists))
		REGEXLIB = tre
	endif
endif

### The PCRE settings:

ifeq ($(REGEXLIB), pcre2)
	LIBS += $(shell pcre2-config --libs-posix)
	INCLUDE += $(shell pcre2-config --cflags)
	DEFINES += -DHAVE_PCRE2POSIX
else ifeq ($(REGEXLIB), pcre)
	LIBS += $(shell pcre-config --libs-posix)
	INCLUDE += $(shell pcre-config --cflags)
	DEFINES += -DHAVE_PCREPOSIX
else ifeq ($(REGEXLIB), tre)
	LIBS += -L$(shell $(PKG_CONFIG) --variable=libdir tre) $(shell $(PKG_CONFIG) --libs tre)
	DEFINES += -DHAVE_LIBTRE
	INCLUDE += $(shell $(PKG_CONFIG) --cflags tre)
endif

### The version number of VDR's plugin API (taken from VDR's "config.h"):

APIVERSION = $(call PKGCFG,apiversion)
DOXYFILE = Doxyfile
DOXYGEN  = doxygen

### Allow user defined options to overwrite defaults:

-include $(PLGCFG)

### The name of the distribution archive:

ARCHIVE = $(PLUGIN)-$(VERSION)
PACKAGE = vdr-$(ARCHIVE)

### The name of the shared object file:
SOFILE = libvdr-$(PLUGIN).so

### Includes and Defines (add further entries here):

INCLUDES += -I$(call PKGCFG,incdir)
DEFINES += -DPLUGIN_NAME_I18N='"$(PLUGIN)"'

### The object files (add further files here):

OBJS = $(PLUGIN).o

### Internationalization (I18N):

PODIR     = po
I18Npo    = $(wildcard $(PODIR)/*.po)
I18Nmo    = $(addsuffix .mo, $(foreach file, $(I18Npo), $(basename $(file))))
I18Nmsgs  = $(addprefix $(LOCDIR)/, $(addsuffix /LC_MESSAGES/vdr-$(PLUGIN).mo, $(notdir $(foreach file, $(I18Npo), $(basename $(file))))))
I18Npot   = $(PODIR)/$(PLUGIN).pot

### Targets:
all: $(SOFILE) i18n

$(SOFILE): $(OBJS) 
	@echo LD $@
	$(Q)$(CXX) $(CXXFLAGS) -shared $(OBJS) $(LDFLAGS) $(LIBS) -o $@
	
install-lib: $(SOFILE)
	@echo IN $(DESTDIR)$(LIBDIR)/$^.$(APIVERSION)
	$(Q)install -D $^ $(DESTDIR)$(LIBDIR)/$^.$(APIVERSION)

install: install-lib install-i18n

%.o: %.c
	@echo CC $@
	$(Q)$(CXX) $(CXXFLAGS) -c $(DEFINES) $(INCLUDES) $<

dist: clean
	@-rm -rf $(TMPDIR)/$(ARCHIVE)
	@mkdir $(TMPDIR)/$(ARCHIVE)
	@cp -a * $(TMPDIR)/$(ARCHIVE)
	@tar czf $(PACKAGE).tgz --exclude=.* -C $(TMPDIR) $(ARCHIVE)
	@-rm -rf $(TMPDIR)/$(ARCHIVE)
	@echo Distribution package created as $(PACKAGE).tgz

clean:
	@-rm -f $(PODIR)/*.mo $(PODIR)/*.pot
	@-rm -f $(OBJS) $(DEPFILE) *.so *.a *.tgz core* *~
	
%.mo: %.po
	@echo MO $@
	$(Q)msgfmt -c -o $@ $<

$(I18Npot): $(wildcard *.c)
	@echo GT $@
	$(Q)xgettext -C -cTRANSLATORS --no-wrap -s --no-location -k -ktr -ktrNOOP -kI18N_NOOP \
	         --package-name=vdr-$(PLUGIN) --package-version=$(VERSION) --msgid-bugs-address='<vdr@joachim-wilke.de>' -o $@ `ls $^`
	$(Q)grep -v POT-Creation $(I18Npot) > $(I18Npot)~
	$(Q)mv $(I18Npot)~ $(I18Npot)

%.po: $(I18Npot)
	@echo PO $@
	$(Q)msgmerge -U --no-wrap --no-location --backup=none -q -N $@ $<
	@touch $@

$(I18Nmsgs): $(LOCDIR)/%/LC_MESSAGES/vdr-$(PLUGIN).mo: $(PODIR)/%.mo
	@echo IN $@
	$(Q)install -D -m644 $< $@

i18n: $(I18Nmo) $(I18Npot)

install-i18n: $(I18Nmsgs)

srcdoc:
	@cp $(DOXYFILE) $(DOXYFILE).tmp
	@echo PROJECT_NUMBER = $(VERSION) >> $(DOXYFILE).tmp
	$(DOXYGEN) $(DOXYFILE).tmp
	@rm $(DOXYFILE).tmp

.PHONY: i18n 
				
# Dependencies:

MAKEDEP = $(CXX) -MM -MG
DEPFILE = .dependencies
$(DEPFILE): Makefile
	@$(MAKEDEP) $(DEFINES) $(INCLUDES) $(OBJS:%.o=%.c) $(CXXFLAGS) > $@

-include $(DEPFILE)

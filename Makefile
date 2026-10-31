ifeq ($(MODULE_BUILD),1)
#---------------------------------------------------------------------------------
# pull in common stratosphere sysmodule configuration
#---------------------------------------------------------------------------------
THIS_MAKEFILE := $(abspath $(lastword $(MAKEFILE_LIST)))
CURRENT_DIRECTORY := $(abspath $(dir $(THIS_MAKEFILE)))
include $(dir $(abspath $(lastword $(MAKEFILE_LIST))))/Atmosphere-libs/config/templates/stratosphere.mk
TARGET := ssl_proxy

GIT_SHORT := $(shell git rev-parse --short HEAD 2>/dev/null)
GIT_BRANCH := $(shell git symbolic-ref --short HEAD 2>/dev/null)
ifeq ($(GIT_SHORT),)
export GIT_REVISION := snapshot
else
export GIT_REVISION := $(or $(GIT_BRANCH),detached)-$(GIT_SHORT)$(if $(shell git status --porcelain --untracked-files=no 2>/dev/null),-dirty)
endif

CFLAGS		+= -DGIT_REVISION="\"${GIT_REVISION}\"" -DAMS_IMPL_ENABLE_LOG -DAMS_ENABLE_DETAILED_ASSERTIONS
CXXFLAGS	+= -DGIT_REVISION="\"${GIT_REVISION}\"" -DAMS_IMPL_ENABLE_LOG -DAMS_ENABLE_DETAILED_ASSERTIONS

ATMOSPHERE_SYSTEM_MODULE_TARGETS := nsp

#---------------------------------------------------------------------------------
# no real need to edit anything past this point unless you need to add additional
# rules for different file extensions
#---------------------------------------------------------------------------------
ifneq ($(__RECURSIVE__),1)
#---------------------------------------------------------------------------------

export TOPDIR	:=	$(CURDIR)

export VPATH	:=	$(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) \
			$(foreach dir,$(DATA),$(CURDIR)/$(dir))

CFILES      :=	$(call FIND_SOURCE_FILES,$(SOURCES),c)
CPPFILES    :=	$(call FIND_SOURCE_FILES,$(SOURCES),cpp)
SFILES      :=	$(call FIND_SOURCE_FILES,$(SOURCES),s)

BINFILES	:=	$(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.*)))

#---------------------------------------------------------------------------------
# use CXX for linking C++ projects, CC for standard C
#---------------------------------------------------------------------------------
ifeq ($(strip $(CPPFILES)),)
#---------------------------------------------------------------------------------
	export LD	:=	$(CC)
#---------------------------------------------------------------------------------
else
#---------------------------------------------------------------------------------
	export LD	:=	$(CXX)
#---------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------

export OFILES	:=	$(addsuffix .o,$(BINFILES)) \
			$(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)

export INCLUDE	:=	$(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
			$(foreach dir,$(LIBDIRS),-I$(dir)/include) \
			$(foreach dir,$(AMS_LIBDIRS),-I$(dir)/include) \
			-I$(CURDIR)/$(ATMOSPHERE_BUILD_DIR)

export LIBPATHS	:=	$(foreach dir,$(LIBDIRS),-L$(dir)/lib) $(foreach dir,$(AMS_LIBDIRS),-L$(dir)/$(ATMOSPHERE_LIBRARY_DIR))

export BUILD_EXEFS_SRC := $(TOPDIR)/$(EXEFS_SRC)

ifeq ($(strip $(CONFIG_JSON)),)
	jsons := $(wildcard *.json)
	ifneq (,$(findstring $(TARGET).json,$(jsons)))
		export APP_JSON := $(TOPDIR)/$(TARGET).json
	else
		ifneq (,$(findstring config.json,$(jsons)))
			export APP_JSON := $(TOPDIR)/config.json
		endif
	endif
else
	export APP_JSON := $(TOPDIR)/$(CONFIG_JSON)
endif

.PHONY: clean all check_lib

#---------------------------------------------------------------------------------
all: $(ATMOSPHERE_OUT_DIR) $(ATMOSPHERE_BUILD_DIR) $(ATMOSPHERE_LIBRARIES_DIR)/libstratosphere/$(ATMOSPHERE_LIBRARY_DIR)/libstratosphere.a
	@$(MAKE) __RECURSIVE__=1 OUTPUT=$(CURDIR)/$(ATMOSPHERE_OUT_DIR)/$(TARGET) \
	DEPSDIR=$(CURDIR)/$(ATMOSPHERE_BUILD_DIR) \
	--no-print-directory -C $(ATMOSPHERE_BUILD_DIR) \
	-f $(THIS_MAKEFILE)

$(ATMOSPHERE_LIBRARIES_DIR)/libstratosphere/$(ATMOSPHERE_LIBRARY_DIR)/libstratosphere.a: check_lib
	@$(SILENTCMD)echo "Checked library."

ifeq ($(ATMOSPHERE_CHECKED_LIBSTRATOSPHERE),1)
check_lib:
else
check_lib:
	@$(MAKE) --no-print-directory -C $(ATMOSPHERE_LIBRARIES_DIR)/libstratosphere -f $(ATMOSPHERE_LIBRARIES_DIR)/libstratosphere/libstratosphere.mk
endif

$(ATMOSPHERE_OUT_DIR) $(ATMOSPHERE_BUILD_DIR):
	@[ -d $@ ] || mkdir -p $@

#---------------------------------------------------------------------------------
clean:
	@echo clean ...
	@rm -fr $(ATMOSPHERE_OUT_DIR) $(ATMOSPHERE_BUILD_DIR)


#---------------------------------------------------------------------------------
else
.PHONY:	all

DEPENDS	:=	$(OFILES:.o=.d)

#---------------------------------------------------------------------------------
# main targets
#---------------------------------------------------------------------------------
all	:	$(foreach target,$(ATMOSPHERE_SYSTEM_MODULE_TARGETS),$(OUTPUT).$(target))

$(OUTPUT).kip :	$(OUTPUT).elf
$(OUTPUT).nsp :	$(OUTPUT).nso $(OUTPUT).npdm
$(OUTPUT).nso :	$(OUTPUT).elf

$(OUTPUT).elf :	$(OFILES)

$(OFILES) : $(ATMOSPHERE_LIBRARIES_DIR)/libstratosphere/$(ATMOSPHERE_LIBRARY_DIR)/libstratosphere.a

%.npdm  :   %.npdm.json
	@echo built ... $< $@
	@npdmtool $< $@
	@echo built ... $(notdir $@)

#---------------------------------------------------------------------------------
# you need a rule like this for each extension you use as binary data
#---------------------------------------------------------------------------------
%.bin.o	:	%.bin
#---------------------------------------------------------------------------------
	@echo $(notdir $<)
	@$(bin2o)

-include $(DEPENDS)

#---------------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------------
else
OUTDIR := out
TITLE_ID := 430053534C505801
SD_ROOT := $(OUTDIR)/sd
TITLE_DIR := $(SD_ROOT)/atmosphere/contents/$(TITLE_ID)

GIT_SHORT := $(shell git rev-parse --short HEAD 2>/dev/null)
GIT_BRANCH := $(shell git symbolic-ref --short HEAD 2>/dev/null)
ifeq ($(GIT_SHORT),)
SSL_PROXY_REVISION := snapshot
else
SSL_PROXY_REVISION := $(subst /,-,$(or $(GIT_BRANCH),detached))-$(GIT_SHORT)$(if $(shell git status --porcelain --untracked-files=no 2>/dev/null),-dirty)
endif

SSL_PROXY_VERSION_MAJOR := $(shell grep 'define SSL_PROXY_VERSION_MAJOR\b' source/ssl_proxy_version.h | tr -s [:blank:] | cut -d' ' -f3)
SSL_PROXY_VERSION_MINOR := $(shell grep 'define SSL_PROXY_VERSION_MINOR\b' source/ssl_proxy_version.h | tr -s [:blank:] | cut -d' ' -f3)
SSL_PROXY_VERSION_MICRO := $(shell grep 'define SSL_PROXY_VERSION_MICRO\b' source/ssl_proxy_version.h | tr -s [:blank:] | cut -d' ' -f3)
SSL_PROXY_VERSION := $(SSL_PROXY_VERSION_MAJOR).$(SSL_PROXY_VERSION_MINOR).$(SSL_PROXY_VERSION_MICRO)-$(SSL_PROXY_REVISION)

all: dist

dependencies:
	git submodule update --init --recursive

build: dependencies
	$(MAKE) -C Atmosphere-libs/libstratosphere nx_release
	$(MAKE) MODULE_BUILD=1 all

pack: build
	@mkdir -p $(TITLE_DIR)/flags
	@cp out/nintendo_nx_arm64_armv8a/release/ssl_proxy.nsp $(TITLE_DIR)/exefs.nsp
	@touch $(TITLE_DIR)/flags/boot2.flag
	@rm -f $(TITLE_DIR)/mitm.lst
	@echo "ssl" >> $(TITLE_DIR)/mitm.lst
	@echo "ssl:s" >> $(TITLE_DIR)/mitm.lst

dist: pack
	@rm -f $(OUTDIR)/ssl_proxy-$(SSL_PROXY_VERSION).zip
	@cd $(SD_ROOT) && zip ../ssl_proxy-$(SSL_PROXY_VERSION).zip \
		atmosphere/contents/$(TITLE_ID)/exefs.nsp \
		atmosphere/contents/$(TITLE_ID)/flags/boot2.flag \
		atmosphere/contents/$(TITLE_ID)/mitm.lst > /dev/null

clean:
	$(MAKE) -C Atmosphere-libs/libstratosphere clean
	$(MAKE) MODULE_BUILD=1 clean

.PHONY: all dependencies build pack dist clean
endif

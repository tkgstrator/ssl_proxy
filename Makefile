OUTDIR := out
TITLE_ID := 4200000000000666
SD_ROOT := $(OUTDIR)/sd
TITLE_DIR := $(SD_ROOT)/atmosphere/contents/$(TITLE_ID)

GIT_SHORT := $(shell git rev-parse --short HEAD 2>/dev/null)
GIT_BRANCH := $(shell git symbolic-ref --short HEAD 2>/dev/null)
ifeq ($(GIT_SHORT),)
SSL_PROXY_REVISION := snapshot
else
SSL_PROXY_REVISION := $(or $(GIT_BRANCH),detached)-$(GIT_SHORT)$(if $(shell git status --porcelain --untracked-files=no 2>/dev/null),-dirty)
endif

NETWORK_MITM_VERSION_MAJOR := $(shell grep 'define NETWORK_MITM_VERSION_MAJOR\b' network_mitm/include/networkmitm_version.h | tr -s [:blank:] | cut -d' ' -f3)
NETWORK_MITM_VERSION_MINOR := $(shell grep 'define NETWORK_MITM_VERSION_MINOR\b' network_mitm/include/networkmitm_version.h | tr -s [:blank:] | cut -d' ' -f3)
NETWORK_MITM_VERSION_MICRO := $(shell grep 'define NETWORK_MITM_VERSION_MICRO\b' network_mitm/include/networkmitm_version.h | tr -s [:blank:] | cut -d' ' -f3)
SSL_PROXY_VERSION := $(NETWORK_MITM_VERSION_MAJOR).$(NETWORK_MITM_VERSION_MINOR).$(NETWORK_MITM_VERSION_MICRO)-$(SSL_PROXY_REVISION)

all: dist

dependencies:
	./scripts/fetch-dependencies.sh

build: dependencies
	$(MAKE) -C Atmosphere-libs/libstratosphere nx_release
	$(MAKE) -C network_mitm all

pack: build
	@mkdir -p $(TITLE_DIR)/flags
	@cp network_mitm/out/nintendo_nx_arm64_armv8a/release/network_mitm.nsp $(TITLE_DIR)/exefs.nsp
	@touch $(TITLE_DIR)/flags/boot2.flag
	@rm -f $(TITLE_DIR)/mitm.lst
	@echo "ssl" >> $(TITLE_DIR)/mitm.lst
	@echo "ssl:s" >> $(TITLE_DIR)/mitm.lst

dist: pack
	@cd $(SD_ROOT) && zip -r ../ssl_proxy-$(SSL_PROXY_VERSION).zip ./* > /dev/null

clean:
	$(MAKE) -C Atmosphere-libs/libstratosphere clean
	$(MAKE) -C network_mitm clean

.PHONY: all dependencies build pack dist clean

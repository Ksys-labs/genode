MICROECC = micro-ecc
GIT_URL  = http://github.com/kmackay/micro-ecc.git
GIT_REV  = ac0d7a3fab4211a00f0d2fb40e0402f333d6542f

#
# Check for tools
#
$(call check_tool,git)

#
# Interface to top-level prepare Makefile
#
PORTS += $(MICROECC)

prepare-micro-ecc: $(CONTRIB_DIR)/$(MICROECC) include/micro-ecc/ecc.h

$(CONTRIB_DIR)/$(MICROECC): clean-micro-ecc

#
# Port-specific local rules
#

$(DOWNLOAD_DIR)/$(MICROECC)/.git:
	$(VERBOSE)git clone $(GIT_URL) $(DOWNLOAD_DIR)/$(MICROECC) && \
	cd $(DOWNLOAD_DIR)/$(MICROECC) && \
	git reset --hard $(GIT_REV) && \
	cd ../.. && touch $@
	
$(CONTRIB_DIR)/$(MICROECC)/.git: $(DOWNLOAD_DIR)/$(MICROECC)/.git
	$(VERBOSE)git clone $(DOWNLOAD_DIR)/$(MICROECC) $(CONTRIB_DIR)/$(MICROECC)
	
$(CONTRIB_DIR)/$(MICROECC): $(CONTRIB_DIR)/$(MICROECC)/.git

include/micro-ecc/ecc.h:
	$(VERBOSE)mkdir -p $(dir $@)
	$(VERBOSE)ln -s ../../$(CONTRIB_DIR)/$(MICROECC)/ecc.h $@

clean-micro-ecc:
	$(VERBOSE)rm -rf include/micro-ecc
	$(VERBOSE)rm -rf $(CONTRIB_DIR)/$(MICROECC)

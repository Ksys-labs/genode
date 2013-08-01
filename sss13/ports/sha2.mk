SHA2_VERSION = 1.0.1
SHA2         = sha2-$(SHA2_VERSION)
SHA2_TGZ     = $(SHA2).tgz
SHA2_URL     = http://www.aarongifford.com/computers/sha2-1.0.1.tgz

#
# Interface to top-level prepare Makefile
#
PORTS += $(SHA2)

prepare-sha2: $(CONTRIB_DIR)/$(SHA2) include/sha2/sha2.h

$(CONTRIB_DIR)/$(SHA2):clean-sha2

#
# Port-specific local rules
#
$(DOWNLOAD_DIR)/$(SHA2_TGZ):
	$(VERBOSE)wget -c -P $(DOWNLOAD_DIR) $(SHA2_URL) && touch $@

$(CONTRIB_DIR)/$(SHA2): $(DOWNLOAD_DIR)/$(SHA2_TGZ)
	$(VERBOSE)tar xf $(DOWNLOAD_DIR)/$(SHA2_TGZ) -C $(CONTRIB_DIR) && touch $@

include/sha2/sha2.h:
	$(VERBOSE)mkdir -p $(dir $@)
	ln -sf ../../$(CONTRIB_DIR)/$(SHA2)/sha2.h $@

clean-sha2:
	$(VERBOSE)rm -rf include/sha2
	$(VERBOSE)rm -rf $(CONTRIB_DIR)/$(SHA2)

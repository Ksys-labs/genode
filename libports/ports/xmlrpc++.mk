XMLRPC     = xmlrpc++0.7
XMLRPC_TGZ = $(XMLRPC).tar.gz
XMLRPC_URL = http://downloads.sourceforge.net/project/xmlrpcpp/xmlrpc++/Version\ 0.7/$(XMLRPC_TGZ)

#
# Interface to top-level prepare Makefile
#
PORTS += $(XMLRPC)

prepare-xmlrpc++: $(CONTRIB_DIR)/$(XMLRPC) include/xmlrpc++

$(CONTRIB_DIR)/$(XMLRPC): clean-xmlrpc++

#
# Port-specific local rules
#
$(DOWNLOAD_DIR)/$(XMLRPC_TGZ):
	$(VERBOSE)wget -c -P $(DOWNLOAD_DIR) $(XMLRPC_URL) && touch $@


$(CONTRIB_DIR)/$(XMLRPC): $(DOWNLOAD_DIR)/$(XMLRPC_TGZ)
	$(VERBOSE)tar xfz $< -C $(CONTRIB_DIR) && touch $@
	$(VERBOSE)patch -d $(CONTRIB_DIR)/$(XMLRPC) -p1 -i ../../src/lib/xmlrpc++/xmlrpc++.patch
	$(VERBOSE)patch -d $(CONTRIB_DIR)/$(XMLRPC) -p1 -i ../../src/lib/xmlrpc++/xmlrpc++_fix_tests.patch

include/xmlrpc++:
	$(VERBOSE)mkdir -p include/xmlrpc++
	$(VERBOSE)ln -s $(addprefix ../../, $(wildcard $(CONTRIB_DIR)/$(XMLRPC)/src/*.h)) -t $@


clean-xmlrpc++:
	$(VERBOSE)rm -rf include/xmlrpc++
	$(VERBOSE)rm -rf $(CONTRIB_DIR)/$(XMLRPC)

TTCP_C   = ttcp.c
#TTCP_URL = http://sd.wareonearth.com/~phil/net/ttcp/ttcp.c
TTCP_ZIP = UnixTTCP.zip
TTCP_URL = http://www.pcausa.com/Utilities/pcattcp/$(TTCP_ZIP)

$(TTCP_C):
	$(VERBOSE)wget -c $(TTCP_URL) && unzip $(TTCP_ZIP) && mv UnixTTCP/$(TTCP_C) . && touch $@
	$(VERBOSE)patch -p1 -i $(REP_DIR)/src/noux-pkg/ttcp/ttcp.patch 

TARGET = ttcp
SRC_C = $(TTCP_C)
LIBS = libc_noux libc

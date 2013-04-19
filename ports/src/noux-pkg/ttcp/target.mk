TTCP_C   = ttcp.c
TTCP_URL = http://sd.wareonearth.com/~phil/net/ttcp/ttcp.c

$(TTCP_C):
	$(VERBOSE)wget -c $(TTCP_URL) && touch $@

TARGET = ttcp
SRC_C = $(TTCP_C)
LIBS = libc_noux libc

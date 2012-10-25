TARGET   = test-xmlrpc-srv

SRC_CC   = HelloServer.cpp

LIBS     = cxx stdcxx xmlrpc++ libc libc_log libm libc_lwip_nic_dhcp

vpath HelloServer.cpp $(REP_DIR)/contrib/xmlrpc++0.7/test
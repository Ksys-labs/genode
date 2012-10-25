XMLRPC     = xmlrpc++0.7
XMLRPC_DIR = $(REP_DIR)/contrib/$(XMLRPC)
LIBS       += cxx stdcxx libc

INC_DIR += $(XMLRPC_DIR)/src/

# dim build noise for contrib code
CC_WARN = -Wno-unused-but-set-variable


SRC_CC += \
	XmlRpcClient.cpp \
	XmlRpcDispatch.cpp \
	XmlRpcServer.cpp \
	XmlRpcServerConnection.cpp \
	XmlRpcServerMethod.cpp \
	XmlRpcSocket.cpp \
	XmlRpcSource.cpp \
	XmlRpcUtil.cpp \
	XmlRpcValue.cpp

vpath %.cpp       $(XMLRPC_DIR)/src

SHARED_LIB = yes

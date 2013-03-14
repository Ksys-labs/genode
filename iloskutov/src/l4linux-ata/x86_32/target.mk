REQUIRES        = x86 32bit
L4LINUX_DIR     = $(REP_DIR)/../ports-foc
INC_DIR        += $(L4LINUX_DIR)/include/32-bit
LX_TARGET       = l4linux-ata
LX_BASE_TARGET  = l4linux
SRC_L4LX_CONFIG = linux_config.x86_32

L4LX_REP_DIR    = $(L4LINUX_DIR)

-include $(L4LINUX_DIR)/mk/l4lx.mk

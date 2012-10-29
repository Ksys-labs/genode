# identify the qt4 repository by searching for a file that is unique for qt4
QT4_REP_DIR := $(call select_from_repositories,lib/import/import-qt4.inc)

ifeq ($(QT4_REP_DIR),)
REQUIRES += qt4
endif

QT4_REP_DIR := $(realpath $(dir $(QT4_REP_DIR))../..)

-include $(QT4_REP_DIR)/src/app/tmpl/target_defaults.inc

QT_MAIN_STACK_SIZE = 768*1024

LIBS     += libc_lwip libc_lwip_nic_dhcp

#
# Prevent contrib code from causing warnings with our toolchain compiler
#
CC_WARN += -Wno-unused-but-set-variable

vpath % $(PRG_DIR)

-include $(QT4_REP_DIR)/src/app/tmpl/target_final.inc
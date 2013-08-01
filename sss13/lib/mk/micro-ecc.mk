MICRO_ECC_DIR = $(REP_DIR)/contrib/micro-ecc

SRC_C = ecc.c

INC_DIR += $(MICRO_ECC_DIR)

LIBS  += libc-static

vpath %.c $(MICRO_ECC_DIR)
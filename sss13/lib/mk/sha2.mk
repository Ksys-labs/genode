SHA2_DIR = $(REP_DIR)/contrib/sha2-1.0.1

SRC_C = sha2.c

INC_DIR += $(SHA2_DIR)

LIBS  += libc-static

vpath %.c $(SHA2_DIR)
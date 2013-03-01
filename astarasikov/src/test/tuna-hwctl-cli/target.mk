#
# \brief  Tests for tuna hardware
# \author Alexander Tarasikov
# \date   2013-01-15
#

TARGET = test-tuna-hwctl-cli
REQUIRES = omap4
SRC_CC += main.cc
LIBS += cxx env

vpath main.cc $(PRG_DIR)

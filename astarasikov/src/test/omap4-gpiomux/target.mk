#
# \brief  Tests for GPIO MUX interface on omap4
# \author Alexander Tarasikov
# \date   2013-01-15
#

TARGET = test-omap4-gpiomux
REQUIRES = omap4
SRC_CC += main.cc
LIBS += cxx env

vpath main.cc $(PRG_DIR)

#
# \brief  Melfas MMS Touchscreen driver
# \author Alexander Tarasikov
# \date   2012-12-25
#

TARGET = test-mms-touchscreen
REQUIRES = omap4
SRC_CC += main.cc
LIBS += cxx env server

vpath main.cc $(PRG_DIR)

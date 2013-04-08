TARGET = test-x86-usb
REQUIRES = x86
SRC_CC += main.cc
LIBS += cxx env signal

vpath main.cc $(PRG_DIR)

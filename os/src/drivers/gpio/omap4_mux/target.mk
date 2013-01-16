TARGET   = omap4_gpiomux_drv
REQUIRES = omap4
SRC_CC   = main.cc mux44xx_data.cc
LIBS     = cxx env server signal
INC_DIR += $(PRG_DIR)

vpath main.cc $(PRG_DIR)


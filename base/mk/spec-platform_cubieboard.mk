#
# \brief  Build-system configurations specific to Cubieboard
# \author Ivan Loskutov
# \date   2013-04-18
#

# denote wich specs are also fullfilled by this spec
SPECS += cortex_a8 sun4i

# add repository relative include paths
REP_INC_DIR += include/platform/sun4i

# include implied specs
include $(call select_from_repositories,mk/spec-cortex_a8.mk)


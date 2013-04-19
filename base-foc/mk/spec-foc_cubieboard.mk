SPECS += foc_arm platform_cubieboard

include $(call select_from_repositories,mk/spec-platform_cubieboard.mk)
include $(call select_from_repositories,mk/spec-foc_arm.mk)

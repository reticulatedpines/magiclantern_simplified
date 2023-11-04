TOP_DIR=..

############################################################################################################
# include platform data to build the rules for every model
# build the targets, by stripping the firmware version from the entries in PLATFORM_MAP above
#
# this allows e.g.
#   make 5D3
#   make 5D3.113
#   make 5D3.123
#   make 5D3_install
#   make 5D3.113_install
# on this platform Makefile
#
include Makefile.platform.map
$(foreach _,$(PLATFORM_MAP),$(eval $(call makerule,$(word 1, $(subst ., ,$_)),$_,./)))
#
# we want ML platforms to be built sequentially, to avoid conflicts
# => use .NOTPARALLEL in the upper-level Makefiles only
# parallel build is still used within each platform
.NOTPARALLEL:
#
############################################################################################################


include $(TOP_DIR)/Makefile.setup


# FIXME this module is broken.  It tries to use current_task fields,
# but in order to know that it must have a concrete definition for
# "struct task".  It cannot, because different cams have different
# structs, and module code must be cam independent.
#
# After we split the task struct into V1, V2 variants etc,
# this behaviour is broken.

# define the module name - make sure name is max 8 characters
MODULE_NAME = trace
MODULE_OBJS = $(BUILD_DIR)/trace.o
TOP_DIR = ../..

# include modules environment
include $(TOP_DIR)/modules/Makefile
.DEFAULT_GOAL := $(BUILD_DIR)/module_complete

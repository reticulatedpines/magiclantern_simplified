
# define the module name - make sure name is max 8 characters
MODULE_NAME=io_crypt
MODULE_OBJS=io_crypt.o crypt_lfsr64.o hash_password.o crypt_rsa.o bigdigits.o bigd.o

# include modules environment
include ../Makefile.modules

# include host tool makefile rules
include Makefile.io_decrypt

CFLAGS+=-DTRACE_DISABLED
HOST_CFLAGS+=-DTRACE_DISABLED
MINGW_CFLAGS+=-DTRACE_DISABLED

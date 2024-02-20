# only "continue" if we are running standalone 
# e.g. arm-none-eabi-gdb -x 80D/patches.gdb

if $STANDALONE
  continue
end

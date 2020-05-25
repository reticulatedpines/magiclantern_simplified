# only include debug-logging.gdb if it wasn't already
# fixme: how to do this properly?

set $STANDALONE = 0
if $_thread == 0
  set $STANDALONE = 1
end

if $STANDALONE
  source -v debug-logging.gdb
  # some address that always has a null value
  macro define CURRENT_TASK 0xC0000000
  macro define CURRENT_ISR 0
end

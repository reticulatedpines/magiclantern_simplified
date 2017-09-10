# only include debug-logging.gdb if it wasn't already
# fixme: how to do this properly?

set $STANDALONE = 0
if $_thread == 0
  set $STANDALONE = 1
end

if $STANDALONE
  source -v debug-logging.gdb
end

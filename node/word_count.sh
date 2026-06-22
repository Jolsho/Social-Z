
find . -path './build' -prune -o \
  \( -name '*.c' -o -name '*.cc' -o -name '*.cpp' -o -name '*.h' -o -name '*.hpp' \) \
  -print0 | xargs -0 wc -l

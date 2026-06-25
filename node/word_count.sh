
find . -path './build' -prune -o \
    -path './install' -prune -o \
  \( -name '*.c' -o -name '*.cc' -o -name '*.cpp' -o -name '*.h' -o -name '*.hpp' \) \
  -print0 | xargs -0 wc -l


# 6/22/26 = 12,531

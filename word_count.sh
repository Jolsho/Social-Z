find . \
  \( -path './node/build' -o \
     -path './node/install' -o \
     -path './web_ui/node_modules' -o \
     -path './web_ui/dist' -o \
     -path './node/wasm_install' -o \
     -path './node/wasm_build' \
  \) -prune -o \
  \( -name '*.c' -o \
     -name '*.cc' -o \
     -name '*.cpp' -o \
     -name '*.h' -o \
     -name '*.hpp' -o \
     -name 'CMakeLists.txt' -o \
     -name '*.cmake' -o \
     -name '*.sh' -o \
     -name '*.go' -o \
     -name '*.ts' -o \
     -name '*.tsx' -o \
     -name '*.css' \
  \) -print0 | xargs -0 wc -l


# 6/29/26 = 17,536
# 7/07/26 = 17,985

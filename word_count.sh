# Copyright (c) 2026 Jolsho
# SPDX-License-Identifier: LGPL-3.0-or-later

find . \
  \( -path './build' -o \
     -path './install' -o \
     -path './-Development' -o \
     -path './-Documentation' -o \
     -path './web_ui/node_modules' -o \
     -path './web_ui/dist' -o \
     -path './wasm_install' -o \
     -path './wasm_build' \
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
# 7/21/26 = 18,469

# 9/01/26 = 17,504
# 9/29/26 = 24,929

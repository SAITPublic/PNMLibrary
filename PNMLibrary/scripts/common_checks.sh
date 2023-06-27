#!/bin/sh

# All project quick common checks are here

ROOT=$(git rev-parse --show-toplevel)
TIDY_CHECK_SRCS=$(cd $ROOT && git diff origin --name-only --diff-filter=d "*.c" "*.h" "*.cpp")

## common checks
echo "Clang format..."
git clang-format origin
echo "Clang tidy..."
[ ! -z "$TIDY_CHECK_SRCS" ] && (cd $ROOT && run-clang-tidy -p build $TIDY_CHECK_SRCS)
echo "Check the correct usage of includes..."
(cd $ROOT && ./scripts/check_includes.sh)

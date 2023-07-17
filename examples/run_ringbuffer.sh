#/bin/bash

TEST_CODE="x9_ringbuffer_1.c"
TEST_BINARY="X9_TEST_RINGBUFFER_1"

echo "- Running examples with GCC with \"-fsanitize=thread,undefined\" enabled."
gcc -Wextra -Wall -Werror -pedantic -O3 -march=native ${TEST_CODE} ../rb.c -o ${TEST_BINARY} -fsanitize=thread,undefined -D X9_DEBUG

stdbuf -oL perf stat ./${TEST_BINARY} | tee out
rm -f ${TEST_BINARY}

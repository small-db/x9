#/bin/bash

TEST_CODE="x9_ringbuffer_1.c"
TEST_BINARY="X9_TEST_RINGBUFFER_1"

echo "- Running examples with GCC with \"-fsanitize=thread,undefined\" enabled."
gcc -Wextra -Wall -Werror -pedantic -O3 -march=native ${TEST_CODE} ../x9.c -o ${TEST_BINARY} -fsanitize=thread,undefined -D X9_DEBUG

time ./${TEST_BINARY}
rm ${TEST_BINARY}

exit 0

echo ""
echo "- Running examples with clang with \"-fsanitize=address,undefined,leak\" enabled."

clang -Wextra -Wall -Werror -O3 -march=native ${TEST_CODE} ../x9.c -o ${TEST_BINARY} -fsanitize=address,undefined,leak -D X9_DEBUG

time ./${TEST_BINARY}
rm ${TEST_BINARY}

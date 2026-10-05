#include <stdint.h>

int32_t caller(int32_t, int32_t *);
static int calls;
void write(int32_t *pointer, int32_t value) { *pointer = value; }
int32_t external(int32_t left, int32_t right) {
  ++calls;
  return left + right;
}

int main() {
  int32_t value = 0;
  int32_t result = caller(21, &value);
  return result == 42 && value == 21 && calls == 2 ? 0 : 1;
}

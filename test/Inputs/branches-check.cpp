#include <stdint.h>

int32_t jump(int32_t);
int32_t choose(bool, int32_t, int32_t);
int32_t swap(int32_t, int32_t);
int32_t layout();
int32_t memory(int32_t);
int32_t *pointer(int32_t *);
int32_t pair(int32_t left, int32_t right) { return left * 100 + right; }

int main() {
  int32_t value = 42;
  return jump(42) == 42 && choose(true, 12, 34) == 12 &&
                 choose(false, 12, 34) == 34 && swap(12, 34) == 3412 &&
                 layout() == 42 && memory(42) == 42 && pointer(&value) == &value
             ? 0
             : 1;
}

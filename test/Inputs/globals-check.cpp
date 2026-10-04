#include <stdint.h>

extern int32_t value;
extern const int32_t answer;
extern bool enabled;
extern float single;
extern double real;
extern int32_t *pointer;

int main() {
  if (value != 12 || answer != 42 || !enabled || single != 2.5f ||
      real != 1.25 || pointer != nullptr)
    return 1;
  value = 13;
  return value != 13;
}

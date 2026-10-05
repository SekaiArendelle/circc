#include <stdint.h>
int32_t read();
void write(int32_t);
int32_t *address();
int32_t read_constant();
int32_t *constant_address();
int32_t read_external();
int32_t *read_pointer();
extern const int32_t answer;
int32_t external_value = 19;
int main() {
  if (read() != 12 || read_constant() != 42 || read_external() != 19 ||
      read_pointer() != nullptr || constant_address() != &answer)
    return 1;
  write(27);
  if (read() != 27 || *address() != 27)
    return 2;
  *address() = 31;
  return read() != 31;
}

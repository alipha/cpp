#include "system.h"

class Example_1 : public RegisteredSystem<Example_1> {
  public:
    static constexpr const char *name = "Example 1";
};

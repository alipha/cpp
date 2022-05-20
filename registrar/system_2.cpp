#include "system.h"

class Example_2 : public RegisteredSystem<Example_2> {
  public:
    static constexpr const char *name = "Example 2";
};

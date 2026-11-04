---
name: cpp-new-file
description: Create a new C++ class, struct, enum or module (header + source + test) following the project layout and header rules. Use when adding a new .hpp/.cpp file, a new class, or a new test file.
---

# Adding a C++ file

## Steps

1. Pick the area folder (`core`, `geom`, `topo`, `euler`, `model`, `app`, ...) and a `snake_case` name.
   Check the name is not already used anywhere in `include/`, `src/` or `tests/` (`src/` and `tests/` are
   flat, so names must be unique).
2. Create the three files:
   - `include/jcad/<area>/<name>.hpp`
   - `src/<name>.cpp` (skip only for header-only templates/constexpr utilities)
   - `tests/test_<name>.cpp`
3. Add `src/<name>.cpp` to its layer's library target in `src/CMakeLists.txt` and `tests/test_<name>.cpp`
   to its layer's test target in `tests/CMakeLists.txt`. Do not glob.
4. Run the Definition of Done from `CLAUDE.md`.

## Header member order

Sections in this order; skip empty ones. Types come before the members that use them, because a
member function signature cannot name a type declared later in the class.

```
public:
  constants
  type aliases and enums
  inner structs/classes
  static functions
  constructors and destructor
  copy/move operations (= default / = delete)
  member functions
  operators
protected:
  same order as private
private:
  constants
  type aliases, enums, inner structs/classes
  static functions
  member functions
  static data members
  data members
```

Prefer default member initializers (`Type m_type{ Type::OTHER };`) over constructor init lists for fixed defaults.

## Test file

```cpp
// tests/test_names_registry.cpp
#include "core/names_registry.hpp"

#include <gtest/gtest.h>

TEST(NamesRegistry, CreateEntryLowercaseInputReturnsUppercase)
{
  const my_reg::Name name = my_reg::NamesRegistry::CreateEntry("abc", false);
  EXPECT_EQ(name.name, "ABC");
  EXPECT_TRUE(name.is_uppercase);
}
```

Use `TEST_F` with a fixture named `<Class>Test` when tests share setup.

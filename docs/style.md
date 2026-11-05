# Naming conventions

| Entity | Style | Example |
|---|---|---|
| Files | `snake_case` | `names_registry.hpp` |
| Classes, structs, enums, type aliases, template params | `PascalCase` | `NamesRegistry` |
| Functions (free and member, static or not) | `PascalCase` | `CreateEntry` |
| Parameters (const or not) | `camelCase` | `isUppercase` |
| Local variables (const or not), lambdas | `snake_case` | `treat_case` |
| Public data members (struct fields) | `snake_case` | `is_uppercase` |
| Private/protected data members | `m_snake_case` | `m_name` |
| Static non-const data members | `s_snake_case` | `s_count` |
| Global non-const variables | `g_snake_case` | `g_state` |
| Constants (global, static, `constexpr`), enumerators, macros | `SCREAMING_CASE` | `MAX_SIZE` |
| Namespaces, concepts | `snake_case` | `my_reg`, `hashable` |
| CMake files and targets | `PascalCase` | `CompilationOptions` |

All code, comments, documentation and commit messages are written in English.

Notes:

- A member function with the same name as a type hides that type inside the class. That is why the kernel's scalar sign predicate is called `SignOf` rather than `Sign`.
- Specializations and members required by the standard library (`std::hash<...>::operator()`, `begin()`/`end()` for range-for) keep the names the library expects.

# Code rules

## Types and ownership

- `struct`: plain data, all public, no invariants. `class`: has invariants or private state.
- Always `enum class`.
- No `new`/`delete`/`malloc`/`free`. Owners are `std::unique_ptr` (`std::shared_ptr` only for truly shared ownership).
- Raw `T*` only as a non-owning, nullable observer. `T&` when always present. `std::optional` for optional values.
- Avoid `friend`.

## Functions

- At most 4 parameters (functions, constructors, lambdas). Group more into a struct declared next to the function.
- No default arguments.
- Parameter passing:
  - cheap types (built-ins, enums, views, small trivially copyable structs): by value
  - read-only: `const T&`
  - sink (function keeps a copy): by value, then `std::move`
  - move-only types (`std::unique_ptr`): by value
  - out-parameters: avoid; return a value (or a struct) instead
  - `T&&` only when the caller must visibly give up ownership; always `std::move` it.
    Forwarding references (`template <typename T> void F(T&& v)`) use `std::forward`.
- Do not modify a by-value parameter, except to move from it.
- At most 4 nesting levels inside a function body (the body itself is level 1; enclosing namespaces and
  classes do not count). Extract a function instead of going deeper.

## Variables

- Every local is `const` unless it is modified. Exception: variables holding a lambda are not `const`.
- Brace initialization: `T x{ ... }`. Use parentheses only when braces would pick an `initializer_list`
  constructor, e.g. `std::vector<int> v(10, 0)`. `T x = F();` is fine for function results.
- `auto` only when the type is already on the right-hand side (`std::make_unique`, casts), for iterators,
  lambdas and structured bindings. Spell the type everywhere else.

## Errors

- Recoverable errors: return `std::expected<T, E>`.
- Constructors that cannot establish their invariant throw. Do not add static `Create` factories to avoid
  this. Exceptions are caught only at boundaries (`main`, thread entry points, UI event handlers).
- Programmer errors and broken invariants: `JCAD_ASSERT(cond)` / `JCAD_ASSERT(cond, "fmt {}", arg)` from
  `core/assert.hpp`. It is compiled out in release, so never put side effects in the condition.

## Files and headers

- `#pragma once`. Include order is handled by clang-format.
- Include a header only when a forward declaration is not enough. Forward-declare project types only,
  never standard or third-party types.
- If a forward-declared type is held in `std::unique_ptr`, define the destructor in the `.cpp`.
- No function bodies in headers, except templates, `constexpr`, `= default`/`= delete`, and header-only utilities.
- Namespace-scope constants in headers are `inline constexpr`.
- One class/struct per header, plus types that only exist to serve it. An `enum class` used by more
  than one header gets its own header.
- In `.cpp` files, define members with qualified names (`void my_ns::MyClass::Run()`) instead of reopening
  the namespace. Helpers go in an anonymous namespace, not `static`.
- `using namespace` only inside function bodies, never `using namespace std`
  (`std::literals` and `std::chrono_literals` are allowed).

## Formatting not handled by clang-format

- A body without braces is allowed only on the same line: `if (done) return;`. If the body goes on the
  next line, it gets braces.

## Comments

- Code should explain itself; comment only non-obvious algorithms and the reason behind surprising code.
- Functions that are hard to use correctly get a Doxygen `/** */` comment on the header declaration.

## Tests

- `tests/test_<file>.cpp`, one per source file.
- `TEST(Suite, MethodConditionExpected)`, where the suite is the class name (or the file's PascalCase
  name for free functions). No underscores: `TEST(NamesRegistry, CreateEntryLowercaseInputReturnsUppercase)`.

## Layout, member order and formatting

- File layout, class member order and the test file template: `.claude/skills/cpp-new-file/SKILL.md`.
- Formatting: the repository's `.clang-format`, checked in CI with clang-format 18.
- When the member order says "constants" before "type aliases" but a constant's type is one of the
  aliases, the alias comes first (types before the members that use them).

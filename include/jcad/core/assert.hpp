#pragma once

#include <format>
#include <source_location>
#include <string>
#include <string_view>

namespace jcad::core::detail
{

  [[noreturn]] void AssertFail(std::string_view expression,
                               std::string_view message,
                               const std::source_location& location);

  std::string FormatAssertMessage();

  // Not constexpr on purpose: it keeps constant conditions such as
  // JCAD_ASSERT(false, "unreachable") from triggering MSVC's C4127.
  [[nodiscard]] inline bool AssertCondition(bool value) noexcept
  {
    return value;
  }

  template <typename... Args>
  std::string FormatAssertMessage(std::format_string<Args...> fmt, Args&&... args)
  {
    return std::format(fmt, std::forward<Args>(args)...);
  }

} // namespace jcad::core::detail

/**
 * Checks a programmer error or a broken invariant: JCAD_ASSERT(cond) or
 * JCAD_ASSERT(cond, "fmt {}", arg). On failure it prints the expression, the
 * message and the location, then aborts.
 *
 * Compiled out when NDEBUG is defined, so the condition must not have side effects.
 */
#ifdef NDEBUG
  #define JCAD_ASSERT(cond, ...) static_cast<void>(0)
#else
  #define JCAD_ASSERT(cond, ...)                                                                   \
    do                                                                                             \
    {                                                                                              \
      if (!::jcad::core::detail::AssertCondition(static_cast<bool>(cond)))                         \
      {                                                                                            \
        ::jcad::core::detail::AssertFail(#cond,                                                    \
                                         ::jcad::core::detail::FormatAssertMessage(__VA_ARGS__),   \
                                         std::source_location::current());                         \
      }                                                                                            \
    } while (false)
#endif

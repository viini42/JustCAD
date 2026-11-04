#pragma once

#include <format>
#include <source_location>
#include <string>
#include <string_view>

namespace justcad::core::detail {

[[noreturn]] void AssertFail(std::string_view expression, std::string_view message,
                             const std::source_location& location);

std::string FormatAssertMessage();

template <typename... Args>
std::string FormatAssertMessage(std::format_string<Args...> fmt, Args&&... args) {
    return std::format(fmt, std::forward<Args>(args)...);
}

}  // namespace justcad::core::detail

/**
 * Checks a programmer error or a broken invariant: JUSTCAD_ASSERT(cond) or
 * JUSTCAD_ASSERT(cond, "fmt {}", arg). On failure it prints the expression, the
 * message and the location, then aborts.
 *
 * Compiled out when NDEBUG is defined, so the condition must not have side effects.
 */
#ifdef NDEBUG
#define JUSTCAD_ASSERT(cond, ...) static_cast<void>(0)
#else
#define JUSTCAD_ASSERT(cond, ...)                                                       \
    do {                                                                                \
        if (!(cond)) {                                                                  \
            ::justcad::core::detail::AssertFail(                                        \
                #cond, ::justcad::core::detail::FormatAssertMessage(__VA_ARGS__),       \
                std::source_location::current());                                       \
        }                                                                               \
    } while (false)
#endif

#include "core/assert.hpp"

#include <cstdio>
#include <cstdlib>

void jcad::core::detail::AssertFail(std::string_view expression,
                                    std::string_view message,
                                    const std::source_location& location)
{
  const std::string text{ std::format("{}:{}: assertion failed: {}{}{}\n",
                                      location.file_name(),
                                      location.line(),
                                      expression,
                                      message.empty() ? "" : ": ",
                                      message) };
  std::fputs(text.c_str(), stderr);
  std::fflush(stderr);
  std::abort();
}

std::string jcad::core::detail::FormatAssertMessage()
{
  return {};
}

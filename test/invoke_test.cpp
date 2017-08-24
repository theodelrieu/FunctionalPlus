// Copyright 2015, Tobias Hermann and the FunctionalPlus contributors.
// https://github.com/Dobiasd/FunctionalPlus
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include <fplus/invoke.hpp>

#include <stdexcept>
#include <string>

using namespace fplus::detail;

namespace
{
int regular_function_sum(int a, int b) noexcept
{
  return a + b;
}

// int throwing_function(bool t)
// {
//   if (t)
//     throw std::runtime_error{""};
//   return 0;
// }
}

TEST_CASE("regular function")
{
  using regular_function_t = decltype(regular_function_sum);
  // using throwing_function_t = decltype(throwing_function);

  // implicit conversions work
  static_assert(is_invocable<regular_function_t, int, unsigned int>::value, "");
  static_assert(is_invocable_r<bool, regular_function_t, int, unsigned int>::value, "");
  // static_assert(is_nothrow_invocable<regular_function_t, int, unsigned int>::value, "");
  // static_assert(is_nothrow_invocable_r<bool, regular_function_t, int, unsigned int>::value, "");
  //
  // static_assert(!is_nothrow_invocable<throwing_function_t, bool>::value, "");
  // static_assert(!is_nothrow_invocable_r<int, throwing_function_t, bool>::value, "");

  static_assert(!is_invocable<regular_function_t, int, char*>::value, "");
  static_assert(!is_invocable<regular_function_t, int, char, char>::value, "");
  static_assert(!is_invocable_r<std::string, regular_function_t, int, unsigned int>::value, "");

  static_assert(noexcept(invoke(regular_function_sum, 32, 10), 42), "");

  REQUIRE_EQ(invoke(regular_function_sum, 32, 10), 42);
}

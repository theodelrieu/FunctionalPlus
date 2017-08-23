// Copyright 2015, Tobias Hermann and the FunctionalPlus contributors.
// https://github.com/Dobiasd/FunctionalPlus
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <type_traits>

namespace fplus
{
namespace detail
{
// C++14 compatible void_t (http://en.cppreference.com/w/cpp/types/void_t)
template <typename... Ts>
struct make_void
{
  using type = void;
};

template <typename... Ts>
using void_t = typename make_void<Ts...>::type;

template <class...>
struct disjunction : std::false_type
{
};

template <class B1>
struct disjunction<B1> : B1
{
};

template <class B1, class... Bn>
struct disjunction<B1, Bn...>
    : std::conditional<bool(B1::value), B1, disjunction<Bn...>>::type
{
};
}
}

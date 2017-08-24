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

// disjunction/conjunction/negation, useful to short circuit SFINAE checks
// Use with parsimony, MSVC 2015 can have ICEs quite easily
template <typename...>
struct disjunction : std::false_type
{
};

template <typename B1>
struct disjunction<B1> : B1
{
};

template <typename B1, typename... Bn>
struct disjunction<B1, Bn...>
    : std::conditional<bool(B1::value), B1, disjunction<Bn...>>::type
{
};

template <typename...>
struct conjunction : std::true_type
{
};

template <typename B1>
struct conjunction<B1> : B1
{
};

template <typename B1, typename... Bn>
struct conjunction<B1, Bn...>
    : std::conditional<bool(B1::value), conjunction<Bn...>, B1>::type
{
};

template <typename B>
struct negation : std::integral_constant<bool, !bool(B::value)>
{
};
}
}

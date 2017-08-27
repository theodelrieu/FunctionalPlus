// Copyright 2015, Tobias Hermann and the FunctionalPlus contributors.
// https://github.com/Dobiasd/FunctionalPlus
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include <functional>
#include <fplus/invoke.hpp>

#include <initializer_list>
#include <stdexcept>
#include <string>
#include <tuple>

using namespace fplus;

namespace
{
template <typename T>
struct identity
{
  identity() = default;
  using type = T;
};

// Thanks to Louis Dionne and his master thesis.
template <typename Ret, typename Class, typename... Args>
auto cv_qualifiers(identity<Ret (Class::*)(Args...)>)
    -> std::tuple<identity<Ret (Class::*)(Args...)>,
                  identity<Ret (Class::*)(Args...) const>,
                  identity<Ret (Class::*)(Args...) volatile>,
                  identity<Ret (Class::*)(Args...) const volatile>>
{
  return{};
}

template <typename Ret, typename Class, typename... Args>
auto ref_qualifiers(identity<Ret (Class::*)(Args...)>)
    -> std::tuple<identity<Ret (Class::*)(Args...) &>,
                  identity<Ret (Class::*)(Args...) &&>>
{
  return {};
}

template <typename Ret, typename Class, typename... Args>
auto ref_qualifiers(identity<Ret (Class::*)(Args...) const>)
    -> std::tuple<identity<Ret (Class::*)(Args...) const &>,
                  identity<Ret (Class::*)(Args...) const &&>>
{
  return {};
}

template <typename Ret, typename Class, typename... Args>
auto ref_qualifiers(identity<Ret (Class::*)(Args...) volatile>)
    -> std::tuple<identity<Ret (Class::*)(Args...) volatile&>,
                  identity<Ret (Class::*)(Args...) volatile&&>>
{
  return {};
}

template <typename Ret, typename Class, typename... Args>
auto ref_qualifiers(identity<Ret (Class::*)(Args...) const volatile>)
    -> std::tuple<identity<Ret (Class::*)(Args...) const volatile&>,
                  identity<Ret (Class::*)(Args...) const volatile&&>>
{
  return {};
}

template <typename T>
auto ref_qualifiers(identity<T>) -> std::tuple<identity<T&>, identity<T&&>>
{
  return std::tuple<identity<T&>, identity<T&&>>{};
}

// long live c++11 ....
template <typename T,
          typename I = decltype(cv_qualifiers(identity<T>{})),
          typename Love = decltype(ref_qualifiers(std::get<0>(I{}))),
          typename Boil = decltype(ref_qualifiers(std::get<1>(I{}))),
          typename Erpl = decltype(ref_qualifiers(std::get<2>(I{}))),
          typename Ate = decltype(ref_qualifiers(std::get<3>(I{})))>
auto all_qualifiers(identity<T>) -> std::tuple<
    typename std::remove_reference<decltype(std::get<0>(Love{}))>::type,
    typename std::remove_reference<decltype(std::get<1>(Love{}))>::type,
    typename std::remove_reference<decltype(std::get<0>(Boil{}))>::type,
    typename std::remove_reference<decltype(std::get<1>(Boil{}))>::type,
    typename std::remove_reference<decltype(std::get<0>(Erpl{}))>::type,
    typename std::remove_reference<decltype(std::get<1>(Erpl{}))>::type,
    typename std::remove_reference<decltype(std::get<0>(Ate{}))>::type,
    typename std::remove_reference<decltype(std::get<1>(Ate{}))>::type>
{
  return {};
}

template <typename... Args>
struct all_invocable;

template <typename... TupleArgs, typename Class, typename... FuncArgs>
struct all_invocable<std::tuple<TupleArgs...>, Class, FuncArgs...>
{
  using Tuple = std::tuple<TupleArgs...>;

  // two ::type, becaus of identity wrapper
  template <std::size_t N, typename T>
  using Elem = typename std::tuple_element<N, T>::type::type;

  static_assert(sizeof...(TupleArgs) == 8,
                "If you don't want to test all cv/ref combinations, you need "
                "to reimplement this with make_index_sequence (and reimplement "
                "it as well, we compile with C++11");

  // Class& because `&` functions can only be invokej on lvalue references
  static constexpr bool value = detail::conjunction<
      detail::is_invocable<Elem<0, Tuple>, Class&, FuncArgs...>,
      detail::is_invocable<Elem<1, Tuple>, Class, FuncArgs...>,
      detail::is_invocable<Elem<2, Tuple>, Class&, FuncArgs...>,
      detail::is_invocable<Elem<3, Tuple>, Class, FuncArgs...>,
      detail::is_invocable<Elem<4, Tuple>, Class&, FuncArgs...>,
      detail::is_invocable<Elem<5, Tuple>, Class, FuncArgs...>,
      detail::is_invocable<Elem<6, Tuple>, Class&, FuncArgs...>,
      detail::is_invocable<Elem<7, Tuple>, Class, FuncArgs...>>::value;
};
}

namespace
{
int regular_function_sum(int a, int b)
{
  return a + b;
}

template <unsigned int N, typename ...Args>
auto return_n_arg_type(Args&&... args) -> typename std::tuple_element<N, std::tuple<Args...>>::type
{
  return std::get<N>(std::forward_as_tuple(std::forward<Args>(args)...));
}

struct function_object_t
{
  int i = 0;

  int operator()(int a, int b) const
  {
    return a + b;
  }

  void mutate_data()
  {
    i = 0;
  }
};

struct derived_function_object_t : function_object_t
{
};
}

TEST_CASE("regular function")
{
  using regular_function_t = decltype(regular_function_sum);
  using regular_function_ptr_t = std::add_pointer<decltype(regular_function_sum)>::type;

  // implicit conversions work
  static_assert(detail::is_invocable<regular_function_t, int, unsigned int>::value, "");
  static_assert(detail::is_invocable_r<bool, regular_function_t, int, unsigned int>::value, "");

  static_assert(!detail::is_invocable<regular_function_t, int, char*>::value, "");
  static_assert(!detail::is_invocable<regular_function_t, int, char, char>::value, "");
  static_assert(!detail::is_invocable_r<std::string, regular_function_t, int, unsigned int>::value, "");

  static_assert(detail::is_invocable<regular_function_ptr_t, int, unsigned int>::value, "");
  static_assert(detail::is_invocable_r<bool, regular_function_ptr_t, int, unsigned int>::value, "");

  static_assert(!detail::is_invocable<regular_function_ptr_t, int, char*>::value, "");
  static_assert(!detail::is_invocable<regular_function_ptr_t, int, char, char>::value, "");
  static_assert(!detail::is_invocable_r<std::string, regular_function_ptr_t, int, unsigned int>::value, "");

  REQUIRE_EQ(detail::invoke(regular_function_sum, 32, 10), 42);
}

TEST_CASE("regular variadic function")
{
  int i = 42;

  using variadic_function_t = decltype(return_n_arg_type<0, int&, float>);

  static_assert(detail::is_invocable<variadic_function_t, int&, float>::value, "");
  static_assert(detail::is_invocable_r<const int&, variadic_function_t, int&, float>::value, "");

  static_assert(!detail::is_invocable<variadic_function_t, int, float>::value, "");
  static_assert(!detail::is_invocable_r<short&, variadic_function_t, int&, float>::value, "");

  REQUIRE_EQ(std::addressof(detail::invoke(return_n_arg_type<0, int&, float>, i, 2.0f)),
             std::addressof(i));
}

TEST_CASE("function object")
{
  static_assert(detail::is_invocable<function_object_t, int const&, double>::value, "");
  static_assert(detail::is_invocable_r<int&&, function_object_t, int&, float>::value, "");

  static_assert(!detail::is_invocable<function_object_t, int, std::string>::value, "");
  static_assert(!detail::is_invocable_r<int&, function_object_t, int, int>::value, "");

  REQUIRE_EQ(detail::invoke(function_object_t{}, 40, 2), 42);
}

TEST_CASE("lambda")
{
  auto add = [](int a, int b) { return a + b; };

  using lambda_t = decltype(add);

  static_assert(detail::is_invocable<lambda_t, int const&, double>::value, "");
  static_assert(detail::is_invocable_r<int&&, lambda_t, int&, float>::value, "");

  static_assert(!detail::is_invocable<lambda_t, int, std::string>::value, "");
  static_assert(!detail::is_invocable_r<int&, lambda_t, int, int>::value, "");

  REQUIRE_EQ(detail::invoke(add, 40, 2), 42);
}

TEST_CASE("member function - object reference")
{
  using call_operator_t = decltype(&function_object_t::operator());
  using mutate_data_t = decltype(&function_object_t::mutate_data);

  auto qualifiers =
      all_qualifiers(identity<int (function_object_t::*)(int, int)>{});
  static_assert(all_invocable<decltype(qualifiers), function_object_t, int const&, double>::value, "");

  static_assert(detail::is_invocable<call_operator_t, function_object_t, int const&, double>::value, "");
  static_assert(detail::is_invocable<mutate_data_t, function_object_t>::value, "");
  static_assert(detail::is_invocable_r<int&&, call_operator_t, function_object_t, int&, float>::value, "");

  // non-const member function
  static_assert(detail::is_invocable<mutate_data_t, function_object_t&>::value, "");
  static_assert(!detail::is_invocable<mutate_data_t, const function_object_t&>::value, "");

  static_assert(!detail::is_invocable_r<int&, call_operator_t, function_object_t, int, int>::value, "");

  auto adder = function_object_t{};
  REQUIRE_EQ(detail::invoke(&function_object_t::operator(), adder, 40, 2), 42);
}

TEST_CASE("member function - reference_wrapper<object>")
{
  using call_operator_t = decltype(&function_object_t::operator());
  using mutate_data_t = decltype(&function_object_t::mutate_data);
  using ref_wrapper_t = std::reference_wrapper<function_object_t>;
  using ref_wrapper_const_t = std::reference_wrapper<const function_object_t>;

  static_assert(detail::is_invocable<call_operator_t, ref_wrapper_t, int const&, double>::value, "");
  static_assert(detail::is_invocable<call_operator_t, ref_wrapper_const_t, int const&, double>::value, "");
  static_assert(detail::is_invocable_r<int&&, call_operator_t, ref_wrapper_t, int&, float>::value, "");
  static_assert(detail::is_invocable_r<int&&, call_operator_t, ref_wrapper_const_t, int&, float>::value, "");

  // non-const member function
  static_assert(detail::is_invocable<mutate_data_t, ref_wrapper_t>::value, "");
  static_assert(!detail::is_invocable<mutate_data_t, ref_wrapper_const_t>::value, "");

  static_assert(!detail::is_invocable_r<int&, call_operator_t, ref_wrapper_t, int, int>::value, "");

  auto adder = function_object_t{};
  REQUIRE_EQ(detail::invoke(&function_object_t::operator(), std::ref(adder), 40, 2), 42);
  REQUIRE_EQ(detail::invoke(&function_object_t::operator(), std::cref(adder), 40, 2), 42);
}

TEST_CASE("member function - object pointer")
{
  using call_operator_t = decltype(&function_object_t::operator());
  using mutate_data_t = decltype(&function_object_t::mutate_data);

  static_assert(detail::is_invocable<call_operator_t, function_object_t*, int const&, double>::value, "");
  static_assert(detail::is_invocable_r<int&&, call_operator_t, function_object_t*, int&, float>::value, "");

  // non-const member function
  static_assert(detail::is_invocable<mutate_data_t, function_object_t*>::value, "");
  static_assert(!detail::is_invocable<mutate_data_t, const function_object_t*>::value, "");

  static_assert(!detail::is_invocable_r<int&, call_operator_t, function_object_t*, int, int>::value, "");

  auto adder = function_object_t{};
  REQUIRE_EQ(detail::invoke(&function_object_t::operator(), &adder, 40, 2), 42);
}

TEST_CASE("member function - derived object reference")
{
  using call_operator_t = decltype(&function_object_t::operator());
  using mutate_data_t = decltype(&function_object_t::mutate_data);

  // should split all_qualifiers to get specific ones, right now it cannot
  // be used to test const objects and reference_wrapper.
  // Need to add make_index_sequence to do that properly.
  auto qualifiers =
      all_qualifiers(identity<int (function_object_t::*)(int, int)>{});
  static_assert(all_invocable<decltype(qualifiers), derived_function_object_t, int const&, double>::value, "");

  static_assert(detail::is_invocable<call_operator_t, derived_function_object_t, int const&, double>::value, "");
  static_assert(detail::is_invocable_r<int&&, call_operator_t, derived_function_object_t, int&, float>::value, "");

  // non-const member function
  static_assert(detail::is_invocable<mutate_data_t, derived_function_object_t&>::value, "");
  static_assert(!detail::is_invocable<mutate_data_t, const derived_function_object_t&>::value, "");

  static_assert(!detail::is_invocable_r<int&, call_operator_t, derived_function_object_t&, int, int>::value, "");

  auto adder = derived_function_object_t{};
  REQUIRE_EQ(detail::invoke(&function_object_t::operator(), adder, 40, 2), 42);
}

TEST_CASE("member function - reference_wrapper<derived object>")
{
  using call_operator_t = decltype(&function_object_t::operator());
  using mutate_data_t = decltype(&function_object_t::mutate_data);

  using ref_wrapper_t = std::reference_wrapper<derived_function_object_t>;
  using ref_wrapper_const_t = std::reference_wrapper<const derived_function_object_t>;

  static_assert(detail::is_invocable<call_operator_t, ref_wrapper_t, int const&, double>::value, "");
  static_assert(detail::is_invocable<call_operator_t, ref_wrapper_const_t, int const&, double>::value, "");
  static_assert(detail::is_invocable_r<int&&, call_operator_t, ref_wrapper_t, int&, float>::value, "");

  // non-const member function
  static_assert(detail::is_invocable<mutate_data_t, ref_wrapper_t>::value, "");
  static_assert(!detail::is_invocable<mutate_data_t, ref_wrapper_const_t>::value, "");

  static_assert(!detail::is_invocable_r<int&, call_operator_t, ref_wrapper_t&, int, int>::value, "");

  auto adder = derived_function_object_t{};
  REQUIRE_EQ(detail::invoke(&function_object_t::operator(), adder, 40, 2), 42);
}

TEST_CASE("member function - derived object pointer")
{
  using call_operator_t = decltype(&function_object_t::operator());
  using mutate_data_t = decltype(&function_object_t::mutate_data);

  static_assert(detail::is_invocable<call_operator_t, derived_function_object_t*, int const&, double>::value, "");
  static_assert(detail::is_invocable_r<int&&, call_operator_t, derived_function_object_t*, int&, float>::value, "");

  // non-const member function
  static_assert(detail::is_invocable<mutate_data_t, derived_function_object_t*>::value, "");
  static_assert(!detail::is_invocable<mutate_data_t, const derived_function_object_t*>::value, "");

  static_assert(!detail::is_invocable_r<int&, call_operator_t, derived_function_object_t*, int, int>::value, "");

  auto adder = derived_function_object_t{};
  REQUIRE_EQ(detail::invoke(&function_object_t::operator(), &adder, 40, 2), 42);
}

TEST_CASE("member data - object reference")
{
  using member_data_t = decltype(&function_object_t::i);

  static_assert(detail::is_invocable<member_data_t, function_object_t>::value, "");
  static_assert(detail::is_invocable_r<int&&, member_data_t, function_object_t>::value, "");

  // cannot convert lvalue ref to rvalue-reference
  static_assert(!detail::is_invocable_r<int&&, member_data_t, function_object_t&>::value, "");

  static_assert(!detail::is_invocable<member_data_t, function_object_t, int>::value, "");

  auto obj = function_object_t{};
  obj.i = 42;
  REQUIRE_EQ(detail::invoke(&function_object_t::i, obj), 42);
  REQUIRE_EQ(detail::invoke(std::mem_fn(&function_object_t::i), obj), 42);
}

TEST_CASE("member data - reference_wrapper<object>")
{
  using member_data_t = decltype(&function_object_t::i);
  using ref_wrapper_t = std::reference_wrapper<function_object_t>;
  using ref_wrapper_const_t = std::reference_wrapper<const function_object_t>;

  static_assert(detail::is_invocable<member_data_t, ref_wrapper_t>::value, "");
  static_assert(detail::is_invocable_r<int const&, member_data_t, ref_wrapper_const_t>::value, "");

  // cannot convert lvalue ref to rvalue-reference
  static_assert(!detail::is_invocable_r<int&&, member_data_t, ref_wrapper_t>::value, "");
  // nor from const lvalue reference to non-const lvalue reference
  static_assert(!detail::is_invocable_r<int&, member_data_t, ref_wrapper_const_t>::value, "");

  auto obj = function_object_t{};
  obj.i = 42;

  REQUIRE_EQ(detail::invoke(&function_object_t::i, std::ref(obj)), 42);
  REQUIRE_EQ(detail::invoke(std::mem_fn(&function_object_t::i), std::ref(obj)), 42);
  REQUIRE_EQ(detail::invoke(&function_object_t::i, std::cref(obj)), 42);
  REQUIRE_EQ(detail::invoke(std::mem_fn(&function_object_t::i), std::cref(obj)), 42);
}

TEST_CASE("member data - object pointer")
{
  using member_data_t = decltype(&function_object_t::i);

  static_assert(detail::is_invocable<member_data_t, function_object_t*>::value, "");
  static_assert(detail::is_invocable_r<int&, member_data_t, function_object_t*>::value, "");

  // cannot convert lvalue ref to rvalue-reference
  static_assert(!detail::is_invocable_r<int&&, member_data_t, function_object_t*>::value, "");
  static_assert(!detail::is_invocable_r<int&, member_data_t, const function_object_t*>::value, "");

  static_assert(!detail::is_invocable<member_data_t, function_object_t*, int>::value, "");

  auto obj = function_object_t{};
  obj.i = 42;
  REQUIRE_EQ(detail::invoke(&function_object_t::i, &obj), 42);
  REQUIRE_EQ(detail::invoke(std::mem_fn(&function_object_t::i), &obj), 42);
}

TEST_CASE("member data - derived object reference")
{
  using member_data_t = decltype(&function_object_t::i);

  static_assert(detail::is_invocable<member_data_t, derived_function_object_t>::value, "");
  static_assert(detail::is_invocable_r<int&&, member_data_t, derived_function_object_t>::value, "");

  // cannot convert lvalue ref to rvalue-reference
  static_assert(!detail::is_invocable_r<int&&, member_data_t, derived_function_object_t&>::value, "");

  static_assert(!detail::is_invocable<member_data_t, derived_function_object_t, int>::value, "");

  auto obj = derived_function_object_t{};
  obj.i = 42;
  REQUIRE_EQ(detail::invoke(&function_object_t::i, obj), 42);
  REQUIRE_EQ(detail::invoke(std::mem_fn(&function_object_t::i), obj), 42);
}

TEST_CASE("member data - reference_wrapper<derived object>")
{
  using member_data_t = decltype(&function_object_t::i);

  using ref_wrapper_t = std::reference_wrapper<derived_function_object_t>;
  using ref_wrapper_const_t = std::reference_wrapper<const derived_function_object_t>;

  static_assert(detail::is_invocable<member_data_t, ref_wrapper_t>::value, "");
  static_assert(detail::is_invocable_r<int const&, member_data_t, ref_wrapper_const_t>::value, "");

  // cannot convert lvalue ref to rvalue-reference
  static_assert(!detail::is_invocable_r<int&&, member_data_t, ref_wrapper_t>::value, "");
  // nor from const lvalue reference to non-const lvalue reference
  static_assert(!detail::is_invocable_r<int&, member_data_t, ref_wrapper_const_t>::value, "");

  auto obj = derived_function_object_t{};
  obj.i = 42;

  REQUIRE_EQ(detail::invoke(&function_object_t::i, std::ref(obj)), 42);
  REQUIRE_EQ(detail::invoke(std::mem_fn(&function_object_t::i), std::ref(obj)), 42);
  REQUIRE_EQ(detail::invoke(&function_object_t::i, std::cref(obj)), 42);
  REQUIRE_EQ(detail::invoke(std::mem_fn(&function_object_t::i), std::cref(obj)), 42);
}

TEST_CASE("member data - derived object pointer")
{
  using member_data_t = decltype(&function_object_t::i);

  static_assert(detail::is_invocable<member_data_t, derived_function_object_t*>::value, "");
  static_assert(detail::is_invocable_r<int&, member_data_t, derived_function_object_t*>::value, "");

  // cannot convert lvalue ref to rvalue-reference
  static_assert(!detail::is_invocable_r<int&&, member_data_t, derived_function_object_t*>::value, "");
  static_assert(!detail::is_invocable_r<int&, member_data_t, const derived_function_object_t*>::value, "");

  static_assert(!detail::is_invocable<member_data_t, derived_function_object_t*, int>::value, "");

  auto obj = derived_function_object_t{};
  obj.i = 42;
  REQUIRE_EQ(detail::invoke(&derived_function_object_t::i, &obj), 42);
  REQUIRE_EQ(detail::invoke(std::mem_fn(&derived_function_object_t::i), &obj), 42);
}

#ifdef CPP14TEST
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
TEST_CASE("generic lambda")
{
  auto add = [](auto a, auto b) { return a + b; };

  using lambda_t = decltype(add);

  static_assert(detail::is_invocable<lambda_t, int const&, double>::value, "");
  static_assert(detail::is_invocable_r<int&&, lambda_t, int&, float>::value, "");

  // compile error, static_assert doesn't trigger though
  // from cppreference: 
  //
  // Formally, determines whether INVOKE(declval<Fn>(),
  // declval<ArgTypes>()...) is well formed when treated as an unevaluated
  // operand, where INVOKE is the operation defined in Callable.
  //
  // This is indeed well-formed in the unevaluated context...
  // static_assert(!detail::is_invocable<lambda_t, int, std::string>::value, "");

  static_assert(!detail::is_invocable_r<int&, lambda_t, int, int>::value, "");

  REQUIRE_EQ(detail::invoke(add, 40, 2), 42);
}

TEST_CASE("transparent function objects")
{
  static_assert(detail::is_invocable<std::plus<>, int, int>::value, "");
  static_assert(detail::is_invocable<std::plus<>, int, float>::value, "");
  // this compiles because std::plus::operator() has on template type parameter,
  // whereas a generic lambda has 1 per auto parameter
  static_assert(!detail::is_invocable<std::plus<>, int, std::string>::value, "");

  REQUIRE_EQ(detail::invoke(std::plus<>{}, 40, 2), 42);
}

#pragma GCC diagnostic pop
#endif

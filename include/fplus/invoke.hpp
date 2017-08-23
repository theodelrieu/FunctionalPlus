// Copyright 2015, Tobias Hermann and the FunctionalPlus contributors.
// https://github.com/Dobiasd/FunctionalPlus
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <functional>
#include <type_traits>
#include <utility>

#include <fplus/meta.hpp>

namespace fplus
{
namespace detail
{
// We need std::invoke to detect callable objects
//
// source:
// http://en.cppreference.com/mwiki/index.php?title=cpp/utility/functional/invoke&oldid=82514
template <class U>
static std::true_type is_refwrap_test(const std::reference_wrapper<U>&);

template <class U>
static std::false_type is_refwrap_test(const U&);

template <class T>
struct is_reference_wrapper : decltype(is_refwrap_test(std::declval<T>()))
{
};

template <class Base, class T, class Derived, class... Args>
inline auto invoke_impl(T Base::*pmf, Derived&& ref, Args&&... args) ->
    typename std::enable_if<
        std::is_function<T>::value &&
            std::is_base_of<Base, typename std::decay<Derived>::type>::value,
        decltype((std::forward<Derived>(ref).*
                  pmf)(std::forward<Args>(args)...))>::type
{
  return (std::forward<Derived>(ref).*pmf)(std::forward<Args>(args)...);
}

template <class Base, class T, class RefWrap, class... Args>
inline auto invoke_impl(T Base::*pmf, RefWrap&& ref, Args&&... args) ->
    typename std::enable_if<
        std::is_function<T>::value &&
            is_reference_wrapper<typename std::decay<RefWrap>::type>::value,
        decltype((ref.get().*pmf)(std::forward<Args>(args)...))>::type
{
  return (ref.get().*pmf)(std::forward<Args>(args)...);
}

template <class Base, class T, class Pointer, class... Args>
inline auto invoke_impl(T Base::*pmf, Pointer&& ptr, Args&&... args) ->
    typename std::enable_if<
        std::is_function<T>::value &&
            !is_reference_wrapper<typename std::decay<Pointer>::type>::value &&
            !std::is_base_of<Base, typename std::decay<Pointer>::type>::value,
        decltype(((*std::forward<Pointer>(ptr)).*
                  pmf)(std::forward<Args>(args)...))>::type
{
  return ((*std::forward<Pointer>(ptr)).*pmf)(std::forward<Args>(args)...);
}

template <class Base, class T, class Derived>
inline auto invoke_impl(T Base::*pmd, Derived&& ref) -> typename std::enable_if<
    !std::is_function<T>::value &&
        std::is_base_of<Base, typename std::decay<Derived>::type>::value,
    decltype(std::forward<Derived>(ref).*pmd)>::type
{
  return std::forward<Derived>(ref).*pmd;
}

template <class Base, class T, class RefWrap>
inline auto invoke_impl(T Base::*pmd, RefWrap&& ref) -> typename std::enable_if<
    !std::is_function<T>::value &&
        is_reference_wrapper<typename std::decay<RefWrap>::type>::value,
    decltype(ref.get().*pmd)>::type
{
  return ref.get().*pmd;
}

template <class Base, class T, class Pointer>
inline auto invoke_impl(T Base::*pmd, Pointer&& ptr) -> typename std::enable_if<
    !std::is_function<T>::value &&
        !is_reference_wrapper<typename std::decay<Pointer>::type>::value &&
        !std::is_base_of<Base, typename std::decay<Pointer>::type>::value,
    decltype((*std::forward<Pointer>(ptr)).*pmd)>::type
{
  return (*std::forward<Pointer>(ptr)).*pmd;
}

template <class F, class... Args>
inline auto invoke_impl(F&& f, Args&&... args) -> typename std::enable_if<
    !std::is_member_pointer<typename std::decay<F>::type>::value,
    decltype(std::forward<F>(f)(std::forward<Args>(args)...))>::type
{
  return std::forward<F>(f)(std::forward<Args>(args)...);
}

template <typename AlwaysVoid, typename, typename...>
struct invoke_result_impl
{
};

template <typename F, typename... Args>
struct invoke_result_impl<decltype(void(invoke_impl(std::declval<F>(),
                                                    std::declval<Args>()...))),
                          F,
                          Args...>
{
  using type =
      decltype(invoke_impl(std::declval<F>(), std::declval<Args>()...));
};

template <class F, class... ArgTypes>
struct invoke_result : invoke_result_impl<void, F, ArgTypes...>
{
};

template <class F, class... Args>
using invoke_result_t = typename invoke_result<F, Args...>::type;

// noexcept omitted on purpose, very tedious to adapt libstdc++ code
// We could detect if C++17 is used and use the std::invoke directly.
//
// Otherwise, unless we want to forward noexcept-ness to every algorithm,
// I don't see a good reason to implement is_nothrow_invocable{_r}.
template <class F, class... ArgTypes>
invoke_result_t<F, ArgTypes...> invoke(F&& f, ArgTypes&&... args)
{
  return invoke_impl(std::forward<F>(f), std::forward<ArgTypes>(args)...);
}

// Invoke useful traits (libstdc++ 7.1.0's implementation, ugly-case removed)
template <typename Result, typename ReturnType, typename = void>
struct is_invocable_impl : std::false_type
{
};

template <typename Result, typename ReturnType>
struct is_invocable_impl<Result, ReturnType, void_t<typename Result::type>>
    : disjunction<std::is_void<ReturnType>,
                  std::is_convertible<typename Result::type, ReturnType>>::type
{
};

template <typename F, typename... ArgTypes>
struct is_invocable
    : is_invocable_impl<invoke_result<F, ArgTypes...>, void>::type
{
};

template <typename ReturnType, typename F, typename... ArgTypes>
struct is_invocable_r
    : is_invocable_impl<invoke_result<F, ArgTypes...>, ReturnType>::type
{
};
}
}

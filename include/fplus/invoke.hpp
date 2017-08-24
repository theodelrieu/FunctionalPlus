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

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"

#ifndef __clang__
#if defined(__GNUC__) || defined(__GNUG__)
#pragma GCC diagnostic ignored "-Wnoexcept-type"
#endif
#endif

struct invoke_member_function_ref
{
};

struct invoke_member_function_ptr
{
};

struct invoke_member_object_ref
{
};

struct invoke_member_object_ptr
{
};

struct invoke_other
{
};

template <typename T, typename U = typename std::decay<T>::type>
struct remove_reference_wrapper
{
  using type = T;
};

template <typename T, typename U>
struct remove_reference_wrapper<T, std::reference_wrapper<U>>
{
  using type = U&;
};

template <typename T>
using remove_reference_wrapper_t = typename remove_reference_wrapper<T>::type;

template <class Base, class T, class Derived, class... Args>
constexpr inline auto get_invoke_tag(T Base::*, Derived&&, Args&&...) ->
    typename std::enable_if<
        std::is_function<T>::value &&
            std::is_base_of<Base, typename std::decay<Derived>::type>::value,
        invoke_member_function_ref>::type
{
  return {};
}

template <class Base, class T, class RefWrap, class... Args>
constexpr inline auto get_invoke_tag(T Base::*, RefWrap&&, Args&&...) ->
    typename std::enable_if<
        std::is_function<T>::value &&
            is_reference_wrapper<typename std::decay<RefWrap>::type>::value,
        invoke_member_function_ref>::type
{
  return {};
}

template <class Base, class T, class Pointer, class... Args>
constexpr inline auto get_invoke_tag(T Base::*, Pointer&&, Args&&...) ->
    typename std::enable_if<
        std::is_function<T>::value &&
            !is_reference_wrapper<typename std::decay<Pointer>::type>::value &&
            !std::is_base_of<Base, typename std::decay<Pointer>::type>::value,
        invoke_member_function_ptr>::type
{
  return {};
}

template <class Base, class T, class Derived>
constexpr inline auto get_invoke_tag(T Base::*, Derived &&) ->
    typename std::enable_if<
        !std::is_function<T>::value &&
            std::is_base_of<Base, typename std::decay<Derived>::type>::value,
        invoke_member_object_ref>::type
{
  return {};
}

template <class Base, class T, class RefWrap>
constexpr inline auto get_invoke_tag(T Base::*, RefWrap &&) ->
    typename std::enable_if<
        !std::is_function<T>::value &&
            is_reference_wrapper<typename std::decay<RefWrap>::type>::value,
        invoke_member_object_ref>::type
{
  return {};
}

template <class Base, class T, class Pointer>
constexpr inline auto get_invoke_tag(T Base::*, Pointer &&) ->
    typename std::enable_if<
        !std::is_function<T>::value &&
            !is_reference_wrapper<typename std::decay<Pointer>::type>::value &&
            !std::is_base_of<Base, typename std::decay<Pointer>::type>::value,
        invoke_member_object_ptr>::type
{
  return {};
}

template <class F, class... Args>
constexpr inline auto get_invoke_tag(F&&, Args&&...) -> typename std::enable_if<
    !std::is_member_pointer<typename std::decay<F>::type>::value,
    invoke_other>::type
{
  return {};
}

template <typename F, typename... Args>
using get_invoke_tag_t =
    decltype(get_invoke_tag(std::declval<F>(), std::declval<Args>()...));

// noexcept check
template <typename MemberObjRef, typename Object>
constexpr inline bool is_call_noexcept(invoke_member_object_ref)
{
  return noexcept(
      static_cast<remove_reference_wrapper_t<Object>>(std::declval<Object>()).*
      std::declval<MemberObjRef>());
}

template <typename MemberObjPtr, typename ObjectPtr>
constexpr inline bool is_call_noexcept(invoke_member_object_ptr)
{
  return noexcept((*std::declval<ObjectPtr>()).*std::declval<MemberObjPtr>());
}

template <typename MemberFunctionRef, typename Object, typename... Args>
constexpr inline bool is_call_noexcept(invoke_member_function_ref)
{
  return noexcept(
      (static_cast<remove_reference_wrapper_t<Object>>(std::declval<Object>()).*
       std::declval<MemberFunctionRef>())(std::declval<Args>()...));
}

template <typename MemberFunctionPtr, typename ObjectPtr, typename... Args>
constexpr inline bool is_call_noexcept(invoke_member_function_ptr)
{
  return noexcept(((*std::declval<ObjectPtr>()).*
                   std::declval<MemberFunctionPtr>())(std::declval<Args>()...));
}

template <typename F, typename... Args>
constexpr inline bool is_call_noexcept(invoke_other)
{
  return noexcept((std::declval<F>())(std::declval<Args>()...));
}

// invoke_impl

template <typename MemberObjPtr, typename ObjectRef>
constexpr auto invoke_impl(invoke_member_object_ref,
                                  MemberObjPtr&& mObjPtr,
                                  ObjectRef&& obj)
    -> decltype(static_cast<remove_reference_wrapper_t<ObjectRef>&&>(obj).*
                mObjPtr)
{
  return (static_cast<remove_reference_wrapper_t<ObjectRef>&&>(obj)).*mObjPtr;
}

template <typename MemberObjPtr, typename ObjectPtr>
constexpr auto is_call_noexcept(invoke_member_object_ptr,
                                       MemberObjPtr&& mObjPtr,
                                       ObjectPtr&& obj)
    -> decltype((*std::forward<ObjectPtr>(obj)).*mObjPtr)
{
  return ((*std::forward<ObjectPtr>(obj)).*mObjPtr);
}

template <typename MemberFunctionPtr, typename ObjectRef, typename... Args>
constexpr inline auto invoke_impl(invoke_member_function_ref,
                                  MemberFunctionPtr&& mFunPtr,
                                  ObjectRef&& obj,
                                  Args&&... args)
    -> decltype((static_cast<remove_reference_wrapper_t<ObjectRef>&&>(obj).*
                 mFunPtr)(std::forward<Args>(args)...))
{
  return (static_cast<remove_reference_wrapper_t<ObjectRef>&&>(obj).*
          mFunPtr)(std::forward<Args>(args)...);
}

template <typename MemberFunctionPtr, typename ObjectPtr, typename... Args>
constexpr inline auto invoke_impl(invoke_member_function_ptr,
                                  MemberFunctionPtr&& mFunPtr,
                                  ObjectPtr&& obj,
                                  Args&&... args)
    -> decltype((*std::forward<ObjectPtr>(obj).*
                 mFunPtr)(std::forward<Args>(args)...))
{
  return (*std::forward<ObjectPtr>(obj).*mFunPtr)(std::forward<Args>(args)...);
}

template <typename F, typename... Args>
constexpr inline auto invoke_impl(invoke_other, F&& f, Args&&... args)
    -> decltype(std::forward<F>(f)(std::forward<Args>(args)...))
{
  return std::forward<F>(f)(std::forward<Args>(args)...);
}

template <typename AlwaysVoid, typename, typename...>
struct invoke_result_impl
{
};

template <typename F, typename... Args>
struct invoke_result_impl<decltype(void(invoke_impl(
                              std::declval<get_invoke_tag_t<F, Args...>>(),
                              std::declval<F>(),
                              std::declval<Args>()...))),
                          F,
                          Args...>
{
  using type =
      decltype(invoke_impl(std::declval<get_invoke_tag_t<F, Args...>>(),
                           std::declval<F>(),
                           std::declval<Args>()...));
};

template <class F, class... Args>
struct invoke_result : invoke_result_impl<void, F, Args...>
{
};

template <class F, class... Args>
using invoke_result_t = typename invoke_result<F, Args...>::type;

// Invoke useful traits (libstdc++ 7.1.0's implementation, ugly-case removed)
template <typename Result, typename ReturnType, typename = void>
struct is_invocable_impl : std::false_type
{
};

template <typename F, typename... Args>
struct is_noexcept : std::integral_constant<bool,
                                            is_call_noexcept<F, Args...>(
                                                get_invoke_tag_t<F, Args...>{})>
{
};

template <typename Result, typename ReturnType>
struct is_invocable_impl<Result, ReturnType, void_t<typename Result::type>>
    : disjunction<std::is_void<ReturnType>,
                  std::is_convertible<typename Result::type, ReturnType>>::type
{
};

template <typename F, typename... Args>
struct is_invocable
    : is_invocable_impl<invoke_result<F, Args...>, void>::type
{
};

template <typename ReturnType, typename F, typename... Args>
struct is_invocable_r
    : is_invocable_impl<invoke_result<F, Args...>, ReturnType>::type
{
};

template <typename F, typename... Args>
struct is_nothrow_invocable
    : conjunction<is_invocable_impl<invoke_result<F, Args...>, void>,
                  is_noexcept<F, Args...>>::type
{
};

template <typename ReturnType, typename F, typename... Args>
struct is_nothrow_invocable_r
    : conjunction<is_invocable_impl<invoke_result<F, Args...>, ReturnType>,
                  is_noexcept<F, Args...>>::type
{
};

// invoke
template <class F, class... Args>
invoke_result_t<F, Args...> invoke(F&& f, Args&&... args) noexcept(
    is_nothrow_invocable<F, Args...>::value)
{
  return invoke_impl(get_invoke_tag_t<F, Args...>{},
                     std::forward<F>(f),
                     std::forward<Args>(args)...);
}

#pragma GCC diagnostic pop

}
}

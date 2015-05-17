//
// Copyright (c) 2012-2015 Krzysztof Jusiak (krzysztof at jusiak dot net)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#ifndef BOOST_DI_CORE_INJECTOR_HPP
#define BOOST_DI_CORE_INJECTOR_HPP

#include "boost/di/aux_/compiler_specific.hpp"
#include "boost/di/aux_/type_traits.hpp"
#include "boost/di/aux_/utility.hpp"
#include "boost/di/core/any_type.hpp"
#include "boost/di/core/binder.hpp"
#include "boost/di/core/copyable.hpp"
#include "boost/di/core/policy.hpp"
#include "boost/di/core/pool.hpp"
#include "boost/di/core/provider.hpp"
#include "boost/di/core/transform.hpp"
#include "boost/di/core/wrapper.hpp"
#include "boost/di/scopes/exposed.hpp"
#include "boost/di/type_traits/ctor_traits.hpp"
#include "boost/di/type_traits/config_traits.hpp"
#include "boost/di/type_traits/referable_traits.hpp"
#include "boost/di/concepts/creatable.hpp"
#include "boost/di/config.hpp"

namespace boost { namespace di { inline namespace v1 { namespace core {

BOOST_DI_HAS_METHOD(call, call);

struct from_injector { };
struct from_deps { };
struct init { };

template<class T>
decltype(auto) arg(const T& arg, const std::false_type&) noexcept {
    return arg;
}

template<class T>
decltype(auto) arg(const T& arg, const std::true_type&) noexcept {
    return arg.configure();
}

template<class T, class TInjector>
inline auto build(const TInjector& injector) noexcept {
    return T{injector};
}

template<class TConfig, class TPolicies = pool<>, class... TDeps>
class injector : public pool<transform_t<TDeps...>>
               , public type_traits::config_traits_t<TConfig, injector<TConfig, TPolicies, TDeps...>>
               , _ {
    template<class...> friend struct provider;
    template<class> friend class scopes::exposed;
    template<class, class, class> friend struct any_type;
    template<class, class, class> friend struct any_type_ref;
    template<class...> friend struct try_provider;
    template<class...> friend struct successful::provider;
    template<class, class> friend struct successful::any_type;
    template<class, class> friend struct successful::any_type_ref;

    using pool_t = pool<transform_t<TDeps...>>;
    using is_root_t = std::true_type;
    using config_t = type_traits::config_traits_t<TConfig, injector>;
    using config = std::conditional_t<
        std::is_default_constructible<TConfig>::value
      , _
      , config_t
    >;

    template<
        class TIsRoot = std::false_type
      , class T
      , class TDependency = std::remove_reference_t<decltype(binder::resolve<T>((injector*)0))>
      , class TCtor = typename type_traits::ctor_traits<typename TDependency::given>::type
    > static auto try_create_impl(const aux::type<T>&) -> std::conditional_t<std::is_convertible<
       decltype(
           std::declval<TDependency>().template try_create<T>(
               try_provider<
                   typename TDependency::expected
                 , typename TDependency::given
                 , TCtor
                 , injector
               >{}
           )
       ), T>::value
       #if !defined(BOOST_DI_MSVC)
           && decltype(policy<pool_t>::template call<type_traits::referable_traits_t<T, TDependency>, no_name, TIsRoot>(
              ((TConfig*)0)->policies(), std::declval<TDependency>(), TCtor{}, std::false_type{})
           )::value
       #endif
       , T, void>;

    template<class TIsRoot = std::false_type, class TParent>
    static auto try_create_impl(const aux::type<any_type_fwd<TParent>>&) ->
        any_type<TParent, injector, std::true_type>;

    template<class TIsRoot = std::false_type, class TParent>
    static auto try_create_impl(const aux::type<any_type_ref_fwd<TParent>>&) ->
        any_type_ref<TParent, injector, std::true_type>;

    template<
        class TIsRoot = std::false_type
      , class T
      , class TName
      , class TDependency = std::remove_reference_t<decltype(binder::resolve<T, TName>((injector*)0))>
      , class TCtor = typename type_traits::ctor_traits<typename TDependency::given>::type
    > static auto try_create_impl(const aux::type<type_traits::named<TName, T>>&) -> std::conditional_t<std::is_convertible<
       decltype(
           std::declval<TDependency>().template try_create<T>(
               try_provider<
                   typename TDependency::expected
                 , typename TDependency::given
                 , TCtor
                 , injector
               >{}
           )
       ), T>::value
       #if !defined(BOOST_DI_MSVC)
           && decltype(policy<pool_t>::template call<type_traits::referable_traits_t<T, TDependency>, TName, TIsRoot>(
              ((TConfig*)0)->policies(), std::declval<TDependency>(), TCtor{}, std::false_type{})
           )::value
       #endif
       , T, void>;

                   public:
    static auto is_creatable_impl(...) -> std::false_type;

    template<class T, class TIsRoot>
    static auto is_creatable_impl(T&&, TIsRoot&&) ->
        aux::is_valid_expr<decltype(try_create_impl<TIsRoot>(aux::type<T>{}))>;

    template<class T, class TIsRoot = std::false_type>
    using is_creatable =
        #if defined(BOOST_DI_MSVC)
            std::true_type;
        #else
            decltype(is_creatable_impl(std::declval<T>(), std::declval<TIsRoot>()));
        #endif

public:
    using deps = transform_t<TDeps...>;

    template<class... TArgs>
    explicit injector(const init&, const TArgs&... args) noexcept
        : injector{from_deps{}, arg(args, has_configure<decltype(args)>{})...}
    { }

    template<class TConfig_, class TPolicies_, class... TDeps_>
    explicit injector(const injector<TConfig_, TPolicies_, TDeps_...>& other) noexcept
        : injector{from_injector{}, other, deps{}}
    { }

    template<class T, BOOST_DI_REQUIRES(is_creatable<T, is_root_t>::value)>
    T create() const {
        return create_successful_impl<is_root_t>(aux::type<T>{});
    }

    template<class T, BOOST_DI_REQUIRES(!is_creatable<T, is_root_t>::value)>
    BOOST_DI_CONCEPTS_CREATABLE_ATTR
    T create() const {
        return create_impl<is_root_t>(aux::type<T>{});
    }

    template<class TAction>
    void call(const TAction& action) {
        call_impl(action, deps{});
    }

private:
    template<class... TArgs>
    explicit injector(const from_deps&, const TArgs&... args) noexcept
        : pool_t{copyable<deps>{}, pool<aux::type_list<TArgs...>>{args...}}
        , config{*this}
    { }

    template<class TInjector, class... TArgs>
    explicit injector(const from_injector&, const TInjector& injector, const aux::type_list<TArgs...>&) noexcept
        : pool_t{copyable<deps>{}, pool_t{build<TArgs>(injector)...}}
        , config{*this}
    { }

    template<class TIsRoot = std::false_type, class T>
    auto create_impl(const aux::type<T>&) const {
        auto&& dependency = binder::resolve<T>((injector*)this);
        using dependency_t = std::remove_reference_t<decltype(dependency)>;
        using expected_t = typename dependency_t::expected;
        using given_t = typename dependency_t::given;
        using ctor_t = typename type_traits::ctor_traits<given_t>::type;
        using provider_t = core::provider<expected_t, given_t, no_name, ctor_t, injector>;
        using wrapper_t = decltype(dependency.template create<T>(provider_t{*this}));
        using create_t = type_traits::referable_traits_t<T, dependency_t>;
        policy<pool_t>::template call<create_t, no_name, TIsRoot>(((TConfig&)*this).policies(), dependency, ctor_t{}, std::true_type{});
        return wrapper<create_t, wrapper_t>{dependency.template create<T>(provider_t{*this})};
    }

    template<class TIsRoot = std::false_type, class TParent>
    auto create_impl(const aux::type<any_type_fwd<TParent>>&) const {
        return any_type<TParent, injector>{*this};
    }

    template<class TIsRoot = std::false_type, class TParent>
    auto create_impl(const aux::type<any_type_ref_fwd<TParent>>&) const {
        return any_type_ref<TParent, injector>{*this};
    }

    template<class TIsRoot = std::false_type, class T, class TName>
    auto create_impl(const aux::type<type_traits::named<TName, T>>&) const {
        auto&& dependency = binder::resolve<T, TName>((injector*)this);
        using dependency_t = std::remove_reference_t<decltype(dependency)>;
        using expected_t = typename dependency_t::expected;
        using given_t = typename dependency_t::given;
        using ctor_t = typename type_traits::ctor_traits<given_t>::type;
        using provider_t = core::provider<expected_t, given_t, TName, ctor_t, injector>;
        using wrapper_t = decltype(dependency.template create<T>(provider_t{*this}));
        using create_t = type_traits::referable_traits_t<T, dependency_t>;
        policy<pool_t>::template call<create_t, TName, TIsRoot>(((TConfig&)*this).policies(), dependency, ctor_t{}, std::true_type{});
        return wrapper<create_t, wrapper_t>{dependency.template create<T>(provider_t{*this})};
    }

    template<class TIsRoot = std::false_type, class T>
    auto create_successful_impl(const aux::type<T>&) const {
        auto&& dependency = binder::resolve<T>((injector*)this);
        using dependency_t = std::remove_reference_t<decltype(dependency)>;
        using expected_t = typename dependency_t::expected;
        using given_t = typename dependency_t::given;
        using ctor_t = typename type_traits::ctor_traits<given_t>::type;
        using provider_t = successful::provider<expected_t, given_t, ctor_t, injector>;
        using wrapper_t = decltype(dependency.template create<T>(provider_t{*this}));
        using create_t = type_traits::referable_traits_t<T, dependency_t>;
        policy<pool_t>::template call<create_t, no_name, TIsRoot>(((TConfig&)*this).policies(), dependency, ctor_t{}, std::true_type{});
        return successful::wrapper<create_t, wrapper_t>{dependency.template create<T>(provider_t{*this})};
    }

    template<class TIsRoot = std::false_type, class TParent>
    auto create_successful_impl(const aux::type<any_type_fwd<TParent>>&) const {
        return successful::any_type<TParent, injector>{*this};
    }

    template<class TIsRoot = std::false_type, class TParent>
    auto create_successful_impl(const aux::type<any_type_ref_fwd<TParent>>&) const {
        return successful::any_type_ref<TParent, injector>{*this};
    }

    template<class TIsRoot = std::false_type, class T, class TName>
    auto create_successful_impl(const aux::type<type_traits::named<TName, T>>&) const {
        auto&& dependency = binder::resolve<T, TName>((injector*)this);
        using dependency_t = std::remove_reference_t<decltype(dependency)>;
        using expected_t = typename dependency_t::expected;
        using given_t = typename dependency_t::given;
        using ctor_t = typename type_traits::ctor_traits<given_t>::type;
        using provider_t = successful::provider<expected_t, given_t, ctor_t, injector>;
        using wrapper_t = decltype(dependency.template create<T>(provider_t{*this}));
        using create_t = type_traits::referable_traits_t<T, dependency_t>;
        policy<pool_t>::template call<create_t, TName, TIsRoot>(((TConfig&)*this).policies(), dependency, ctor_t{}, std::true_type{});
        return successful::wrapper<create_t, wrapper_t>{dependency.template create<T>(provider_t{*this})};
    }

    template<class TAction, class... Ts>
    void call_impl(const TAction& action, const aux::type_list<Ts...>&) {
        int _[]{0, (call_impl<Ts>(action, has_call<Ts, const TAction&>{}), 0)...}; (void)_;
    }

    template<class T, class TAction>
    void call_impl(const TAction& action, const std::true_type&) {
        static_cast<T&>(*this).call(action);
    }

    template<class, class TAction>
    void call_impl(const TAction&, const std::false_type&) { }
};

template<class TConfig, class... TDeps>
class injector<TConfig, pool<>, TDeps...>
    : public pool<transform_t<TDeps...>>
    , public type_traits::config_traits_t<TConfig, injector<TConfig, pool<>, TDeps...>>
    , _ {
    template<class...> friend struct provider;
    template<class> friend class scopes::exposed;
    template<class, class, class> friend struct any_type;
    template<class, class, class> friend struct any_type_ref;
    template<class...> friend struct try_provider;
    template<class...> friend struct successful::provider;
    template<class, class> friend struct successful::any_type;
    template<class, class> friend struct successful::any_type_ref;

    using pool_t = pool<transform_t<TDeps...>>;
    using is_root_t = std::true_type;
    using config_t = type_traits::config_traits_t<TConfig, injector>;
    using config = std::conditional_t<
        std::is_default_constructible<TConfig>::value
      , _
      , config_t
    >;

    template<
        class TIsRoot = std::false_type
      , class T
      , class TDependency = std::remove_reference_t<decltype(binder::resolve<T>((injector*)0))>
      , class TCtor = typename type_traits::ctor_traits<typename TDependency::given>::type
    > static auto try_create_impl(const aux::type<T>&) -> std::conditional_t<std::is_convertible<
       decltype(
           std::declval<TDependency>().template try_create<T>(
               try_provider<
                   typename TDependency::expected
                 , typename TDependency::given
                 , TCtor
                 , injector
               >{}
           )
       ), T>::value, T, void>;

    template<class TIsRoot = std::false_type, class TParent>
    static auto try_create_impl(const aux::type<any_type_fwd<TParent>>&) ->
        any_type<TParent, injector, std::true_type>;

    template<class TIsRoot = std::false_type, class TParent>
    static auto try_create_impl(const aux::type<any_type_ref_fwd<TParent>>&) ->
        any_type_ref<TParent, injector, std::true_type>;

    template<
        class TIsRoot = std::false_type
      , class T
      , class TName
      , class TDependency = std::remove_reference_t<decltype(binder::resolve<T, TName>((injector*)0))>
      , class TCtor = typename type_traits::ctor_traits<typename TDependency::given>::type
    > static auto try_create_impl(const aux::type<type_traits::named<TName, T>>&) -> std::conditional_t<std::is_convertible<
       decltype(
           std::declval<TDependency>().template try_create<T>(
               try_provider<
                   typename TDependency::expected
                 , typename TDependency::given
                 , TCtor
                 , injector
               >{}
           )
       ), T>::value, T, void>;

        public:
    static auto is_creatable_impl(...) -> std::false_type;

    template<class T, class TIsRoot>
    static auto is_creatable_impl(T&&, TIsRoot&&) ->
        aux::is_valid_expr<decltype(try_create_impl<TIsRoot>(aux::type<T>{}))>;

    template<class T, class TIsRoot = std::false_type>
    using is_creatable =
        #if defined(BOOST_DI_MSVC)
            std::true_type;
        #else
            decltype(is_creatable_impl(std::declval<T>(), std::declval<TIsRoot>()));
        #endif

public:
    using deps = transform_t<TDeps...>;

    template<class... TArgs>
    explicit injector(const init&, const TArgs&... args) noexcept
        : injector{from_deps{}, arg(args, has_configure<decltype(args)>{})...}
    { }

    template<class TConfig_, class TPolicies_, class... TDeps_>
    explicit injector(const injector<TConfig_, TPolicies_, TDeps_...>& other) noexcept
        : injector{from_injector{}, other, deps{}}
    { }

    template<class T, BOOST_DI_REQUIRES(is_creatable<T, is_root_t>::value)>
    T create() const {
        return create_successful_impl<is_root_t>(aux::type<T>{});
    }

    template<class T, BOOST_DI_REQUIRES(!is_creatable<T, is_root_t>::value)>
    BOOST_DI_CONCEPTS_CREATABLE_ATTR
    T create() const {
        return create_impl<is_root_t>(aux::type<T>{});
    }

    template<class TAction>
    void call(const TAction& action) {
        call_impl(action, deps{});
    }

private:
    template<class... TArgs>
    explicit injector(const from_deps&, const TArgs&... args) noexcept
        : pool_t{copyable<deps>{}, pool<aux::type_list<TArgs...>>{args...}}
        , config{*this}
    { }

    template<class TInjector, class... TArgs>
    explicit injector(const from_injector&, const TInjector& injector, const aux::type_list<TArgs...>&) noexcept
        : pool_t{copyable<deps>{}, pool_t{build<TArgs>(injector)...}}
        , config{*this}
    { }

    template<class TIsRoot = std::false_type, class T>
    auto create_impl(const aux::type<T>&) const {
        auto&& dependency = binder::resolve<T>((injector*)this);
        using dependency_t = std::remove_reference_t<decltype(dependency)>;
        using expected_t = typename dependency_t::expected;
        using given_t = typename dependency_t::given;
        using ctor_t = typename type_traits::ctor_traits<given_t>::type;
        using provider_t = core::provider<expected_t, given_t, no_name, ctor_t, injector>;
        using wrapper_t = decltype(dependency.template create<T>(provider_t{*this}));
        using create_t = type_traits::referable_traits_t<T, dependency_t>;
        return wrapper<create_t, wrapper_t>{dependency.template create<T>(provider_t{*this})};
    }

    template<class TIsRoot = std::false_type, class TParent>
    auto create_impl(const aux::type<any_type_fwd<TParent>>&) const {
        return any_type<TParent, injector>{*this};
    }

    template<class TIsRoot = std::false_type, class TParent>
    auto create_impl(const aux::type<any_type_ref_fwd<TParent>>&) const {
        return any_type_ref<TParent, injector>{*this};
    }

    template<class TIsRoot = std::false_type, class T, class TName>
    auto create_impl(const aux::type<type_traits::named<TName, T>>&) const {
        auto&& dependency = binder::resolve<T, TName>((injector*)this);
        using dependency_t = std::remove_reference_t<decltype(dependency)>;
        using expected_t = typename dependency_t::expected;
        using given_t = typename dependency_t::given;
        using ctor_t = typename type_traits::ctor_traits<given_t>::type;
        using provider_t = successful::provider<expected_t, given_t, ctor_t, injector>;
        using wrapper_t = decltype(dependency.template create<T>(provider_t{*this}));
        using create_t = type_traits::referable_traits_t<T, dependency_t>;
        return successful::wrapper<create_t, wrapper_t>{dependency.template create<T>(provider_t{*this})};
    }

    template<class TIsRoot = std::false_type, class T>
    auto create_successful_impl(const aux::type<T>&) const {
        auto&& dependency = binder::resolve<T>((injector*)this);
        using dependency_t = std::remove_reference_t<decltype(dependency)>;
        using expected_t = typename dependency_t::expected;
        using given_t = typename dependency_t::given;
        using ctor_t = typename type_traits::ctor_traits<given_t>::type;
        using provider_t = successful::provider<expected_t, given_t, ctor_t, injector>;
        using wrapper_t = decltype(dependency.template create<T>(provider_t{*this}));
        using create_t = type_traits::referable_traits_t<T, dependency_t>;
        return successful::wrapper<create_t, wrapper_t>{dependency.template create<T>(provider_t{*this})};
    }

    template<class TIsRoot = std::false_type, class TParent>
    auto create_successful_impl(const aux::type<any_type_fwd<TParent>>&) const {
        return successful::any_type<TParent, injector>{*this};
    }

    template<class TIsRoot = std::false_type, class TParent>
    auto create_successful_impl(const aux::type<any_type_ref_fwd<TParent>>&) const {
        return successful::any_type_ref<TParent, injector>{*this};
    }

    template<class TIsRoot = std::false_type, class T, class TName>
    auto create_successful_impl(const aux::type<type_traits::named<TName, T>>&) const {
        auto&& dependency = binder::resolve<T, TName>((injector*)this);
        using dependency_t = std::remove_reference_t<decltype(dependency)>;
        using expected_t = typename dependency_t::expected;
        using given_t = typename dependency_t::given;
        using ctor_t = typename type_traits::ctor_traits<given_t>::type;
        using provider_t = successful::provider<expected_t, given_t, ctor_t, injector>;
        using wrapper_t = decltype(dependency.template create<T>(provider_t{*this}));
        using create_t = type_traits::referable_traits_t<T, dependency_t>;
        return successful::wrapper<create_t, wrapper_t>{dependency.template create<T>(provider_t{*this})};
    }

    template<class TAction, class... Ts>
    void call_impl(const TAction& action, const aux::type_list<Ts...>&) {
        int _[]{0, (call_impl<Ts>(action, has_call<Ts, const TAction&>{}), 0)...}; (void)_;
    }

    template<class T, class TAction>
    void call_impl(const TAction& action, const std::true_type&) {
        static_cast<T&>(*this).call(action);
    }

    template<class, class TAction>
    void call_impl(const TAction&, const std::false_type&) { }
};

}}}} // boost::di::v1::core

#endif


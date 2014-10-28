//
// Copyright (c) 2014 Krzysztof Jusiak (krzysztof at jusiak dot net)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#ifndef BOOST_DI_CORE_POOL_HPP
#define BOOST_DI_CORE_POOL_HPP

#include "boost/di/aux_/mpl.hpp"

#include <type_traits>

namespace boost {
namespace di {
namespace core {

struct init { };

template<typename = type_list<>>
class pool;

template<typename... TArgs>
class pool<type_list<TArgs...>> : public TArgs... {
public:
    using type = pool;

    template<typename... Ts>
    explicit pool(const Ts&... args)
        : Ts(args)...
    { }

    template<typename TPool>
    pool(const TPool& p, const init&)
        : pool(get<TArgs>(p)...)
    { (void)p; }

    template<typename T>
    inline const T& get() const {
        return static_cast<const T&>(*this);
    }

private:
    template<typename T, typename TPool>
    std::enable_if_t<std::is_base_of<T, pool>{} && std::is_base_of<T, TPool>{}, T>
    inline get(const TPool& p) const {
        return p.template get<T>();
    }

    template<typename T, typename TPool>
    std::enable_if_t<std::is_base_of<T, pool>{} && !std::is_base_of<T, TPool>{}, T>
    inline get(const TPool&) const {
        return T();
    }

    template<typename T, typename TPool>
    std::enable_if_t<!std::is_base_of<T, pool>{}, const T&>
    inline get(const TPool&) const {
        return T();
    }
};

} // namespace core
} // namespace di
} // namespace boost

#endif


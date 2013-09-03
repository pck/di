//
// Copyright (c) 2012 Krzysztof Jusiak (krzysztof at jusiak dot net)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#ifndef BOOST_DI_CONCEPTS_BIND_HPP
#define BOOST_DI_CONCEPTS_BIND_HPP

#include <boost/utility/enable_if.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/mpl/or.hpp>

#include "boost/di/concepts/annotate.hpp"
#include "boost/di/type_traits/is_same_base_of.hpp"
#include "boost/di/config.hpp"

namespace boost {
namespace di {
namespace concepts {

template<
    typename TExpected
  , typename TGiven
  , template<
        typename
      , typename T
      , typename = T
      , typename = mpl::vector0<>
      , typename = type_traits::is_same_base_of<T>
    > class TDependency
  , template<
        typename
      , typename
      , typename = void
    > class TNamed
>
struct bind
    : TDependency<
          mpl::_1
        , TExpected
        , TGiven
      >
    , annotate<>::with_<
          TExpected
        , mpl::vector0<>
      >
{
    template<BOOST_DI_TYPES_DEFAULT_MPL(T)>
    struct in_call
        : TDependency<
              mpl::_1
            , TExpected
            , TGiven
            , mpl::vector<BOOST_DI_TYPES_PASS_MPL(T)>
          >
        , annotate<>::with_<
              TExpected
            , mpl::vector<BOOST_DI_TYPES_PASS_MPL(T)>
          >
    {
        template<typename TName>
        struct in_name
            : TDependency<
                  mpl::_1
                , TNamed<TExpected, TName>
                , TGiven
                , mpl::vector<BOOST_DI_TYPES_PASS_MPL(T)>
              >
            , annotate<>::with_<
                  TNamed<TExpected, TName>
                , mpl::vector<BOOST_DI_TYPES_PASS_MPL(T)>
              >
        { };
    };

    template<typename TName>
    struct in_name
        : TDependency<
              mpl::_1
            , TNamed<TExpected, TName>
            , TGiven
          >
        , annotate<>::with_<
              TNamed<TExpected, TName>
          >
    {
        template<BOOST_DI_TYPES_DEFAULT_MPL(T)>
        struct in_call
            : TDependency<
                  mpl::_1
                , TNamed<TExpected, TName>
                , TGiven
                , mpl::vector<BOOST_DI_TYPES_PASS_MPL(T)>
              >
            , annotate<>::with_<
                  TNamed<TExpected, TName>
                , mpl::vector<BOOST_DI_TYPES_PASS_MPL(T)>
              >
        { };
    };
};

} // namespace concepts
} // namespace di
} // namespace boost

#endif


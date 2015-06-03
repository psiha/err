////////////////////////////////////////////////////////////////////////////////
///
/// \file errno.hpp
/// ---------------
///
/// Copyright (c) Domagoj Saric 2015.
///
/// Use, modification and distribution is subject to the
/// Boost Software License, Version 1.0.
/// (See accompanying file LICENSE_1_0.txt or copy at
/// http://www.boost.org/LICENSE_1_0.txt)
///
/// For more information, see http://www.boost.org
///
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
#ifndef errno_hpp__93D8479F_215F_49FB_823A_D7096D3A92B3
#define errno_hpp__93D8479F_215F_49FB_823A_D7096D3A92B3
#pragma once
//------------------------------------------------------------------------------
#include "boost/config.hpp"

#include <cerrno>
#include <cstdint>
#include <cstring>
#include <stdexcept>
//------------------------------------------------------------------------------
namespace boost
{
//------------------------------------------------------------------------------
namespace err
{
//------------------------------------------------------------------------------

struct last_errno
{
    using value_type = /*std*/::errno_t;

    static value_type const no_error = 0;

    BOOST_ATTRIBUTES( BOOST_COLD, BOOST_RESTRICTED_FUNCTION_L2, BOOST_EXCEPTIONLESS )
    static value_type BOOST_CC_REG get() { return /*std::*/errno; }
    BOOST_ATTRIBUTES( BOOST_COLD, BOOST_EXCEPTIONLESS )
    static void       BOOST_CC_REG set( value_type const value ) { errno = value; }

    BOOST_ATTRIBUTES( BOOST_COLD, BOOST_EXCEPTIONLESS )
    static void       BOOST_CC_REG clear() { return set( 0 ); }

    operator value_type () const { return get(); }

#ifndef NDEBUG
    last_errno() noexcept { BOOST_ASSERT_MSG( instance_counter++ == 0, "More than one last_errno instance detected (the last one overrides the previous ones)." ); }
   ~last_errno() noexcept { BOOST_ASSERT_MSG( instance_counter-- == 1, "More than one last_errno instance detected (the last one overrides the previous ones)." ); }
    static BOOST_THREAD_LOCAL_POD std::uint8_t instance_counter;
#endif // NDEBUG
}; // struct last_errno

#ifndef NDEBUG
BOOST_OVERRIDABLE_MEMBER_SYMBOL BOOST_THREAD_LOCAL_POD std::uint8_t last_errno::instance_counter( 0 );
#endif // NDEBUG


inline BOOST_ATTRIBUTES( BOOST_COLD )
std::runtime_error BOOST_CC_REG make_exception( last_errno )
{
    auto const error_code( last_errno::get() );
    BOOST_ASSERT_MSG( error_code != last_errno::no_error, "Throwing on no error?" );
    return std::runtime_error( std::strerror( error_code ) );
}

//------------------------------------------------------------------------------
} // namespace err
//------------------------------------------------------------------------------
} // namespace boost
//------------------------------------------------------------------------------
#endif // errno_hpp

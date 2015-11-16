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
#include <boost/config.hpp>

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

// http://www.virtsync.com/c-error-codes-include-errno

struct last_errno
{
    // http://stackoverflow.com/questions/24206989/error-use-of-undeclared-identifier-errno-t#24207339
#if defined( _MSC_VER ) || defined( __APPLE__ )
    using value_type = /*std*/::errno_t;
#else
    using value_type = int;
#endif // _MSC_VER || __APPLE__

    static value_type const no_error = 0;

    BOOST_ATTRIBUTES( BOOST_MINSIZE, BOOST_RESTRICTED_FUNCTION_L2, BOOST_EXCEPTIONLESS, BOOST_WARN_UNUSED_RESULT )
    static value_type BOOST_CC_REG get() { return /*std::*/errno; }
    BOOST_ATTRIBUTES( BOOST_COLD, BOOST_EXCEPTIONLESS )
    static void       BOOST_CC_REG set( value_type const value ) { errno = value; }

    BOOST_ATTRIBUTES( BOOST_MINSIZE, BOOST_EXCEPTIONLESS )
    static void       BOOST_CC_REG clear() { return set( 0 ); }

    BOOST_ATTRIBUTES( BOOST_MINSIZE, BOOST_RESTRICTED_FUNCTION_L2, BOOST_EXCEPTIONLESS, BOOST_WARN_UNUSED_RESULT )
    static bool       BOOST_CC_REG is( value_type const value ) { return get() == value; }
    template <value_type value>
    BOOST_ATTRIBUTES( BOOST_MINSIZE, BOOST_RESTRICTED_FUNCTION_L2, BOOST_EXCEPTIONLESS, BOOST_WARN_UNUSED_RESULT )
    static bool       BOOST_CC_REG is() { return is( value ); }

    operator value_type () const { return get(); }

#ifndef NDEBUG
    last_errno() noexcept { BOOST_ASSERT_MSG( instance_counter++ == 0, "More than one last_errno instance detected (the last one overrides the previous ones)." ); }
   ~last_errno() noexcept { BOOST_ASSERT_MSG( instance_counter-- == 1, "More than one last_errno instance detected (the last one overrides the previous ones)." ); }
    static BOOST_THREAD_LOCAL_POD std::uint8_t instance_counter;

    static /*thread_local*/ value_type const volatile & value; //...mrmlj...Apple still (OSX10.11/iOS9.1) hasn't implemented C++ TLS ...
#endif // NDEBUG
}; // struct last_errno

#ifndef NDEBUG
// GCC 4.9 from Android NDK 10e ICEs if BOOST_OVERRIDABLE_SYMBOL is put 'in the front'
BOOST_THREAD_LOCAL_POD std::uint8_t                            last_errno::instance_counter( 0 ) /*BOOST_OVERRIDABLE_MEMBER_SYMBOL*/        ;
/*thread_local*/       last_errno::value_type const volatile & last_errno::value                 /*BOOST_OVERRIDABLE_MEMBER_SYMBOL*/ = errno;
#endif // NDEBUG


#if defined( BOOST_MSVC )
#   pragma warning( push )
#   pragma warning( disable : 4996 ) // "The POSIX name for this item is deprecated. Instead, use the ISO C++ conformant name."
#endif // BOOST_MSVC
inline BOOST_ATTRIBUTES( BOOST_COLD )
std::runtime_error BOOST_CC_REG make_exception( last_errno )
{
    auto const error_code( last_errno::get() );
    BOOST_ASSERT_MSG( error_code != last_errno::no_error, "Throwing on no error?" );
    return std::runtime_error( std::strerror( error_code ) );
}
#if defined( BOOST_MSVC )
#   pragma warning( pop )
#endif // BOOST_MSVC

//------------------------------------------------------------------------------
} // namespace err
//------------------------------------------------------------------------------
} // namespace boost
//------------------------------------------------------------------------------
#endif // errno_hpp
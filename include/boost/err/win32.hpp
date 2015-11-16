////////////////////////////////////////////////////////////////////////////////
///
/// \file .hpp
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
#ifndef win32_hpp__ER092463_ED23_445A_9C11_5AA500DC59A8
#define win32_hpp__ER092463_ED23_445A_9C11_5AA500DC59A8
#pragma once
//------------------------------------------------------------------------------
#include <boost/config.hpp>
#include <boost/detail/winapi/error_handling.hpp>
#include <boost/detail/winapi/GetLastError.hpp>

//...mrmlj...for SetLastError...
#ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
#endif // WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
    #define NOMINMAX
#endif // NOMINMAX
#include <windows.h>

#include <cstdint>
#include <stdexcept>
//------------------------------------------------------------------------------
namespace boost
{
//------------------------------------------------------------------------------
namespace err
{
//------------------------------------------------------------------------------

BOOST_OPTIMIZE_FOR_SIZE_BEGIN()

// System Error Codes
// https://msdn.microsoft.com/en-us/library/windows/desktop/ms681382(v=vs.85).aspx

// ERROR_NOT_ENOUGH_MEMORY vs ERROR_OUTOFMEMORY
// http://unixwiz.net/techtips/not-enough-codes.html
// http://comp.os.ms-windows.programmer..narkive.com/UqDGBM2n/what-is-the-difference-between-error-not-enough-memory-and-error-outofmemory

struct last_win32_error
{
    using value_type = std::uint32_t;

    static value_type const no_error = 0;

    BOOST_ATTRIBUTES( BOOST_MINSIZE, BOOST_RESTRICTED_FUNCTION_L2, BOOST_EXCEPTIONLESS, BOOST_WARN_UNUSED_RESULT )
    static value_type BOOST_CC_REG get() { return boost::detail::winapi::GetLastError(); }
    BOOST_ATTRIBUTES( BOOST_COLD, BOOST_EXCEPTIONLESS )
    static void       BOOST_CC_REG set( value_type const value ) { return /*boost::detail::winapi*/::SetLastError( value ); }

    BOOST_ATTRIBUTES( BOOST_MINSIZE, BOOST_EXCEPTIONLESS )
    static void       BOOST_CC_REG clear() { return set( ERROR_SUCCESS ); }

    BOOST_ATTRIBUTES( BOOST_MINSIZE, BOOST_RESTRICTED_FUNCTION_L2, BOOST_EXCEPTIONLESS, BOOST_WARN_UNUSED_RESULT )
    static bool       BOOST_CC_REG is( value_type const value ) { return get() == value; }
    template <value_type value>
    BOOST_ATTRIBUTES( BOOST_MINSIZE, BOOST_RESTRICTED_FUNCTION_L2, BOOST_EXCEPTIONLESS, BOOST_WARN_UNUSED_RESULT )
    static bool       BOOST_CC_REG is() { return is( value ); }

    /// \todo interferes with result_or_error constructor SFINAE selection
    /// for Result types constructible from value_type. Cleanup...
    ///                                       (08.10.2015.) (Domagoj Saric)
    //operator value_type () const { return get(); }

#if !defined( NDEBUG )
    last_win32_error() noexcept { BOOST_ASSERT_MSG( instance_counter++ == 0, "More than one last_win32_error instance detected (the last one overrides the previous ones)." ); }
   ~last_win32_error() noexcept { BOOST_ASSERT_MSG( instance_counter-- <= 2, "More than one last_win32_error instance detected (the last one overrides the previous ones)." ); }
    last_win32_error( last_win32_error const  & ) = default; // delete;
    last_win32_error( last_win32_error       && ) = default;
    static BOOST_THREAD_LOCAL_POD std::uint8_t instance_counter;
#endif // NDEBUG
}; // struct last_win32_error

#if !defined( NDEBUG )
BOOST_OVERRIDABLE_MEMBER_SYMBOL BOOST_THREAD_LOCAL_POD std::uint8_t last_win32_error::instance_counter( 0 );
#endif // NDEBUG


inline BOOST_ATTRIBUTES( BOOST_COLD )
std::runtime_error BOOST_CC_REG make_exception( last_win32_error )
{
    using namespace boost::detail::winapi;

    auto const error_code( last_win32_error::get() );
    BOOST_ASSERT_MSG( error_code != last_win32_error::no_error, "Throwing on no error?" );

    char message[ 128 ];
    auto const message_length
    (
        ::FormatMessageA
        (
            FORMAT_MESSAGE_FROM_SYSTEM_ | FORMAT_MESSAGE_IGNORE_INSERTS_,
            nullptr, error_code, 0, message, _countof( message ), 0
        )
    );
    BOOST_ASSERT( message_length                           );
    BOOST_ASSUME( std::strlen( message ) == message_length );

    return std::runtime_error( message );
}

BOOST_OPTIMIZE_FOR_SIZE_END()

//------------------------------------------------------------------------------
} // namespace err
//------------------------------------------------------------------------------
} // namespace boost
//------------------------------------------------------------------------------
#endif // win32_hpp

////////////////////////////////////////////////////////////////////////////////
///
/// \file win32.hpp
/// ---------------
///
/// Copyright (c) Domagoj Saric 2015 - 2024.
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
#pragma once

#include <boost/assert.hpp>
#include <boost/config_ex.hpp>
#include <boost/winapi/error_handling.hpp>
#include <boost/winapi/get_last_error.hpp>

//...mrmlj...for SetLastError...
#ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
#endif // WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
    #define NOMINMAX
#endif // NOMINMAX
#include <windows.h>
#undef __bound

#include <cstdint>
#include <cstring>  // needed for std::strlen
#include <stdexcept>
//------------------------------------------------------------------------------
namespace psi::err
{
//------------------------------------------------------------------------------

BOOST_OPTIMIZE_FOR_SIZE_BEGIN()

// System Error Codes
// https://msdn.microsoft.com/en-us/library/windows/desktop/ms681382(v=vs.85).aspx

// ERROR_NOT_ENOUGH_MEMORY vs ERROR_OUTOFMEMORY
// http://unixwiz.net/techtips/not-enough-codes.html

struct last_win32_error
{
    using value_type = std::uint32_t;

    static value_type const no_error     = 0;
    static value_type const invalid_data = ERROR_INVALID_DATA;

    BOOST_ATTRIBUTES( BOOST_COLD, BOOST_EXCEPTIONLESS, BOOST_RESTRICTED_FUNCTION_L2, BOOST_WARN_UNUSED_RESULT )
    static value_type get(                        ) noexcept { return   boost::winapi  ::GetLastError(       ); }
    BOOST_ATTRIBUTES( BOOST_COLD, BOOST_EXCEPTIONLESS )
    static void       set( value_type const value ) noexcept { return /*boost::winapi*/::SetLastError( value ); }

    BOOST_ATTRIBUTES( BOOST_MINSIZE, BOOST_EXCEPTIONLESS )
    static void       clear() noexcept { return set( ERROR_SUCCESS ); }

    BOOST_ATTRIBUTES( BOOST_MINSIZE, BOOST_RESTRICTED_FUNCTION_L2, BOOST_EXCEPTIONLESS, BOOST_WARN_UNUSED_RESULT )
    static bool       is( value_type const value ) noexcept { return get() == value; }
    template <value_type value>
    BOOST_ATTRIBUTES( BOOST_MINSIZE, BOOST_RESTRICTED_FUNCTION_L2, BOOST_EXCEPTIONLESS, BOOST_WARN_UNUSED_RESULT )
    static bool       is() noexcept { return is( value ); }

    /// \todo Interferes with result_or_error constructor SFINAE selection
    /// for Result types constructible from value_type if implicit. Cleanup...
    ///                                       (08.10.2015.) (Domagoj Saric)
    explicit
    operator value_type() const noexcept { return value/*get()*/; }

    /// \note In order to support multiple/coexisting last-err objects (i.e.
    /// multiple fallible_results saved to result_or_error objects) we have to
    /// add state (i.e. save the current error on construction).
    ///                                       (28.02.2016.) (Domagoj Saric)
    value_type const value = last_win32_error::get();

#if !defined( NDEBUG ) && 0 // disabled (no longer stateless)
    last_win32_error() noexcept { BOOST_ASSERT_MSG( instance_counter++ == 0, "More than one last_win32_error instance detected (the last one overrides the previous ones)." ); }
   ~last_win32_error() noexcept { BOOST_ASSERT_MSG( instance_counter-- <= 2, "More than one last_win32_error instance detected (the last one overrides the previous ones)." ); }
    last_win32_error( last_win32_error const  & ) = default; // delete;
    last_win32_error( last_win32_error       && ) = default;
    static thread_local std::uint8_t instance_counter;
#endif // NDEBUG
}; // struct last_win32_error

#if !defined( NDEBUG ) && 0 // disabled (no longer stateless)
inline thread_local std::uint8_t last_win32_error::instance_counter( 0 );
#endif // NDEBUG


inline BOOST_ATTRIBUTES( BOOST_COLD )
std::runtime_error make_exception( last_win32_error const error )
{
    using namespace boost::winapi;

    BOOST_ASSERT_MSG( error.value != last_win32_error::no_error, "Throwing on no error?" );

    char message[ 128 ];
    auto const message_length
    (
        ::FormatMessageA
        (
            FORMAT_MESSAGE_FROM_SYSTEM_ | FORMAT_MESSAGE_IGNORE_INSERTS_,
            nullptr, error.value, 0, message, _countof( message ), 0
        )
    );
#ifdef __clang__
#   pragma clang diagnostic push
#   pragma clang diagnostic ignored "-Wassume"
#endif
    BOOST_ASSUME( message_length                           );
#ifndef _MSC_VER // VS 17.10.0 MSVC ICE
    BOOST_ASSUME( std::strlen( message ) == message_length );
#endif
#ifdef __clang__
#   pragma clang diagnostic pop
#endif

    return std::runtime_error( message );
}

BOOST_OPTIMIZE_FOR_SIZE_END()

//------------------------------------------------------------------------------
} // namespace psi::err
//------------------------------------------------------------------------------

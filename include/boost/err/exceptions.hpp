////////////////////////////////////////////////////////////////////////////////
///
/// \file exceptios.hpp
/// -------------------
///
/// Copyright (c) Domagoj Saric 2015 - 2020.
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
#ifndef exceptions_hpp__15955CEE_978C_45BB_BB81_EFF1D165B311
#define exceptions_hpp__15955CEE_978C_45BB_BB81_EFF1D165B311
#pragma once
//------------------------------------------------------------------------------
#include <boost/config_ex.hpp>
#include <boost/throw_exception.hpp>

#include <exception>
#include <type_traits>
#include <utility>

#ifdef __APPLE__
#include <Availability.h>
#include <TargetConditionals.h>
#endif

//------------------------------------------------------------------------------
namespace boost
{
//------------------------------------------------------------------------------
namespace err
{
//------------------------------------------------------------------------------

BOOST_OPTIMIZE_FOR_SIZE_BEGIN()

////////////////////////////////////////////////////////////////////////////////
//
// boost::err::make_exception()
// ----------------------------
//
////////////////////////////////////////////////////////////////////////////////
///
/// \brief Transform an error code/object into a corresponding exception.
///
/// \detail Intended to be specialised or overloaded for user types. Boost.Err
/// will call it unqualified in order to allow ADL to kick in.
///
/// \throws nothing
///
////////////////////////////////////////////////////////////////////////////////

template <class Error>
Error && make_exception( Error && error ) { return std::forward<Error>( error ); }


template <class Exception>
[[noreturn]] BOOST_ATTRIBUTES( BOOST_COLD )
typename std::enable_if<!std::is_fundamental<Exception>::value>::type
BOOST_CC_REG throw_exception( Exception && exception ) { BOOST_THROW_EXCEPTION( std::forward<Exception>( exception ) ); }

template <typename Exception>
[[noreturn]] BOOST_ATTRIBUTES( BOOST_COLD )
typename std::enable_if<std::is_fundamental<Exception>::value>::type
BOOST_CC_REG throw_exception( Exception const exception ) { BOOST_THROW_EXCEPTION( exception ); }

template <typename Error>
[[noreturn]] BOOST_ATTRIBUTES( BOOST_COLD )
void BOOST_CC_REG make_and_throw_exception( Error && error ) { throw_exception( std::move( make_exception( std::forward<Error>( error ) ) ) ); }

template <typename Error>
[[noreturn]] BOOST_ATTRIBUTES( BOOST_COLD )
void BOOST_CC_REG make_and_throw_exception() { make_and_throw_exception<Error>( Error() ); }

template <typename Error>
BOOST_ATTRIBUTES( BOOST_COLD )
std::exception_ptr BOOST_CC_REG make_exception_ptr( Error && error ) { return std::make_exception_ptr( make_exception( std::forward<Error>( error ) ) ); }

namespace detail
{
    inline auto uncaught_exceptions() noexcept
    {
    #if ( __cpp_lib_uncaught_exceptions && !defined( __APPLE__ ) ) || ( __cpp_lib_uncaught_exceptions && defined( __APPLE__ ) && defined( TARGET_OS_IPHONE ) && __IPHONE_OS_VERSION_MIN_REQUIRED >= __IPHONE_10_0 )
        return std::uncaught_exceptions();
    #else
        return std::uncaught_exception();
    #endif // __cpp_lib_uncaught_exceptions
    }

    template <typename Error>
    BOOST_ATTRIBUTES( BOOST_COLD )
    void BOOST_CC_REG conditional_throw( Error && error )
    {
        if ( BOOST_LIKELY( !uncaught_exceptions() ) )
            make_and_throw_exception( std::move( error ) );
    }
} // namespace detail


/// \note Reuse preexisting compiler-specific implementations.
///                                           (28.02.2016.) (Domagoj Saric)
#ifdef BOOST_MSVC
__if_exists( std::_Xbad_alloc )
{
    template <> [[noreturn]] BOOST_ATTRIBUTES( BOOST_COLD )
    inline void BOOST_CC_REG make_and_throw_exception<std::bad_alloc>() { std::_Xbad_alloc(); }
}
#endif // BOOST_MSVC

BOOST_OPTIMIZE_FOR_SIZE_END()

//------------------------------------------------------------------------------
} // namespace err
//------------------------------------------------------------------------------
} // namespace boost
//------------------------------------------------------------------------------
#endif // exceptions_hpp

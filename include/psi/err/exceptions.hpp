////////////////////////////////////////////////////////////////////////////////
///
/// \file exceptios.hpp
/// -------------------
///
/// Copyright (c) Domagoj Saric 2015 - 2025.
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
namespace psi::err
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
////////////////////////////////////////////////////////////////////////////////

template <class Error>
Error && make_exception( Error && error ) { return std::forward<Error>( error ); }


template <class Exception>
[[ noreturn ]] BOOST_ATTRIBUTES( BOOST_COLD )
void BOOST_CC_REG throw_exception( Exception && exception ) requires( !std::is_fundamental_v<Exception> )
{
    if constexpr ( std::convertible_to<Exception, std::exception const &> )
        BOOST_THROW_EXCEPTION( std::forward<Exception>( exception ) );
    else
        throw std::forward<Exception>( exception );
}

template <typename Exception>
[[noreturn]] BOOST_ATTRIBUTES( BOOST_COLD )
void BOOST_CC_REG throw_exception( Exception const exception ) requires( std::is_fundamental_v<Exception> ) { throw exception; }

template <typename Error>
[[noreturn]] BOOST_ATTRIBUTES( BOOST_COLD )
void BOOST_CC_REG make_and_throw_exception( Error && error ) { throw_exception( std::move( make_exception( std::forward<Error>( error ) ) ) ); }

template <typename Error>
[[noreturn]] BOOST_ATTRIBUTES( BOOST_COLD )
void BOOST_CC_REG make_and_throw_exception() { make_and_throw_exception<Error>( Error{} ); }

template <typename Error>
BOOST_ATTRIBUTES( BOOST_COLD )
std::exception_ptr BOOST_CC_REG make_exception_ptr( Error && error ) { return std::make_exception_ptr( make_exception( std::forward<Error>( error ) ) ); }

namespace detail
{
    template <typename Error>
    BOOST_ATTRIBUTES( BOOST_COLD )
    void BOOST_CC_REG conditional_throw( Error && error )
    {
        if ( BOOST_LIKELY( !std::uncaught_exceptions() ) )
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
} // namespace psi::err
//------------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
///
/// \file exceptios.hpp
/// -------------------
///
/// Copyright (c) Domagoj Saric 2015 - 2016.
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
#include <boost/config.hpp>
#include <boost/throw_exception.hpp>

#include <exception>
#include <type_traits>
#include <utility>
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
BOOST_ATTRIBUTES( BOOST_DOES_NOT_RETURN, BOOST_COLD )
typename std::enable_if<!std::is_fundamental<Exception>::value>::type
BOOST_CC_REG throw_exception( Exception && exception ) { BOOST_THROW_EXCEPTION( std::forward<Exception>( exception ) ); }

template <typename Exception>
BOOST_ATTRIBUTES( BOOST_DOES_NOT_RETURN, BOOST_COLD )
typename std::enable_if<std::is_fundamental<Exception>::value>::type
BOOST_CC_REG throw_exception( Exception const exception ) { BOOST_THROW_EXCEPTION( exception ); }

template <typename Error>
BOOST_ATTRIBUTES( BOOST_DOES_NOT_RETURN, BOOST_COLD )
void BOOST_CC_REG make_and_throw_exception( Error && error ) { throw_exception( std::move( make_exception( std::forward<Error>( error ) ) ) ); }

template <typename Error>
BOOST_ATTRIBUTES( BOOST_DOES_NOT_RETURN, BOOST_COLD )
void BOOST_CC_REG make_and_throw_exception() { make_and_throw_exception<Error>( Error() ); }

template <typename Error>
BOOST_ATTRIBUTES( BOOST_COLD )
std::exception_ptr BOOST_CC_REG make_exception_ptr( Error && error ) { return std::make_exception_ptr( make_exception( std::forward<Error>( error ) ) ); }

namespace detail
{
    template <typename Error>
    BOOST_ATTRIBUTES( BOOST_COLD )
    void BOOST_CC_REG conditional_throw( Error && error )
    {
        if ( BOOST_LIKELY( !std::uncaught_exception() ) )
            make_and_throw_exception( std::move( error ) );
    }
} // namespace detail

BOOST_OPTIMIZE_FOR_SIZE_END()

//------------------------------------------------------------------------------
} // namespace err
//------------------------------------------------------------------------------
} // namespace boost
//------------------------------------------------------------------------------
#endif // exceptions_hpp

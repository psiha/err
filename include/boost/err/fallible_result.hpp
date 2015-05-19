////////////////////////////////////////////////////////////////////////////////
///
/// \file fallible_result.hpp
/// -------------------------
///
/// Copyright (c) Domagoj Saric 2015.
///
///  Use, modification and distribution is subject to the
///  Boost Software License, Version 1.0.
///  (See accompanying file LICENSE_1_0.txt or copy at
///  http://www.boost.org/LICENSE_1_0.txt)
///
/// For more information, see http://www.boost.org
///
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
#ifndef fallible_result_hpp__568DC1C1_A422_4EE6_9993_8FC98EE846C8
#define fallible_result_hpp__568DC1C1_A422_4EE6_9993_8FC98EE846C8
#pragma once
//------------------------------------------------------------------------------
#include "result_or_error.hpp"

#include "boost/config.hpp"

#include <type_traits>
#include <utility>
//------------------------------------------------------------------------------
namespace boost
{
//------------------------------------------------------------------------------
namespace err
{
//------------------------------------------------------------------------------


////////////////////////////////////////////////////////////////////////////////
///
/// \class fallible_result
///
/// \brief A result_or_error wrapper that can either:
/// - trivially move-convert/'decay' to result_or_error (error handling mode)
/// - try-convert to Result for a successful call or throw the Error object
///   otherwise (exception handling mode).
///
////////////////////////////////////////////////////////////////////////////////

template <class Result, class Error>
class fallible_result
{
public:
	template <typename T>
	fallible_result( T && argument ) noexcept( std::is_nothrow_move_constructible<Result>::value ) : result_or_error_( std::forward<T>( argument ) ) {}

	~fallible_result() noexcept( false )
	{
		result_or_error_.throw_if_uninspected_error();
		BOOST_ASSUME( result_or_error_.inspected_ );
	};

	operator result_or_error<Result, Error> && () && noexcept { return std::move( result_or_error_ ); }
	operator Result                         && () &&          { return std::move( result()         ); }

	Result & operator *  () { return  result(); }
	Result * operator -> () { return &result(); }

	void ignore() { result_or_error_.inspected_ = true; }

private:
	Result && result()
	{
		result_or_error_.throw_if_error();
		BOOST_ASSUME( result_or_error_.inspected_ );
		BOOST_ASSUME( result_or_error_.succeeded_ );
		return static_cast<Result &&>( result_or_error_.result_ );
	}

private:
	result_or_error<Result, Error> result_or_error_;
}; // class fallible_result

//------------------------------------------------------------------------------
} // namespace err
//------------------------------------------------------------------------------
} // namespace boost
//------------------------------------------------------------------------------
#endif // fallible_result_hpp

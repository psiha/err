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
#include "fallible_result.hpp"

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

template <class Result, class Error>
class fallible_result
{
public:
	template <typename T>
	fallible_result( T && argument ) : result_or_error_( std::forward<T>( argument ) ) noexcept( std::is_nothrow_move_constructible<Result>::value ) {}

	~fallible_result() noexcept( false )
	{
		result_or_error_.throw_if_uninspected_error();
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

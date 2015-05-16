////////////////////////////////////////////////////////////////////////////////
///
/// \file result_or_error.hpp
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
#ifndef result_or_error_hpp__3A4D7BDA_D64A_456B_AA06_82E407BB8EAB
#define result_or_error_hpp__3A4D7BDA_D64A_456B_AA06_82E407BB8EAB
#pragma once
//------------------------------------------------------------------------------
#include "boost/assert.hpp"
#include "boost/config.hpp"
#include "boost/throw_exception.hpp"

#include <exception>
#include <type_traits>
//------------------------------------------------------------------------------
namespace boost
{
//------------------------------------------------------------------------------
namespace err
{
//------------------------------------------------------------------------------

template <class Result, class Error>
class fallible_result;

template <class Result, class Error>
class result_or_error
{
public:
	result_or_error( Result && result ) : succeeded_( true  ), inspected_( false ), result_( std::move( result ) ) noexcept( std::is_nothrow_move_constructible<Result>::value ) {}
	result_or_error( Error  && error  ) : succeeded_( false ), inspected_( false ), error_ ( std::move( error  ) ) noexcept( std::is_nothrow_move_constructible<Error >::value ) {}
#if 0
	result_or_error( result_or_error const & other )
		: succeeded_( other.succedded_ )
	{
		if ( succeeded_ )
			new ( &result_ ) Result( other.result_ );
		else
			new ( &error_  ) Error ( other.error_  );
	}
#endif
	result_or_error( result_or_error && other )
		noexcept
		(
			std::is_nothrow_move_constructible<Result>::value &&
			std::is_nothrow_move_constructible<Error >::value
		)
		: succeeded_( other.succeeded() ), inspected_( false )
	{
		if ( BOOST_LIKELY( succeeded_ ) )
			new ( &result_ ) Result( std::move( other.result_ ) );
		else
			new ( &error_  ) Error ( std::move( other.error_  ) );
	}

	~result_or_error() noexcept( false )
	{
		//BOOST_ASSERT_MSG( inspected(), "Ignored error return code." );
		if ( succeeded() )
			result_.~Result();
		else
			error_.~Error();
	};

	Error  const & error ()       { BOOST_ASSERT_MSG( !succeeded_, "Querying the error of a succeeded operation." ); return error_ ; }
	Result       & result()       { BOOST_ASSERT_MSG(  succeeded_, "Querying the result of a failed operation."   ); return result_; }
	Result const & result() const { return const_cast<Result &>( *this ).result(); }

					  bool inspected() const { return inspected_; }
		              bool succeeded() const { inspected_ = true; return BOOST_LIKELY( succeeded_ ); }
	explicit operator bool          () const { return succeeded(); }

	void throw_if_error()
	{
		if ( !succeeded() )
			throw_error();
	}

	void throw_if_uninspected_error()
	{
		if ( !inspected() )
			throw_if_error();
	}

	BOOST_ATTRIBUTE_NORETURN
	void throw_error()
	{
		BOOST_ASSERT( !succeeded() );
		BOOST_ASSERT( !std::uncaught_exception() )
		BOOST_THROW_EXCEPTION( std::move( error_ ) );
	}

protected:
	        bool succeeded_;
	mutable bool inspected_;

private: friend class fallible_result<Result, Error>;
	union
	{
		Result result_;
		Error  error_ ;
	};
}; // class result_or_error

//------------------------------------------------------------------------------
} // namespace err
//------------------------------------------------------------------------------
} // namespace boost
//------------------------------------------------------------------------------
#endif // result_or_error_hpp

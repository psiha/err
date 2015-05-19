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

template <class Error>
BOOST_NORETURN BOOST_COLD
typename std::enable_if<!std::is_fundamental<Error>::value>::type
make_and_throw_exception( Error && error ) { BOOST_THROW_EXCEPTION( std::forward( error ) ); }

template <typename Error>
BOOST_NORETURN BOOST_COLD
typename std::enable_if<std::is_fundamental<Error>::value>::type
make_and_throw_exception( Error const error ) { throw error; }


////////////////////////////////////////////////////////////////////////////////
///
/// \class result_or_error
///
/// \brief A discriminated union of a possible Result object or a possible
/// Error object
///
////////////////////////////////////////////////////////////////////////////////

template <class Result, class Error>
class fallible_result;

template <class Result, class Error>
class result_or_error
{
public:
	/// \note Be liberal with the constructor argument type in order to allow
	/// emplacement/eliminate an intermediate move constructor call (as it can
	/// actually be nontriviall, e.g. with MSVC14 RC std::string).
	///                                       (18.05.2015.) (Domagoj Saric)
	template <typename Source> result_or_error( Source && BOOST_RESTRICTED_REF result, typename std::enable_if<std::is_constructible<Result, Source &&>::value>::type const * = nullptr ) noexcept( std::is_nothrow_move_constructible<Result>::value ) : succeeded_( true  ), inspected_( false ), result_( std::forward<Source>( result ) ) {}
	template <typename Source> result_or_error( Source && BOOST_RESTRICTED_REF error , typename std::enable_if<std::is_constructible<Error , Source &&>::value>::type const * = nullptr ) noexcept( std::is_nothrow_move_constructible<Error >::value ) : succeeded_( false ), inspected_( false ), error_ ( std::forward<Source>( error  ) ) {}
#if 0 // disabled copy construction
	result_or_error( result_or_error const & other )
		: succeeded_( other.succedded_ )
	{
		if ( succeeded_ )
			new ( &result_ ) Result( other.result_ );
		else
			new ( &error_  ) Error ( other.error_  );
	}
#endif // disabled copy construction
	result_or_error( result_or_error && BOOST_RESTRICTED_REF other )
		noexcept
		(
			std::is_nothrow_move_constructible<Result>::value &&
			std::is_nothrow_move_constructible<Error >::value
		)
		: succeeded_( other.succeeded() ), inspected_( false )
	{
		/// \note MSVC14 RC still cannot 'see through' placement new and inserts
		/// braindead branching code so we help it with assume statements.
		///                                   (18.05.2015.) (Domagoj Saric)
		if ( BOOST_LIKELY( succeeded_ ) )
		{
			auto const ptr( new ( &result_ ) Result( std::move( other.result_ ) ) );
			BOOST_ASSUME( ptr );
			BOOST_ASSUME( other.succeeded_ );
		}
		else
		{
			auto const ptr( new ( &error_  ) Error ( std::move( other.error_  ) ) );
			BOOST_ASSUME( ptr );
			BOOST_ASSUME( !other.succeeded_ );
		}
		BOOST_ASSUME( other.inspected_ );
	}

	~result_or_error() noexcept( false )
	{
		//BOOST_ASSERT_MSG( inspected(), "Ignored error return code." );
		if ( BOOST_LIKELY( succeeded_ ) )
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
		BOOST_ASSERT( !std::uncaught_exception() );
		if ( BOOST_LIKELY( succeeded() ) )
		{
			BOOST_ASSUME( inspected_ );
			BOOST_ASSUME( succeeded_ );
			return;
		}
		BOOST_ASSUME( inspected_ );
		throw_error();
	}

	void throw_if_uninspected_error()
	{
		if ( !inspected() )
		{
			throw_if_error();
			BOOST_ASSUME( succeeded_ );
		}
		BOOST_ASSUME( inspected_ );
	}

	BOOST_NORETURN BOOST_COLD
	void throw_error()
	{
		BOOST_ASSERT( !succeeded() );
		BOOST_ASSERT( !std::uncaught_exception() );
		make_and_throw_exception( std::move( error_ ) );
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

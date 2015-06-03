////////////////////////////////////////////////////////////////////////////////
///
/// \file result_or_error.hpp
/// -------------------------
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

struct no_err_t {};
static no_err_t const no_err    = {};
static no_err_t const success   = {};
static no_err_t const succeeded = {};

struct an_err_t {};
static an_err_t const an_err  = {};
static an_err_t const failure = {};
static an_err_t const failed  = {};

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

namespace detail
{
    template <class Exception>
    BOOST_ATTRIBUTES( BOOST_DOES_NOT_RETURN, BOOST_COLD )
    typename std::enable_if<!std::is_fundamental<Exception>::value>::type
    BOOST_CC_REG throw_exception( Exception && exception ) { BOOST_THROW_EXCEPTION( std::forward<Exception>( exception ) ); }

    template <typename Exception>
    BOOST_ATTRIBUTES( BOOST_DOES_NOT_RETURN, BOOST_COLD )
    typename std::enable_if<std::is_fundamental<Exception>::value>::type
    BOOST_CC_REG throw_exception( Exception const exception ) { throw exception; }

    template <typename Error>
    BOOST_ATTRIBUTES( BOOST_DOES_NOT_RETURN, BOOST_COLD )
    void BOOST_CC_REG make_and_throw_exception( Error && error ) { throw_exception( make_exception( std::forward<Error>( error ) ) ); }

    template <typename Error>
    BOOST_ATTRIBUTES( BOOST_COLD )
    std::exception_ptr BOOST_CC_REG make_exception_ptr( Error && error ) { return std::make_exception_ptr( make_exception( std::forward<Error>( error ) ) ); }
} // namespace detail

BOOST_OPTIMIZE_FOR_SIZE_END()

template <class Result, class Error>
using compressed_result_error_variant =
    std::integral_constant
    <
        bool,
        std::is_empty                <Error       >::value &&
        std::is_convertible          <Result, bool>::value &&
        std::is_default_constructible<Result      >::value
    >; //...mrmlj...todo/track std::is_explicitly_convertible


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

template <class Result, class Error, typename = void>
class result_or_error
{
public:
	/// \note Be liberal with the constructor argument type in order to allow
	/// emplacement/eliminate an intermediate move constructor call (as it can
	/// actually be nontrivial, e.g. with MSVC14 RC std::string).
	///                                       (18.05.2015.) (Domagoj Saric)
	template <typename Source> result_or_error( Source && BOOST_RESTRICTED_REF result, typename std::enable_if<std::is_constructible<Result, Source &&>::value>::type const * = nullptr ) noexcept( std::is_nothrow_constructible<Result, Source &&>::value ) : succeeded_( true  ), inspected_( false ), result_( std::forward<Source>( result ) ) {}
	template <typename Source> result_or_error( Source && BOOST_RESTRICTED_REF error , typename std::enable_if<std::is_constructible<Error , Source &&>::value>::type const * = nullptr ) noexcept( std::is_nothrow_constructible<Error , Source &&>::value ) : succeeded_( false ), inspected_( false ), error_ ( std::forward<Source>( error  ) ) {}

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

    result_or_error( result_or_error const & ) = delete;

    BOOST_OPTIMIZE_FOR_SIZE_BEGIN()
    BOOST_ATTRIBUTES( BOOST_COLD )
	~result_or_error() noexcept( std::is_nothrow_destructible<Result>::value && std::is_nothrow_destructible<Error>::value )
	{
		//BOOST_ASSERT_MSG( inspected(), "Ignored error return code." );
		if ( BOOST_LIKELY( succeeded_ ) )
            result_.~Result();
        else
            error_.~Error();
	};
    BOOST_OPTIMIZE_FOR_SIZE_END()

	Error  const & error ()       { BOOST_ASSERT_MSG( !succeeded_, "Querying the error of a succeeded operation." ); return error_ ; }
	Result       & result()       { BOOST_ASSERT_MSG(  succeeded_, "Querying the result of a failed operation."   ); return result_; }
	Result const & result() const { return const_cast<Result &>( *this ).result(); }

					  bool inspected() const noexcept {                    return BOOST_LIKELY( inspected_ ); }
		              bool succeeded() const noexcept { inspected_ = true; return BOOST_LIKELY( succeeded_ ); }
	explicit operator bool          () const noexcept {                    return succeeded()               ; }

    BOOST_OPTIMIZE_FOR_SIZE_BEGIN()
#ifndef BOOST_NO_EXCEPTIONS
	void throw_if_error() noexcept( false )
	{
		BOOST_ASSERT( !std::uncaught_exception() );
		if ( succeeded() )
		{
			BOOST_ASSUME( inspected_ );
			BOOST_ASSUME( succeeded_ );
			return;
		}
		BOOST_ASSUME( inspected_ );
		throw_error();
	}

	void throw_if_uninspected_error() noexcept( false )
	{
		if ( !inspected() )
		{
			throw_if_error();
			BOOST_ASSUME( succeeded_ );
		}
		BOOST_ASSUME( inspected_ );
	}

	BOOST_ATTRIBUTES( BOOST_DOES_NOT_RETURN, BOOST_COLD )
	void throw_error() noexcept( false )
	{
		BOOST_ASSERT( !succeeded() );
		BOOST_ASSERT( !std::uncaught_exception() );
		detail::make_and_throw_exception( std::move( error_ ) );
	}
#endif // BOOST_NO_EXCEPTIONS

    BOOST_ATTRIBUTES( BOOST_COLD )
    std::exception_ptr make_exception_ptr()
	{
        // http://en.cppreference.com/w/cpp/error/exception_ptr
		BOOST_ASSERT( !succeeded() );
		BOOST_ASSERT( !std::uncaught_exception() );
		return detail::make_exception_ptr( std::move( error_ ) );
	}
    BOOST_OPTIMIZE_FOR_SIZE_END()

protected:
	        bool succeeded_;
	mutable bool inspected_;

private: friend class fallible_result<Result, Error>;
#ifdef BOOST_MSVC
    #pragma warning( push )
    #pragma warning( disable : 4510 ) // Default constructor was implicitly defined as deleted.
    #pragma warning( disable : 4624 ) // Destructor was implicitly defined as deleted because a base class destructor is inaccessible or deleted.
#endif // BOOST_MSVC
	union
	{
		Result result_;
		Error  error_ ;
	};
#ifdef BOOST_MSVC
    #pragma warning( pop )
#endif // BOOST_MSVC
}; // class result_or_error


////////////////////////////////////////////////////////////////////////////////
///
/// 'compressed' result_or_error specialisation (for default-constructible
/// Results and empty Errors).
///
////////////////////////////////////////////////////////////////////////////////

template <class Result, class Error>
class result_or_error<Result, Error, typename std::enable_if<compressed_result_error_variant<Result, Error>::value>::type>
{
public:
	template <typename Source>
    result_or_error( Source && BOOST_RESTRICTED_REF result ) noexcept( std::is_nothrow_constructible<Result, Source &&>::value ) : inspected_( false ), result_( std::forward<Source>( result ) ) {}

	result_or_error( result_or_error && BOOST_RESTRICTED_REF other ) noexcept( std::is_nothrow_move_constructible<Result>::value )
		:
        result_   ( std::move( other.result_ ) ),
        inspected_( false                      )
	{
		other.inspected_ = true;
	}

    result_or_error( result_or_error const & ) = delete;

#if 0 // disabled
	~result_or_error() noexcept { BOOST_ASSERT_MSG( inspected(), "Ignored error return code." ); };
#endif

	Error          error ()       noexcept { BOOST_ASSERT_MSG( inspected() && !*this, "Querying the error of a (possibly) succeeded operation." ); return Error(); }
	Result       & result()       noexcept { BOOST_ASSERT_MSG( inspected() &&  *this, "Querying the result of a (possibly) failed operation."   ); return result_; }
	Result const & result() const noexcept { return const_cast<Result &>( *this ).result(); }

					  bool inspected() const noexcept {                    return BOOST_LIKELY( inspected_ ); }
                      bool succeeded() const noexcept { inspected_ = true; return BOOST_LIKELY( !!result_  ); }
	explicit operator bool          () const noexcept {                    return succeeded()               ; }

    BOOST_OPTIMIZE_FOR_SIZE_BEGIN()
#ifndef BOOST_NO_EXCEPTIONS //...mrmlj...kill this duplication...
    BOOST_ATTRIBUTES( BOOST_COLD )
	void throw_if_error() noexcept( false )
	{
		BOOST_ASSERT( !std::uncaught_exception() );
		if ( BOOST_LIKELY( succeeded() ) )
		{
			BOOST_ASSUME( inspected_ );
			return;
		}
		BOOST_ASSUME( inspected_ );
		throw_error();
	}

	void throw_if_uninspected_error() noexcept( false )
	{
		if ( !inspected() )
		{
			throw_if_error();
		}
		BOOST_ASSUME( inspected_ );
	}

	BOOST_ATTRIBUTES( BOOST_DOES_NOT_RETURN, BOOST_COLD )
	void throw_error() noexcept( false )
	{
		BOOST_ASSERT( !succeeded() );
		BOOST_ASSERT( !std::uncaught_exception() );
		detail::make_and_throw_exception( error() );
	}
#endif // BOOST_NO_EXCEPTIONS

    BOOST_ATTRIBUTES( BOOST_COLD )
    std::exception_ptr make_exception_ptr()
	{
		BOOST_ASSERT( !succeeded() );
		BOOST_ASSERT( !std::uncaught_exception() );
		return detail::make_exception_ptr( error() );
	}
    BOOST_OPTIMIZE_FOR_SIZE_END()

private: friend class fallible_result<Result, Error>;
	Result result_;

protected:
    mutable bool inspected_;
}; // class result_or_error 'compressed' specialisation


////////////////////////////////////////////////////////////////////////////////
///
/// result_or_error<void, Error> specialisation for void-return functions.
///
////////////////////////////////////////////////////////////////////////////////

template <class Error>
using void_or_error = result_or_error<void, Error, void>;

template <class Error>
class result_or_error<void, Error, void>
{
public:
	template <typename Source>
    result_or_error( Source && BOOST_RESTRICTED_REF error, typename std::enable_if<!std::is_same<Source, fallible_result<void, Error>>::value>::type const * = nullptr )
        noexcept( std::is_nothrow_constructible<Error, Source &&>::value )
        : succeeded_( false ), inspected_( false ), error_( std::forward<Source>( error ) ) {}

    result_or_error( no_err_t ) noexcept : succeeded_( true ), inspected_( false ) {}

	result_or_error( result_or_error && BOOST_RESTRICTED_REF other ) noexcept( std::is_nothrow_move_constructible<Error>::value )
        : succeeded_( other.succeeded() ), inspected_( false )
	{
		if ( BOOST_UNLIKELY( !succeeded_ ) )
		{
			auto const ptr( new ( &error_ ) Error ( std::move( other.error_ ) ) );
			BOOST_ASSUME( ptr );
			BOOST_ASSUME( !other.succeeded_ );
		}
	}

    result_or_error( result_or_error const & ) = delete;

    BOOST_ATTRIBUTES( BOOST_COLD )
	~result_or_error() noexcept( std::is_nothrow_destructible<Error>::value )
	{
		if ( BOOST_UNLIKELY( !succeeded_ ) )
            error_.~Error();
	};

	Error const & error() noexcept { BOOST_ASSERT_MSG( inspected() && !*this, "Querying the error of a (possibly) succeeded operation." ); return error_; }

					  bool inspected() const noexcept {                    return BOOST_LIKELY( inspected_ ); }
                      bool succeeded() const noexcept { inspected_ = true; return BOOST_LIKELY( succeeded_ ); }
	explicit operator bool          () const noexcept {                    return succeeded()               ; }

    BOOST_OPTIMIZE_FOR_SIZE_BEGIN()
#ifndef BOOST_NO_EXCEPTIONS //...mrmlj...kill this duplication...
    BOOST_ATTRIBUTES( BOOST_COLD )
	void throw_if_error() noexcept( false )
	{
		BOOST_ASSERT( !std::uncaught_exception() );
		if ( BOOST_LIKELY( succeeded() ) )
		{
			BOOST_ASSUME( inspected_ );
			return;
		}
		BOOST_ASSUME( inspected_ );
		throw_error();
	}

	void throw_if_uninspected_error() noexcept( false )
	{
		if ( !inspected() )
		{
			throw_if_error();
		}
		BOOST_ASSUME( inspected_ );
	}

	BOOST_ATTRIBUTES( BOOST_DOES_NOT_RETURN, BOOST_COLD )
	void throw_error() noexcept( false )
	{
		BOOST_ASSERT( !succeeded() );
		BOOST_ASSERT( !std::uncaught_exception() );
		detail::make_and_throw_exception( error() );
	}
#endif // BOOST_NO_EXCEPTIONS

    BOOST_ATTRIBUTES( BOOST_COLD )
    std::exception_ptr make_exception_ptr()
	{
		BOOST_ASSERT( !succeeded() );
		BOOST_ASSERT( !std::uncaught_exception() );
		return detail::make_exception_ptr( error() );
	}
    BOOST_OPTIMIZE_FOR_SIZE_END()

private: friend class fallible_result<void, Error>;
#ifdef BOOST_MSVC
    #pragma warning( push )
    #pragma warning( disable : 4510 ) // Default constructor was implicitly defined as deleted.
    #pragma warning( disable : 4624 ) // Destructor was implicitly defined as deleted because a base class destructor is inaccessible or deleted.
#endif // BOOST_MSVC
    union { Error error_; };
#ifdef BOOST_MSVC
    #pragma warning( pop )
#endif // BOOST_MSVC

protected:
            bool const succeeded_;
    mutable bool       inspected_;
}; // class result_or_error 'void result' specialisation


template <typename Result, typename Error, typename Dummy> bool operator==( result_or_error<Result, Error, Dummy> const & result, no_err_t ) noexcept { return  result.succeeded(); }
template <typename Result, typename Error, typename Dummy> bool operator==( result_or_error<Result, Error, Dummy> const & result, an_err_t ) noexcept { return !result.succeeded(); }
template <typename Result, typename Error, typename Dummy> bool operator!=( result_or_error<Result, Error, Dummy> const & result, no_err_t ) noexcept { return !result.succeeded(); }
template <typename Result, typename Error, typename Dummy> bool operator!=( result_or_error<Result, Error, Dummy> const & result, an_err_t ) noexcept { return  result.succeeded(); }

//------------------------------------------------------------------------------
} // namespace err
//------------------------------------------------------------------------------
} // namespace boost
//------------------------------------------------------------------------------
#endif // result_or_error_hpp

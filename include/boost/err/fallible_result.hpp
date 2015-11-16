////////////////////////////////////////////////////////////////////////////////
///
/// \file fallible_result.hpp
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
#ifndef fallible_result_hpp__568DC1C1_A422_4EE6_9993_8FC98EE846C8
#define fallible_result_hpp__568DC1C1_A422_4EE6_9993_8FC98EE846C8
#pragma once
//------------------------------------------------------------------------------
#include "result_or_error.hpp"

#include <boost/assert.hpp>
#include <boost/config.hpp>

#include <cstdint>
#include <new>
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
/// \detail
///  these objects are not meant to be saved (on the stack or "as members)"
///
////////////////////////////////////////////////////////////////////////////////

#ifndef NDEBUG
namespace detail
{
    // https://devforums.apple.com/message/1101679#1101679
    // http://comments.gmane.org/gmane.editors.textmate.general/38535
    // https://github.com/textmate/textmate/commit/172ce9d4282e408fe60b699c432390b9f6e3f74a
    BOOST_THREAD_LOCAL_POD std::uint8_t fallible_result_counter( 0 ) /*BOOST_OVERRIDABLE_SYMBOL*/; // GCC 4.9 from Android NDK 10e ICEs if BOOST_OVERRIDABLE_SYMBOL is put 'in the front' and Clang does not like at the end...
} // namespace detail
#endif // NDEBUG

template <class Result, class Error>
class fallible_result
{
public:
    // todo: variadic arguments
	template <typename T>
	fallible_result( T && BOOST_RESTRICTED_REF argument ) noexcept( std::is_nothrow_constructible<result_or_error<Result, Error>, T &&>::value )
        : result_or_error_( std::forward<T>( argument ) )
    {
        BOOST_ASSERT_MSG
        (
            detail::fallible_result_counter++ == 0,
            "More than one fallible_result instance detected"
        );
        BOOST_ASSUME( !result_or_error_.inspected_ );
    }

	fallible_result( fallible_result && BOOST_RESTRICTED_REF other ) noexcept( std::is_nothrow_move_constructible<result_or_error<Result, Error>>::value )
        : result_or_error_( std::move( std::move( other ).operator result_or_error<Result, Error> &&() ) )
    {
        BOOST_ASSERT_MSG
        (
            detail::fallible_result_counter++ == 0,
            "More than one fallible_result instance detected"
        );
        BOOST_ASSUME( !result_or_error_.inspected_ );
    }

    fallible_result( fallible_result const & ) = delete;

	~fallible_result() noexcept( false )
	{
        BOOST_ASSERT_MSG
        (
            detail::fallible_result_counter-- == 1,
            "More than one fallible_result instance detected"
        );
		result_or_error_.throw_if_uninspected_error();
		BOOST_ASSUME( result_or_error_.inspected_ );
	};

    result_or_error<Result, Error> && as_result_or_error() && noexcept( true ) { std::move( *this ).ignore_failure(); return std::move( result_or_error_ ); }

	operator result_or_error<Result, Error> && () && noexcept( true  ) { return std::move( *this ).as_result_or_error(); }
	operator Result                         && () && noexcept( false ) { return std::move( result() ); }

	                                                     Result && operator *  () && { return  result(); }
	BOOST_ATTRIBUTES( BOOST_RESTRICTED_FUNCTION_RETURN ) Result *  operator -> () && { return &result(); }

    explicit operator bool() && noexcept { return static_cast<bool>( result_or_error_ ); }

    void ignore_failure() && noexcept { result_or_error_.inspected_ = true; }

private:
	Result && result()
    {
        result_or_error_.throw_if_error();
        BOOST_ASSUME( result_or_error_.inspected_ );
      //BOOST_ASSUME( result_or_error_.succeeded_ ); // the 'compressed' specialisation does not have the 'succeeded_' member
        return static_cast<Result &&>( result_or_error_.result_ );
    }

    void * operator new     (         std::size_t                         ) = delete;
    void * operator new[]   (         std::size_t                         ) = delete;
    void * operator new     (         std::size_t, std::nothrow_t const & ) = delete;
    void * operator new[]   (         std::size_t, std::nothrow_t const & ) = delete;
    void   operator delete  ( void *                                      ) = delete;
    void   operator delete[]( void *                                      ) = delete;
    void   operator delete  ( void *, std::size_t                         ) = delete;
    void   operator delete[]( void *, std::size_t                         ) = delete;

private: friend class result_or_error<Result, Error>;
    /// \note (Private) inheritance cannot be used as that would break the
    /// result_or_error implicit conversion operator.
    ///                                       (22.05.2015.) (Domagoj Saric)
	result_or_error<Result, Error> result_or_error_;
}; // class fallible_result


////////////////////////////////////////////////////////////////////////////////
///
/// fallible_result<void, Error> specialisation for void-return functions.
///
////////////////////////////////////////////////////////////////////////////////

template <class Error> using possible      = fallible_result<void, Error>;
template <class Error> using fallible_with = fallible_result<void, Error>;

template <class Error>
class fallible_result<void, Error> //...mrmlj...kill the duplication...
{
public:
    using result = result_or_error<void, Error>;

public:
	template <typename T>
	fallible_result( T && BOOST_RESTRICTED_REF argument ) noexcept( std::is_nothrow_constructible<result, T &&>::value )
        : void_or_error_( std::forward<T>( argument ) )
    {
        BOOST_ASSERT_MSG
        (
            detail::fallible_result_counter++ == 0,
            "More than one fallible_result instance detected"
        );
        BOOST_ASSUME( !void_or_error_.inspected_ );
    }

	fallible_result( fallible_result && BOOST_RESTRICTED_REF other ) noexcept( std::is_nothrow_move_constructible<result>::value )
        : void_or_error_( std::move( std::move( other ).operator result &&() ) )
    {
        BOOST_ASSERT_MSG
        (
            detail::fallible_result_counter++ == 0,
            "More than one fallible_result instance detected"
        );
        BOOST_ASSUME( !void_or_error_.inspected_ );
    }

    BOOST_OPTIMIZE_FOR_SIZE_BEGIN()
	~fallible_result() noexcept( false )
	{
        BOOST_ASSERT_MSG
        (
            detail::fallible_result_counter-- == 1,
            "More than one fallible_result instance detected"
        );
		void_or_error_.throw_if_uninspected_error();
		BOOST_ASSUME( void_or_error_.inspected_ );
	};
    BOOST_OPTIMIZE_FOR_SIZE_END()

	operator result && () && noexcept { static_cast<fallible_result &&>( *this ).ignore_failure(); return std::move( void_or_error_ ); }

    bool succeeded() && noexcept { return void_or_error_.succeeded(); }

    explicit operator bool() && noexcept { return std::move( *this ).succeeded(); }

    void ignore_failure() && noexcept { static_cast<fallible_result &&>( *this ).succeeded(); }

private:
    void * operator new     (         std::size_t                         ) = delete;
    void * operator new[]   (         std::size_t                         ) = delete;
    void * operator new     (         std::size_t, std::nothrow_t const & ) = delete;
    void * operator new[]   (         std::size_t, std::nothrow_t const & ) = delete;
    void   operator delete  ( void *                                      ) = delete;
    void   operator delete[]( void *                                      ) = delete;
    void   operator delete  ( void *, std::size_t                         ) = delete;
    void   operator delete[]( void *, std::size_t                         ) = delete;

private: friend result;
	result void_or_error_;
}; // class fallible_result<void, Error>


template <typename Result, typename Error> bool operator==( fallible_result<Result, Error> && result, no_err_t ) noexcept { return  std::move( result ).succeeded(); }
template <typename Result, typename Error> bool operator==( fallible_result<Result, Error> && result, an_err_t ) noexcept { return !std::move( result ).succeeded(); }
template <typename Result, typename Error> bool operator!=( fallible_result<Result, Error> && result, no_err_t ) noexcept { return !std::move( result ).succeeded(); }
template <typename Result, typename Error> bool operator!=( fallible_result<Result, Error> && result, an_err_t ) noexcept { return  std::move( result ).succeeded(); }

//------------------------------------------------------------------------------
} // namespace err
//------------------------------------------------------------------------------
} // namespace boost
//------------------------------------------------------------------------------
#endif // fallible_result_hpp

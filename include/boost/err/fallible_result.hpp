////////////////////////////////////////////////////////////////////////////////
///
/// \file fallible_result.hpp
/// -------------------------
///
/// Copyright (c) Domagoj Saric 2015 - 2019.
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

#ifndef NDEBUG
#include "detail/thread_singleton.hpp"

#include <exception>
#endif // !NDEBUG

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
/// These objects are designed to be used solely as rvalues, i.e. they are not
/// meant to be saved (on the stack or as members): either they are to be left
/// unreferenced or implicitly converted to T [when they self(convert&)throw in
/// case of failure] or to be explicitly saved for later inspection (converted
/// to a result_or_error lvalue).<BR>
/// Correct usage is enforced at compile-time where possible and with runtime
/// assertions in the very few areas where the language does not yet provide the
/// necessary constructs (e.g. rvalue destructors) for compile-time checks.<BR>
/// One such area is especially tricky: the handling of multiple 'live'
/// (coexisting) fallible_results per-thread. Normally this would indicate a
/// programmer error (i.e. that the programmer accidentally saved a
/// fallible_result, e.g. in an auto variable, and forgot about it so
/// compile-time checks didn't kick in) except in one case: expressions with
/// multiple function calls that return/create fallible_results (e.g. calls to
/// functions with multiple parameters). Because the standard (intentionally)
/// leaves the order of construction of function arguments unspecified (even to
/// the point that the implicit conversion of a return fallible_result<T, err_t>
/// to T or result_or_error<T, err_t> is not guaranteed to happen before the
/// construction of the next fallible_result<U, err_t> begins in the current
/// expression) we have to allow the possibility of multiple coexisting
/// fallible_results[1]. However, two bad things happen if we allow this:
/// * correct usage cannot be checked as 'neatly' - we can only check that at
///   least one fallible_result instance has been inspected on a given thread
///   before leaving the current scope (or an exception has been thrown by '3rd
///   party code')
/// * multiple live fallible_results -> multiple possibly throwing destructors
///   -> possible std::terminate call - we can workaround it by making the
///   destructors throw only if !std::uncaught_exception(). [2]
/// <BR>
/// [1] One exception, for obvious reasons, are void functions (returning
/// fallible_result<void, err_t>). For this reason the fallible_result<void>
/// partial specialisation does not allow multiple coexisting instances.
/// [2] This does mean that construction of other parameters can proceed even
/// if a previous one has already failed but this is actually not that different
/// from what you have to expect anyway precisely because of the undefined order
/// in which parameters are constructed. If this behaviour is undesired/strictly
/// prohibited it can be solved two ways:
/// * on the 'library' side - Boost.Err/fallible_result can provide a
///   'failed_result_pending()'-like function (the equivalent of
///   std::uncaught_exception()) which a function returning a fallible_result
///   can check and abort/return early if necessary
/// * on the 'client' side - the user can simply first save the fallible_results
///   as named (T or result_or_error<T>) objects/lvalues and then call the
///   function/evaulate the expression with the multiple objects/arguments.
/// (see more @
/// http://boost.2283326.n4.nabble.com/err-RFC-td4681600.html#a4681672)
///
////////////////////////////////////////////////////////////////////////////////

#ifndef NDEBUG
namespace detail
{
    struct fallible_result_sanitizer
    {
        std::uint8_t live_instance_counter  = 0;
        bool         at_least_one_inspected = false;

        std::uint8_t live_void_instance_counter = 0;

        static fallible_result_sanitizer & singleton() { return thread_singleton<fallible_result_sanitizer>::instance(); }

        static void    add_instance(                      ) { singleton().   add_instance_aux(           ); }
        static void remove_instance( bool const inspected ) { singleton().remove_instance_aux( inspected ); }

    private:
        static auto uncaught_exceptions() noexcept
        {
        #if __cpp_lib_uncaught_exceptions
            return std::uncaught_exceptions();
        #else
            return std::uncaught_exception();
        #endif // __cpp_lib_uncaught_exceptions
        }

        void add_instance_aux() { ++live_instance_counter; }

        void remove_instance_aux( bool const inspected )
        {
            BOOST_ASSERT_MSG( live_instance_counter > 0, "Mismatched add/remove instance." );
            at_least_one_inspected |= inspected;
            --live_instance_counter;
            BOOST_ASSERT_MSG
            (
                at_least_one_inspected ||
                live_instance_counter  ||  // there are still live fallible_results (allow that one of those will be inspected even if none have been so far)
                uncaught_exceptions(), // a '3rd party' exception caused early exit
                "Uninspected fallible_result<T>."
            );
            at_least_one_inspected &= ( live_instance_counter != 0 );
        }
    }; // struct fallible_result_sanitizer
} // namespace detail
#endif // NDEBUG

template <class Result, class Error>
class fallible_result
{
public:
    template <typename ... T>
    fallible_result( T && BOOST_RESTRICTED_REF ... argument ) noexcept( std::is_nothrow_constructible<result_or_error<Result, Error>, T &&...>::value )
        : result_or_error_( std::forward<T>( argument )... )
    {
    #ifndef NDEBUG
        detail::fallible_result_sanitizer::add_instance();
    #endif // NDEBUG
        BOOST_ASSUME( !result_or_error_.inspected_ );
    }

    fallible_result( fallible_result && BOOST_RESTRICTED_REF other ) noexcept( std::is_nothrow_move_constructible<result_or_error<Result, Error>>::value )
        : result_or_error_( std::move( std::move( other ).as_result_or_error() ) )
    {
    #ifndef NDEBUG
        detail::fallible_result_sanitizer::add_instance();
    #endif // NDEBUG
        BOOST_ASSUME( other.result_or_error_.inspected_ == true  );
        BOOST_ASSUME( this->result_or_error_.inspected_ == false );
    }

    fallible_result( fallible_result const & ) = delete;

    BOOST_ATTRIBUTES( BOOST_MINSIZE )
    ~fallible_result() noexcept( false )
    {
    #ifndef NDEBUG
        detail::fallible_result_sanitizer::remove_instance( result_or_error_.inspected_ );
    #endif // NDEBUG
        result_or_error_.throw_if_uninspected_error();
        BOOST_ASSUME( result_or_error_.inspected_ );
    }

    result_or_error<Result, Error> && as_result_or_error() && noexcept { BOOST_ASSUME( !result_or_error_.inspected_ ); std::move( *this ).ignore_failure(); return std::move( result_or_error_ ); }
    result_or_error<Result, Error> && operator()        () && noexcept { return std::move( *this ).as_result_or_error(); }

    operator result_or_error<Result, Error> && () && noexcept { return std::move( *this ).as_result_or_error(); }
    operator Result                         && () &&          { return std::move( result() ); }

                                                         Result && operator *  () && { return  result(); }
    BOOST_ATTRIBUTES( BOOST_RESTRICTED_FUNCTION_RETURN ) Result *  operator -> () && { return &result(); }

    explicit operator bool() && noexcept { return static_cast<bool>( result_or_error_ ); }

    void ignore_failure() BOOST_RESTRICTED_THIS && noexcept { result_or_error_.inspected_ = true; }

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
    template <typename ... T>
    fallible_result( T && BOOST_RESTRICTED_REF ... argument ) noexcept( std::is_nothrow_constructible<result, T && ...>::value )
        : void_or_error_( std::forward<T>( argument )... )
    {
        BOOST_ASSERT_MSG
        (
            detail::fallible_result_sanitizer::singleton().live_void_instance_counter++ == 0,
            "More than one fallible_result<void> instance detected"
        );
        BOOST_ASSUME( !void_or_error_.inspected_ );
    }

    fallible_result( fallible_result && BOOST_RESTRICTED_REF other ) noexcept( std::is_nothrow_move_constructible<result>::value )
        : void_or_error_( std::move( std::move( other ).operator result &&() ) )
    {
        BOOST_ASSERT_MSG
        (
            detail::fallible_result_sanitizer::singleton().live_void_instance_counter++ == 0,
            "More than one fallible_result<void> instance detected"
        );
        BOOST_ASSUME( !void_or_error_.inspected_ );
    }

    BOOST_OPTIMIZE_FOR_SIZE_BEGIN()
    ~fallible_result() noexcept( false )
    {
        BOOST_ASSERT_MSG
        (
            detail::fallible_result_sanitizer::singleton().live_void_instance_counter-- == 1,
            "More than one fallible_result<void> instance detected"
        );
        void_or_error_.throw_if_uninspected_error();
        BOOST_ASSUME( void_or_error_.inspected_ );
    }
    BOOST_OPTIMIZE_FOR_SIZE_END()

    result && operator()() && noexcept { return std::move( *this ); }

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


template <typename T>
using infallible_result = T;

//------------------------------------------------------------------------------
} // namespace err
//------------------------------------------------------------------------------
} // namespace boost
//------------------------------------------------------------------------------
#endif // fallible_result_hpp

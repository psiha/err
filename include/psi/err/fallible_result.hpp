////////////////////////////////////////////////////////////////////////////////
///
/// \file fallible_result.hpp
/// -------------------------
///
/// Copyright (c) Domagoj Saric 2015 - 2024.
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

#include "result_or_error.hpp"

#include <boost/assert.hpp>
#include <boost/config.hpp>

#ifndef NDEBUG
#include <exception>
#ifdef __APPLE__
#include <Availability.h>
#include <TargetConditionals.h>
#endif
#endif // !NDEBUG

#include <cstdint>
#include <new>
#include <type_traits>
#include <utility>
//------------------------------------------------------------------------------
namespace psi::err
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
/// from what has to be expected anyway precisely because of the undefined order
/// in which parameters are constructed. If this behaviour is undesired/strictly
/// prohibited it can be solved two ways:
/// * on the 'library' side - Psi.Err/fallible_result can provide a
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

        static BOOST_THREAD_LOCAL_POD fallible_result_sanitizer singleton;

        static void    add_instance(                      ) { singleton.   add_instance_aux(           ); }
        static void remove_instance( bool const inspected ) { singleton.remove_instance_aux( inspected ); }

    private:
        void add_instance_aux() { ++live_instance_counter; }

        void remove_instance_aux( bool const inspected )
        {
            BOOST_ASSERT_MSG( live_instance_counter > 0, "Mismatched add/remove instance." );
            at_least_one_inspected |= inspected;
            --live_instance_counter;
            BOOST_ASSERT_MSG
            (
                at_least_one_inspected ||
                live_instance_counter  ||   // there are still live fallible_results (allow that one of those will be inspected even if none have been so far)
                std::uncaught_exceptions(), // a '3rd party' exception caused early exit
                "Uninspected fallible_result<T>."
            );
            at_least_one_inspected &= ( live_instance_counter != 0 );
        }
    }; // struct fallible_result_sanitizer

    inline BOOST_THREAD_LOCAL_POD fallible_result_sanitizer fallible_result_sanitizer::singleton;
} // namespace detail
#endif // NDEBUG

template <class Result, class Error>
class [[ clang::trivial_abi ]] fallible_result
{
public:
    template <typename ... T>
    fallible_result( T && __restrict ... argument ) noexcept( std::is_nothrow_constructible<result_or_error<Result, Error>, T &&...>::value )
        : result_or_error_( std::forward<T>( argument )... )
    {
    #ifndef NDEBUG
        detail::fallible_result_sanitizer::add_instance();
    #endif // NDEBUG
        BOOST_ASSUME( !result_or_error_.inspected_ );
    }

    fallible_result( fallible_result const & ) = delete;

    BOOST_ATTRIBUTES( BOOST_MINSIZE ) PSI_RELEASE_FORCEINLINE
    ~fallible_result() noexcept( false )
    {
    #ifndef NDEBUG
        detail::fallible_result_sanitizer::remove_instance( result_or_error_.inspected_ );
    #endif // NDEBUG
        result_or_error_.throw_if_uninspected_error();
        BOOST_ASSUME( result_or_error_.inspected_ );
    }

    // Due to rules regarding NRVO, over which a programmer has no control,
    // move and copy constructors have to be made private to avoid the case of
    // someone saving a fallible_result to a local named variable, inspecting it
    // (using std::move) and then unconditionally returning it to the
    // caller/further up the chain: the compiler is free to return the inspected
    // value directly - which can cause incorrect behaviour in the caller, e.g.
    // an error is not thrown because the returned fallible_result instance has
    // the inspected_ flag set to true. The current workaround, if you require
    // that sort of usage, is to return calling propagate (which inhibts NRVO)
    // (e.g. return my_result.propagate();).
    // https://en.cppreference.com/w/cpp/language/copy_elision
    fallible_result propagate() noexcept( std::is_nothrow_move_constructible_v<Result> ) { return std::move( *this ); }

    result_or_error<Result, Error> as_result_or_error() && noexcept { BOOST_ASSUME( !result_or_error_.inspected_ ); std::move( *this ).ignore_failure(); return std::move( result_or_error_ ); }
    result_or_error<Result, Error> operator()        () && noexcept { return std::move( *this ).as_result_or_error(); }

    operator result_or_error<Result, Error> &&() && noexcept { return std::move( *this ).as_result_or_error(); }
    operator Result                         &&() &&          { return std::move( result() ); }

    Result && operator *  () && { return  result(); }
    Result *  operator -> () && { return &result(); }

    explicit operator bool() && noexcept { return static_cast<bool>( result_or_error_ ); }

    void ignore_failure() BOOST_RESTRICTED_THIS && noexcept { result_or_error_.inspected_ = true; }

private:
    fallible_result( fallible_result && __restrict other ) noexcept( std::is_nothrow_move_constructible<result_or_error<Result, Error>>::value )
        : result_or_error_( std::move( std::move( other ).as_result_or_error() ) )
    {
#   ifndef NDEBUG
        detail::fallible_result_sanitizer::add_instance();
#   endif // NDEBUG
        BOOST_ASSUME( other.result_or_error_.inspected_ == true  );
        BOOST_ASSUME( this->result_or_error_.inspected_ == false );
    }

    Result && result()
    {
        result_or_error_.throw_if_error();
        BOOST_ASSUME( result_or_error_.inspected_ );
      //BOOST_ASSUME( result_or_error_.succeeded_ ); // the 'compressed' specialisation does not have the 'succeeded_' member
        return static_cast<Result &&>( result_or_error_.result_ );
    }

private: // prevent bogus heap creation
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
class [[ clang::trivial_abi ]] fallible_result<void, Error> //...mrmlj...kill the duplication...
{
public:
    using result = result_or_error<void, Error>;

public:
    template <typename ... T>
    fallible_result( T && __restrict ... argument ) noexcept( std::is_nothrow_constructible<result, T && ...>::value )
        : void_or_error_( std::forward<T>( argument )... )
    {
        BOOST_ASSERT_MSG
        (
            detail::fallible_result_sanitizer::singleton.live_void_instance_counter++ == 0,
            "More than one fallible_result<void> instance detected"
        );
        BOOST_ASSUME( !void_or_error_.inspected_ );
    }

    BOOST_OPTIMIZE_FOR_SIZE_BEGIN() PSI_RELEASE_FORCEINLINE
    ~fallible_result() noexcept( false )
    {
        BOOST_ASSERT_MSG
        (
            detail::fallible_result_sanitizer::singleton.live_void_instance_counter-- == 1 + moved_from_,
            "More than one fallible_result<void> instance detected"
        );
        void_or_error_.throw_if_uninspected_error();
        BOOST_ASSUME( void_or_error_.inspected_ );
    }
    BOOST_OPTIMIZE_FOR_SIZE_END()

    // see the note in the main template
    fallible_result propagate() noexcept( std::is_nothrow_move_constructible_v<result> ) { return std::move( *this ); }

    result operator()() && noexcept { return std::move( *this ); }
    operator result  () && noexcept { return std::move( void_or_error_ ); }

    bool succeeded() && noexcept { return void_or_error_.succeeded(); }

    explicit operator bool() && noexcept { return std::move( *this ).succeeded(); }

    void ignore_failure() && noexcept { std::move( *this ).succeeded(); }

private: // see not for propagate()
    fallible_result( fallible_result && __restrict other ) noexcept( std::is_nothrow_move_constructible_v<result> )
        : void_or_error_( std::move( other ).operator result &&() )
    {
#   ifndef NDEBUG
        BOOST_ASSERT( !other.moved_from_ );
        other.moved_from_ = true;
        BOOST_ASSERT_MSG
        (
            detail::fallible_result_sanitizer::singleton.live_void_instance_counter++ == /*other destructor yet has to be called*/1,
            "More than one fallible_result<void> instance detected"
        );
#   endif
        BOOST_ASSUME( !void_or_error_.inspected_ );
    }

private: // prevent bogus heap creation
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
#ifndef NDEBUG
    bool moved_from_{ false };
#endif
}; // class fallible_result<void, Error>


template <typename Result, typename Error> bool operator==( fallible_result<Result, Error> && result, no_err_t ) noexcept { return  std::move( result ).succeeded(); }
template <typename Result, typename Error> bool operator==( fallible_result<Result, Error> && result, an_err_t ) noexcept { return !std::move( result ).succeeded(); }
template <typename Result, typename Error> bool operator!=( fallible_result<Result, Error> && result, no_err_t ) noexcept { return !std::move( result ).succeeded(); }
template <typename Result, typename Error> bool operator!=( fallible_result<Result, Error> && result, an_err_t ) noexcept { return  std::move( result ).succeeded(); }


template <typename T>
using infallible_result = T;

//------------------------------------------------------------------------------
} // namespace psi::err
//------------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
///
/// \file result_or_error.hpp
/// -------------------------
///
/// Copyright (c) Domagoj Saric 2015 - 2017.
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
#include "exceptions.hpp"

#include <boost/assert.hpp>
#include <boost/config_ex.hpp>

#include <type_traits>
#include <utility>
//------------------------------------------------------------------------------
namespace boost
{
//------------------------------------------------------------------------------
namespace err
{
//------------------------------------------------------------------------------

struct no_err_t {};
static no_err_t constexpr no_err    = {};
static no_err_t constexpr success   = {};
static no_err_t constexpr succeeded = {};

struct an_err_t {};
static an_err_t constexpr an_err  = {};
static an_err_t constexpr failure = {};
static an_err_t constexpr failed  = {};


template <class Result, class Error>
using compressed_result_error_variant =
    std::integral_constant
    <
        bool,
        std::is_empty                <Error       >::value &&
        std::is_convertible          <Result, bool>::value &&
        std::is_default_constructible<Result      >::value &&
       !std::is_fundamental          <Result      >::value    // 'fundamentals' implicitly convert to bool for all of their values so we have to exclude them
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
class [[nodiscard]] result_or_error
{
public:
    /// \note Be liberal with the constructor argument type in order to allow
    /// emplacement/eliminate an intermediate move constructor call (as it can
    /// actually be nontrivial, e.g. classes w/ SBO).
    ///                                       (18.05.2015.) (Domagoj Saric)
    /// \todo Consider adding a runtime check to the 'successful'/Result
    /// constructor for Results convertible to bool (assuming this constitutes a
    /// 'validity' check) i.e. don't assume succeeded_ = true if the 'from
    /// result' constructor is invoked.
    ///                                       (17.02.2016.) (Domagoj Saric)
    // todo: variadic arguments
    template <typename Source>                                result_or_error( Source && BOOST_RESTRICTED_REF result, typename std::enable_if<std::is_constructible<Result, Source &&>::value>::type const * = nullptr ) noexcept( std::is_nothrow_constructible<Result, Source &&>::value ) : succeeded_( true  ), inspected_( false ), result_( std::forward<Source>( result ) ) {}
    template <typename Source> BOOST_ATTRIBUTES( BOOST_COLD ) result_or_error( Source && BOOST_RESTRICTED_REF error , typename std::enable_if<std::is_constructible<Error , Source &&>::value>::type const * = nullptr ) noexcept( std::is_nothrow_constructible<Error , Source &&>::value ) : succeeded_( false ), inspected_( false ), error_ ( std::forward<Source>( error  ) ) {}

    result_or_error( result_or_error && BOOST_RESTRICTED_REF other )
        noexcept
        (
            std::is_nothrow_move_constructible<Result>::value &&
            std::is_nothrow_move_constructible<Error >::value
        )
        : succeeded_( other.succeeded() ), inspected_( false )
    {
        /// \note MSVC14 still cannot 'see through' placement new and inserts
        /// braindead branching code so we help it with assume statements.
        ///                                   (18.05.2015.) (Domagoj Saric)
        if ( BOOST_LIKELY( succeeded_ ) )
        {
            auto * __restrict const ptr( new ( &result_ ) Result( std::move( other.result_ ) ) );
            BOOST_ASSUME( ptr              );
            BOOST_ASSUME( other.succeeded_ );
        }
        else
        {
            auto * __restrict const ptr( new ( &error_  ) Error ( std::move( other.error_  ) ) );
            BOOST_ASSUME( ptr               );
            BOOST_ASSUME( !other.succeeded_ );
        }
        BOOST_ASSUME( this->inspected_ == false );
        BOOST_ASSUME( other.inspected_ == true  );
    }

    result_or_error( result_or_error const & ) = delete;

BOOST_OPTIMIZE_FOR_SIZE_BEGIN()
    BOOST_ATTRIBUTES( BOOST_MINSIZE )
    ~result_or_error() noexcept( std::is_nothrow_destructible<Result>::value && std::is_nothrow_destructible<Error>::value )
    {
        /// \note This assertion is too naive: multiple result_or_error
        /// instances have to be supported (and allow early function exist after
        /// only one instance is expected and found as failed).
        ///                                   (05.01.2016.) (Domagoj Saric)
        /// \todo Move the fallible_result_sanitizer logic 'up here' (and find
        /// a way to) handle (ensure at least one inspected instance on scope
        /// exit) both types.
        /// This is problematic because the user might be using other forms of
        /// error handling or similar logic that would cause an early function
        /// exit (before the saved result_or_errors are inspected). Ultimately,
        /// this will probably have to be solved with (compiler specific) type
        /// or function attributes (e.g. GCC's warn_unused_result).
        ///                                   (05.01.2016.) (Domagoj Saric)
        //BOOST_ASSERT_MSG( inspected(), "Ignored error return code." );
        if ( BOOST_LIKELY( succeeded_ ) ) result_.~Result();
        else                              error_ .~Error ();
    };
BOOST_OPTIMIZE_FOR_SIZE_END()

    Error  const & error () const noexcept { BOOST_ASSERT_MSG( inspected(), "Using a result_or_error w/o prior inspection" ); BOOST_ASSERT_MSG( !succeeded_, "Querying the error of a succeeded operation." ); return error_ ; }
    Result       & result()       noexcept { BOOST_ASSERT_MSG( inspected(), "Using a result_or_error w/o prior inspection" ); BOOST_ASSERT_MSG(  succeeded_, "Querying the result of a failed operation."   ); return result_; }
    Result const & result() const noexcept { return const_cast<result_or_error &>( *this ).result(); }

    Result       && operator *  ()       && noexcept { return std::move( result() ); }
    Result       &  operator *  ()       &  noexcept { return            result()  ; }
    Result const &  operator *  () const &  noexcept { return            result()  ; }
    Result       &  operator -> ()          noexcept { return            result()  ; }
    Result const &  operator -> () const    noexcept { return            result()  ; }

    /// \note Automatic to-Result conversion makes it too easy to forget to
    /// first inspect the returned value for success.
    ///                                       (11.04.2017.) (Domagoj Saric)
    //operator Result       & ()       noexcept { return result(); }
    //operator Result const & () const noexcept { return result(); }

                      bool inspected() BOOST_RESTRICTED_THIS const noexcept {                    return BOOST_LIKELY( inspected_ ); }
                      bool succeeded() BOOST_RESTRICTED_THIS const noexcept { inspected_ = true; return BOOST_LIKELY( succeeded_ ); }
    explicit operator bool          () BOOST_RESTRICTED_THIS const noexcept {                    return succeeded()               ; }

    Result && assume_succeeded() && noexcept { BOOST_VERIFY( succeeded() ); return std::move( result() ); }

BOOST_OPTIMIZE_FOR_SIZE_BEGIN()
#ifndef BOOST_NO_EXCEPTIONS
    BOOST_ATTRIBUTES( BOOST_MINSIZE )
    void BOOST_CC_REG throw_if_error() BOOST_RESTRICTED_THIS
    {
        //BOOST_ASSERT( !std::uncaught_exception() );
        if ( succeeded() )
        {
            BOOST_ASSUME( inspected_ );
            BOOST_ASSUME( succeeded_ );
            return;
        }
        BOOST_ASSUME( inspected_ );
        throw_error();
    }
    BOOST_ATTRIBUTES( BOOST_MINSIZE )
    void BOOST_CC_REG throw_if_uninspected_error() BOOST_RESTRICTED_THIS
    {
        if ( !inspected() )
        {
            throw_if_error();
            BOOST_ASSUME( succeeded_ );
        }
        BOOST_ASSUME( inspected_ );
    }

    [[noreturn]] BOOST_ATTRIBUTES( BOOST_COLD )
    void BOOST_CC_REG throw_error() BOOST_RESTRICTED_THIS
    {
        BOOST_ASSERT( !succeeded() );
        //BOOST_ASSERT( !std::uncaught_exception() );
        make_and_throw_exception( std::move( error_ ) );
    }
#endif // BOOST_NO_EXCEPTIONS

    BOOST_ATTRIBUTES( BOOST_COLD )
    std::exception_ptr BOOST_CC_REG make_exception_ptr() noexcept
    {
        // http://en.cppreference.com/w/cpp/error/exception_ptr
        BOOST_ASSERT( !succeeded() );
        BOOST_ASSERT( !std::uncaught_exception() );
        return make_exception_ptr( std::move( error_ ) );
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
/// 'compressed' result_or_error specialisation for:
/// - default-constructible Results with operator bool AND
/// - empty Errors.
///
////////////////////////////////////////////////////////////////////////////////

template <class Result, class Error>
class [[nodiscard]] result_or_error<Result, Error, typename std::enable_if<compressed_result_error_variant<Result, Error>::value>::type>
{
public:
    template <typename Source>
    result_or_error( Source          && BOOST_RESTRICTED_REF result ) noexcept( std::is_nothrow_constructible     <Result, Source &&>::value ) : result_( std::forward<Source>( result ) ), inspected_( false ) {}
    result_or_error( result_or_error && BOOST_RESTRICTED_REF other  ) noexcept( std::is_nothrow_move_constructible<Result           >::value )
        :
        result_   ( std::move( other.result_ ) ),
        inspected_( false                      )
    {
        other.inspected_ = true;
        BOOST_ASSUME( this->inspected_ == false );
        BOOST_ASSUME( other.inspected_ == true  );
    }

    result_or_error( result_or_error const & ) = delete;

#if 0 // disabled
    ~result_or_error() noexcept { BOOST_ASSERT_MSG( inspected(), "Ignored error return code." ); };
#endif
    Result       && operator *  ()       && noexcept { return std::move( result() ); }
    Result       &  operator *  ()       &  noexcept { return            result()  ; }
    Result const &  operator *  () const &  noexcept { return            result()  ; }
    Result       &  operator -> ()          noexcept { return            result()  ; }
    Result const &  operator -> () const    noexcept { return            result()  ; }

    BOOST_ATTRIBUTES( BOOST_COLD )
    Error          error () const noexcept { BOOST_ASSERT_MSG( inspected() && !*this, "Querying the error of a (possibly) succeeded operation." ); return Error(); }
    Result       & result()       noexcept { BOOST_ASSERT_MSG( inspected() &&  *this, "Querying the result of a (possibly) failed operation."   ); return result_; }
    Result const & result() const noexcept { return const_cast<result_or_error &>( *this ).result(); }

                      bool inspected() const noexcept {                    return BOOST_LIKELY(                    inspected_ ); }
                      bool succeeded() const noexcept { inspected_ = true; return BOOST_LIKELY( static_cast<bool>( result_ )  ); }
    explicit operator bool          () const noexcept {                    return succeeded()                                  ; }

    Result && assume_succeeded() && noexcept { BOOST_VERIFY( succeeded() ); return std::move( result() ); }

    BOOST_OPTIMIZE_FOR_SIZE_BEGIN()
#ifndef BOOST_NO_EXCEPTIONS //...mrmlj...kill this duplication with the unspecialized template...
    BOOST_ATTRIBUTES( BOOST_MINSIZE )
    void throw_if_error()
    {
        //BOOST_ASSERT( !std::uncaught_exception() );
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
            throw_if_error();
        BOOST_ASSUME( inspected_ );
    }

    BOOST_ATTRIBUTES( BOOST_COLD )
    void throw_error()
    {
        BOOST_ASSERT( !succeeded() );
        //BOOST_ASSERT( !std::uncaught_exception() );
        detail::conditional_throw( error() );
    }
#endif // BOOST_NO_EXCEPTIONS

    BOOST_ATTRIBUTES( BOOST_COLD )
    std::exception_ptr BOOST_CC_REG make_exception_ptr()
    {
        BOOST_ASSERT( !succeeded() );
        BOOST_ASSERT( !std::uncaught_exception() );
        return make_exception_ptr( error() );
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
class [[nodiscard]] result_or_error<void, Error, void>
{
public:
    template <typename Source>
    BOOST_ATTRIBUTES( BOOST_COLD )
    result_or_error( Source && BOOST_RESTRICTED_REF error, typename std::enable_if<!std::is_same<Source, fallible_result<void, Error>>::value>::type const * = nullptr )
        noexcept( std::is_nothrow_constructible<Error, Source &&>::value )
        : error_( std::forward<Source>( error ) ), succeeded_( false ), inspected_( false ) {}

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
        other.inspected_ = true;
    }

    result_or_error( result_or_error const & ) = delete;

    BOOST_ATTRIBUTES( BOOST_MINSIZE )
    ~result_or_error() noexcept( std::is_nothrow_destructible<Error>::value )
    {
        /** @note
         * See above implementation note for generic version.
         *                            (02.02.2018.) (Nenad Miksa)
         */
        // BOOST_ASSERT_MSG( inspected(), "Ignored (error) return value." );
        if ( BOOST_UNLIKELY( !succeeded_ ) )
            error_.~Error();
    };

    BOOST_ATTRIBUTES( BOOST_COLD )
    Error const & error() const noexcept { BOOST_ASSERT_MSG( inspected() && !*this, "Querying the error of a (possibly) succeeded operation." ); return error_; }

                      bool inspected() const noexcept {                    return BOOST_LIKELY( inspected_ ); }
                      bool succeeded() const noexcept { auto & __restrict inspected( inspected_ ); inspected = true; return BOOST_LIKELY( succeeded_ ); }
    explicit operator bool          () const noexcept {                    return succeeded()               ; }

BOOST_OPTIMIZE_FOR_SIZE_BEGIN()
#ifndef BOOST_NO_EXCEPTIONS //...mrmlj...kill this duplication...
    BOOST_ATTRIBUTES( BOOST_MINSIZE )
    void throw_if_error()
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
            throw_if_error();
        BOOST_ASSUME( inspected_ );
    }

    [[noreturn]] BOOST_ATTRIBUTES( BOOST_COLD )
    void throw_error() noexcept( false )
    {
        BOOST_ASSERT( !succeeded() );
        BOOST_ASSERT( !std::uncaught_exception() );
        make_and_throw_exception( error() );
    }
#endif // BOOST_NO_EXCEPTIONS

    BOOST_ATTRIBUTES( BOOST_COLD )
    std::exception_ptr BOOST_CC_REG make_exception_ptr()
    {
        BOOST_ASSERT( !succeeded() );
        BOOST_ASSERT( !std::uncaught_exception() );
        return make_exception_ptr( error() );
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

#define BOOST_ERR_PROPAGATE_FAILURE( expression ) \
    { auto const result( expression ); if ( !result ) result.error(); }

#define BOOST_ERR_SAVE_RESULT_OR_PROPAGATE_FAILURE( result, expression ) \
    auto result( expression ); if ( !result ) result.error();

//------------------------------------------------------------------------------
} // namespace err
//------------------------------------------------------------------------------
} // namespace boost
//------------------------------------------------------------------------------
#endif // result_or_error_hpp

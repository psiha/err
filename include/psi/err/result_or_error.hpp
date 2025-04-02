////////////////////////////////////////////////////////////////////////////////
///
/// \file result_or_error.hpp
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
//------------------------------------------------------------------------------
#include "exceptions.hpp"

#include <boost/assert.hpp>
#include <boost/config_ex.hpp>

#include <type_traits>
#include <utility>
//------------------------------------------------------------------------------
namespace psi::err
{
//------------------------------------------------------------------------------

#ifdef NDEBUG
#   define PSI_RELEASE_FORCEINLINE BOOST_FORCEINLINE
#else
#   define PSI_RELEASE_FORCEINLINE
#endif

struct no_err_t {};
inline no_err_t constexpr no_err    = {};
inline no_err_t constexpr success   = {};
inline no_err_t constexpr succeeded = {};

struct an_err_t {};
inline an_err_t constexpr an_err  = {};
inline an_err_t constexpr failure = {};
inline an_err_t constexpr failed  = {};


template <class Result, class Error>
bool constexpr compressed_result_error_variant
{
    std::is_empty_v                <Error       > &&
    std::is_convertible_v          <Result, bool> &&
    std::is_default_constructible_v<Result      > &&
    !std::is_fundamental_v         <Result      >    // 'fundamentals' implicitly convert to bool for all of their values so we have to exclude them
}; //...mrmlj...todo/track std::is_explicitly_convertible


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
class [[ nodiscard, clang::trivial_abi ]] result_or_error
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
    template <typename Source> requires std::is_constructible_v<Result, Source &&>                                result_or_error( Source && __restrict result ) noexcept( std::is_nothrow_constructible_v<Result, Source &&> ) : succeeded_( true  ), inspected_( false ), result_( std::forward<Source>( result ) ) {}
    template <typename Source> requires std::is_constructible_v<Error , Source &&> BOOST_ATTRIBUTES( BOOST_COLD ) result_or_error( Source && __restrict error  ) noexcept( std::is_nothrow_constructible_v<Error , Source &&> ) : succeeded_( false ), inspected_( false ), error_ ( std::forward<Source>( error  ) ) {}

    result_or_error( Result && result ) : succeeded_( true  ), inspected_( false ), result_( std::forward< Result >( result ) ) {}
    result_or_error( Error  && error  ) : succeeded_( false ), inspected_( false ), error_ ( std::forward< Error  >( error  ) ) {}
    result_or_error( result_or_error const & ) = delete;

BOOST_OPTIMIZE_FOR_SIZE_BEGIN()
    BOOST_ATTRIBUTES( BOOST_MINSIZE )
    ~result_or_error() noexcept( std::is_nothrow_destructible_v<Result> && std::is_nothrow_destructible_v<Error> )
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
        /// exit (before the saved result_or_errors are inspected). Relying on
        /// [[ nodiscard ]] to flag these/remaining cases.
        ///                                   (05.01.2016.) (Domagoj Saric)
        //BOOST_ASSERT_MSG( inspected(), "Ignored error return code." );
        if ( BOOST_LIKELY( succeeded_ ) ) result_.~Result();
        else                              error_ .~Error ();
    };
BOOST_OPTIMIZE_FOR_SIZE_END()

    // See the note for propagate in fallible_result. For result_or_error the
    // unexpected/buggy behaviour NRVO may manifest is:
    //  - failure to get an assertion failure that you forgot to inspect the
    //    state of the result before calling result() or error() getters
    //  - throw_if_uninspected_error() might not throw when it should
    result_or_error propagate() noexcept( std::is_nothrow_move_constructible_v<Result> ) { return std::move( *this ); }

    fallible_result<Result, Error> as_fallible_result() noexcept( std::is_nothrow_move_constructible_v<Result> ) { return { std::move( *this ) }; }

    Error  const & error () const noexcept { BOOST_ASSERT_MSG( inspected(), "Using a result_or_error w/o prior inspection" ); BOOST_ASSERT_MSG( !succeeded_, "Querying the error of a succeeded operation." ); return error_ ; }
    Result       & result()       noexcept { BOOST_ASSERT_MSG( inspected(), "Using a result_or_error w/o prior inspection" ); BOOST_ASSERT_MSG(  succeeded_, "Querying the result of a failed operation."   ); return result_; }
    Result const & result() const noexcept { return const_cast<result_or_error &>( *this ).result(); }

    Result       && operator *  ()       && noexcept { return std::move     ( result() ); }
    Result       &  operator *  ()       &  noexcept { return                 result()  ; }
    Result const &  operator *  () const &  noexcept { return                 result()  ; }
    Result       *  operator -> ()          noexcept { return std::addressof( result() ); }
    Result const *  operator -> () const    noexcept { return std::addressof( result() ); }

    /// \note Automatic to-Result conversion makes it too easy to forget to
    /// first inspect the returned value for success.
    ///                                       (11.04.2017.) (Domagoj Saric)
    //operator Result       & ()       noexcept { return result(); }
    //operator Result const & () const noexcept { return result(); }

    [[ gnu::pure ]]   bool inspected() BOOST_RESTRICTED_THIS const noexcept {                    return BOOST_LIKELY( inspected_ ); }
    [[ gnu::pure ]]   bool succeeded() BOOST_RESTRICTED_THIS const noexcept { inspected_ = true; return BOOST_LIKELY( succeeded_ ); }
    explicit operator bool          () BOOST_RESTRICTED_THIS const noexcept {                    return succeeded()               ; }

    Result && assume_succeeded() && noexcept { BOOST_ASSUME( succeeded() ); return std::move( result() ); }

BOOST_OPTIMIZE_FOR_SIZE_BEGIN()

    BOOST_ATTRIBUTES( BOOST_MINSIZE )
    void BOOST_CC_REG throw_if_error() BOOST_RESTRICTED_THIS
    {
        //BOOST_ASSERT( !std::uncaught_exceptions() );
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
    void throw_if_uninspected_error() BOOST_RESTRICTED_THIS
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
        //BOOST_ASSERT( !std::uncaught_exceptions() );
        // Cast away the effective restrict qualifier from the error_ (causing
        // compilation errors).
        make_and_throw_exception( std::move( const_cast<Error &>( error_ ) ) );
    }

    BOOST_ATTRIBUTES( BOOST_COLD )
    std::exception_ptr BOOST_CC_REG make_exception_ptr() noexcept
    {
        // http://en.cppreference.com/w/cpp/error/exception_ptr
        BOOST_ASSERT( !succeeded() );
        BOOST_ASSERT( !std::uncaught_exceptions() );
        return make_exception_ptr( std::move( error_ ) );
    }
BOOST_OPTIMIZE_FOR_SIZE_END()

protected:
    result_or_error( result_or_error && __restrict other )
        noexcept
        (
            std::is_nothrow_move_constructible_v<Result> &&
            std::is_nothrow_move_constructible_v<Error >
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
requires compressed_result_error_variant<Result, Error>
class [[ nodiscard, clang::trivial_abi ]] result_or_error<Result, Error>
{
public:
    template <typename Source>
    result_or_error( Source && result ) noexcept( std::is_nothrow_constructible_v<Result, Source &&> )
        :
        result_{ std::forward<Source>( result ) }, inspected_{ false }
    {}
    result_or_error( result_or_error const & ) = delete;

#if 0 // disabled
    ~result_or_error() noexcept { BOOST_ASSERT_MSG( inspected(), "Ignored error return code." ); };
#endif

    result_or_error propagate() noexcept( std::is_nothrow_move_constructible_v<Result> ) { return std::move( *this ); }

    fallible_result<Result, Error> as_fallible_result() noexcept( std::is_nothrow_move_constructible_v<Result> ) { return { std::move( *this ) }; }

    Result       && operator *  ()       && noexcept { return std::move     ( result() ); }
    Result       &  operator *  ()       &  noexcept { return                 result()  ; }
    Result const &  operator *  () const &  noexcept { return                 result()  ; }
    Result       *  operator -> ()          noexcept { return std::addressof( result() ); }
    Result const *  operator -> () const    noexcept { return std::addressof( result() ); }

    BOOST_ATTRIBUTES( BOOST_COLD )
    Error          error () const noexcept { BOOST_ASSERT_MSG( inspected() && !*this, "Querying the error of a (possibly) succeeded operation." ); return Error(); }
    Result       & result()       noexcept { BOOST_ASSERT_MSG( inspected() &&  *this, "Querying the result of a (possibly) failed operation."   ); return result_; }
    Result const & result() const noexcept { return const_cast<result_or_error &>( *this ).result(); }

    [[ gnu::pure ]]   bool inspected() const noexcept {                    return BOOST_LIKELY(                    inspected_ ); }
    [[ gnu::pure ]]   bool succeeded() const noexcept { inspected_ = true; return BOOST_LIKELY( static_cast<bool>( result_ )  ); }
    explicit operator bool          () const noexcept {                    return succeeded()                                  ; }

    Result && assume_succeeded() && noexcept { BOOST_ASSUME( succeeded() ); return std::move( result() ); }

    BOOST_OPTIMIZE_FOR_SIZE_BEGIN()
#ifndef BOOST_NO_EXCEPTIONS //...mrmlj...kill this duplication with the unspecialized template...
    BOOST_ATTRIBUTES( BOOST_MINSIZE )
    void throw_if_error()
    {
        //BOOST_ASSERT( !std::uncaught_exceptions() );
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
        //BOOST_ASSERT( !std::uncaught_exceptions() );
        detail::conditional_throw( error() );
    }
#endif // BOOST_NO_EXCEPTIONS

    BOOST_ATTRIBUTES( BOOST_COLD )
    std::exception_ptr BOOST_CC_REG make_exception_ptr()
    {
        BOOST_ASSERT( !succeeded() );
        BOOST_ASSERT( !std::uncaught_exceptions() );
        return make_exception_ptr( error() );
    }
    BOOST_OPTIMIZE_FOR_SIZE_END()

protected:
    result_or_error( result_or_error && __restrict other  ) noexcept( std::is_nothrow_move_constructible_v<Result> )
        :
        result_   { std::move( other.result_ ) },
        inspected_{ false                      }
    {
        other.inspected_ = true;
        BOOST_ASSUME( this->inspected_ == false );
        BOOST_ASSUME( other.inspected_ == true  );
    }

private: friend class fallible_result<Result, Error>;
    Result result_;

protected:
    mutable bool inspected_;
}; // class result_or_error 'compressed' specialisation


////////////////////////////////////////////////////////////////////////////////
///
/// result_or_error<void, Error> specialization for void-return functions.
///
////////////////////////////////////////////////////////////////////////////////

template <class Error>
using void_or_error = result_or_error<void, Error>;

template <class Error>
class [[ nodiscard, clang::trivial_abi ]] result_or_error<void, Error>
{
public:
    template <typename Source>
    requires( !std::is_same_v<Source, fallible_result<void, Error>> )
    BOOST_ATTRIBUTES( BOOST_COLD )
    result_or_error( Source && __restrict error )
        noexcept( std::is_nothrow_constructible_v<Error, Source &&> )
        : 
        error_{ std::forward<Source>( error ) }, succeeded_{ false }, inspected_{ false } 
    {}
    result_or_error( no_err_t ) noexcept : succeeded_{ true }, inspected_{ false } {}
    result_or_error( result_or_error const & ) = delete;
    BOOST_ATTRIBUTES( BOOST_MINSIZE )
    ~result_or_error() noexcept( std::is_nothrow_destructible_v<Error> )
    {
        /** @note
         * See above implementation note for generic version.
         *                            (02.02.2018.) (Nenad Miksa)
         */
        // BOOST_ASSERT_MSG( inspected(), "Ignored (error) return value." );
        if ( !succeeded_ ) [[ unlikely ]]
            error_.~Error();
    };

    result_or_error              propagate         () noexcept( std::is_nothrow_move_constructible_v<Error> ) { return   std::move( *this )  ; }
    fallible_result<void, Error> as_fallible_result() noexcept( std::is_nothrow_move_constructible_v<Error> ) { return { std::move( *this ) }; }

    BOOST_ATTRIBUTES( BOOST_COLD )
    Error const & error() const noexcept { BOOST_ASSERT_MSG( inspected() && !*this, "Querying the error of a (possibly) succeeded operation." ); return error_; }

    [[ gnu::pure ]]   bool inspected() const noexcept {                    return BOOST_LIKELY( inspected_ ); }
    [[ gnu::pure ]]   bool succeeded() const noexcept { inspected_ = true; return BOOST_LIKELY( succeeded_ ); }
    explicit operator bool          () const noexcept {                    return succeeded()               ; }

    void assume_succeeded() && noexcept { BOOST_ASSUME( succeeded() ); }

BOOST_OPTIMIZE_FOR_SIZE_BEGIN()
#ifndef BOOST_NO_EXCEPTIONS //...mrmlj...kill this duplication...
    BOOST_ATTRIBUTES( BOOST_MINSIZE )
    void throw_if_error()
    {
        BOOST_ASSERT( !std::uncaught_exceptions() );
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

    [[ noreturn ]] BOOST_ATTRIBUTES( BOOST_COLD )
    void throw_error() noexcept( false )
    {
        BOOST_ASSERT( !succeeded() );
        BOOST_ASSERT( !std::uncaught_exceptions() );
        make_and_throw_exception( error() );
    }
#endif // BOOST_NO_EXCEPTIONS

    BOOST_ATTRIBUTES( BOOST_COLD )
    std::exception_ptr BOOST_CC_REG make_exception_ptr()
    {
        BOOST_ASSERT( !succeeded() );
        BOOST_ASSERT( !std::uncaught_exceptions() );
        return make_exception_ptr( error() );
    }
BOOST_OPTIMIZE_FOR_SIZE_END()

protected:
    result_or_error( result_or_error && __restrict other ) noexcept( std::is_nothrow_move_constructible_v<Error> )
        : succeeded_( other.succeeded() ), inspected_( false )
    {
        if ( !succeeded_ ) [[ unlikely ]]
        {
            auto const ptr( new ( &error_ ) Error ( std::move( other.error_ ) ) );
            BOOST_ASSUME( ptr );
            BOOST_ASSUME( !other.succeeded_ );
        }
        BOOST_ASSUME( other.inspected_ );
    }

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


template <typename Result, typename Error> bool operator==( result_or_error<Result, Error> const & result, no_err_t ) noexcept { return  result.succeeded(); }
template <typename Result, typename Error> bool operator==( result_or_error<Result, Error> const & result, an_err_t ) noexcept { return !result.succeeded(); }
template <typename Result, typename Error> bool operator!=( result_or_error<Result, Error> const & result, no_err_t ) noexcept { return !result.succeeded(); }
template <typename Result, typename Error> bool operator!=( result_or_error<Result, Error> const & result, an_err_t ) noexcept { return  result.succeeded(); }

#define PSI_ERR_PROPAGATE_FAILURE( expression ) \
    { auto const result( expression ); if ( !result ) result.error(); }

#define PSI_ERR_SAVE_RESULT_OR_PROPAGATE_FAILURE( result, expression ) \
    auto result( expression ); if ( !result ) result.error();

//------------------------------------------------------------------------------
} // namespace psi::err
//------------------------------------------------------------------------------

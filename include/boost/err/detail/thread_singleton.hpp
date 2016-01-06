////////////////////////////////////////////////////////////////////////////////
///
/// \file thread_singleton.hpp
/// --------------------------
///
/// Copyright (c) Domagoj Saric 2015 - 2016.
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
#ifndef thread_singleton_hpp__70DBC6F7_5CEC_47FA_8A79_9DBCB9B375C0
#define thread_singleton_hpp__70DBC6F7_5CEC_47FA_8A79_9DBCB9B375C0
#pragma once
//------------------------------------------------------------------------------
#include <boost/assert.hpp>
#include <boost/config.hpp>

#include <new>
#include <type_traits>

#ifndef BOOST_THREAD_LOCAL_POD
#include "../exceptions.hpp"

#include <pthread.h>
#endif // !BOOST_THREAD_LOCAL_POD
//------------------------------------------------------------------------------
namespace boost
{
//------------------------------------------------------------------------------
namespace err
{
//------------------------------------------------------------------------------
namespace detail
{
//------------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
///
/// \class thread_singleton
///
/// \brief BOOST_THREAD_LOCAL_POD alternative that works even on Android.
///
/// \detail Even in C++14 (but 'real') world we cannot 'just use' thread_local
/// or even __thread/_Thread_local:
/// - OSX/iOS/Apple Clang do not support the C++11 thread_local keyword
///   https://devforums.apple.com/message/1101679#1101679
///   http://comments.gmane.org/gmane.editors.textmate.general/38535
///   https://github.com/textmate/textmate/commit/172ce9d4282e408fe60b699c432390b9f6e3f74a
/// - __thread is not available on Android with Clang (see documentation/comment
///   for BOOST_THREAD_LOCAL_POD)
///                                           (16.12.2015.) (Domagoj Saric)
///
////////////////////////////////////////////////////////////////////////////////

#ifdef BOOST_THREAD_LOCAL_POD
template <typename T, typename Tag = void>
struct thread_singleton
{
    static_assert( std::is_pod<T>::value, "" );
    static T & instance()
    {
        static BOOST_THREAD_LOCAL_POD T instance_;
        return instance_;
    }
}; // struct thread_singleton
#else // BOOST_THREAD_LOCAL_POD
template <typename T, typename Tag = void>
class thread_singleton
{
public:
    BOOST_ATTRIBUTES( BOOST_RESTRICTED_FUNCTION_L3, BOOST_MINSIZE ) // may throw :/
    static T & instance()
    {
        struct wrapper { T instance; }; // workaround for arrays
        struct key_creator_t
        {
            BOOST_ATTRIBUTES( BOOST_COLD )
            key_creator_t()
            {
                auto const failure
                (
                    pthread_key_create
                    (
                        &tls_key_,
                        []( void * const p_object )
                        {
                            delete static_cast<wrapper *>( p_object );
                            BOOST_VERIFY( pthread_setspecific( tls_key_, nullptr ) == 0 );
                        }
                    )
                );
                if ( BOOST_UNLIKELY( failure ) )
                {
                    BOOST_ASSERT( errno == ENOMEM );
                    make_and_throw_exception<std::bad_alloc>();
                }
            }
            BOOST_ATTRIBUTES( BOOST_COLD )
            ~key_creator_t() noexcept { BOOST_VERIFY( pthread_key_delete( tls_key_ ) == 0 ); }
        }; // struct key_creator_t
        static key_creator_t const key_creator;
        BOOST_ASSERT( tls_key_ != static_cast<pthread_key_t>( -1 ) );

        auto * p_t( static_cast<wrapper *>( pthread_getspecific( tls_key_ ) ) );
        if ( BOOST_UNLIKELY( p_t == nullptr ) )
        {
            /// \note First allocate the TLS slot.
            ///                               (06.01.2016.) (Domagoj Saric)
            if ( BOOST_UNLIKELY( pthread_setspecific( tls_key_, &tls_key_ ) != 0 ) )
            {
                BOOST_ASSERT( errno == ENOMEM );
                make_and_throw_exception<std::bad_alloc>();
            }
            p_t = new wrapper();
            BOOST_VERIFY( pthread_setspecific( tls_key_, p_t ) == 0 );
        }
        return p_t->instance;
    }

private:
    static pthread_key_t  tls_key_;
    static pthread_once_t tls_allocation_guard_;
}; // class thread_singleton
template <typename T, typename Tag> pthread_key_t  thread_singleton<T, Tag>::tls_key_              = static_cast<pthread_key_t>( -1 );
template <typename T, typename Tag> pthread_once_t thread_singleton<T, Tag>::tls_allocation_guard_ = PTHREAD_ONCE_INIT;
#endif // !BOOST_THREAD_LOCAL_POD

//------------------------------------------------------------------------------
} // namespace detail
//------------------------------------------------------------------------------
} // namespace err
//------------------------------------------------------------------------------
} // namespace boost
//------------------------------------------------------------------------------
#endif // thread_singleton_hpp

////////////////////////////////////////////////////////////////////////////////
///
/// \file err.hpp
/// -------------
///
/// Copyright (c) Domagoj Saric 2015 - 2026.
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
#pragma once
//------------------------------------------------------------------------------
#include "fallible_result.hpp"
#include "result_or_error.hpp"
//------------------------------------------------------------------------------
namespace psi::err
{
//------------------------------------------------------------------------------

#ifdef _WIN32
struct last_win32_error;
using last_error = last_win32_error;
#else
struct last_errno;
using last_error = last_errno;
#endif

//------------------------------------------------------------------------------
} // namespace psi::err
//------------------------------------------------------------------------------

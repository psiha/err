// Module correctness smoke test (module.cmake, PSI_ERR_MODULE) - only built when the option is
// on, so it adds nothing to the default (off) build.
import psi.err;

#include <cerrno>
#include <exception>
#include <string>

namespace err = psi::err;

namespace
{
    // A representative fallible_result<Result, Error> user function: succeeds with a value or
    // fails with the current errno - the shape psi::err is meant for. `std::string` (rather than
    // `int`) is used as Result: an `int` Result reproduces a genuine, pre-existing and
    // self-documented ambiguity in result_or_error's constrained constructors - both the Result-
    // and the Error-constructor templates end up satisfied for an int-convertible Source, because
    // last_errno's own (explicit) `operator value_type()` makes `int` constructible from
    // `last_errno` too (see the \todo on that operator in errno.hpp). That is a pre-existing
    // library limitation, unrelated to and out of scope for module support - reported separately,
    // not fixed here.
    err::fallible_result<std::string, err::last_errno> describe( int const value )
    {
        if ( value <= 0 )
        {
            errno = EINVAL;
            return err::last_errno{};
        }
        return std::string( "positive" );
    }
} // anonymous namespace

int main()
{
    // Exercise all four free operator==/!= overloads (fallible_result.hpp, bottom) on both the
    // success and the failure path - the fix this test guards is
    // fallible_result<Result, Error>::succeeded() (added by this same change: it was missing on
    // the non-void primary template even though these operator overloads already assumed it
    // existed for every Result type, void included, so all four were uninstantiable for any
    // non-void Result before the fix). A failing result compared against err::failure is also a
    // regression check that succeeded() correctly marks the result inspected: otherwise the
    // temporary's destructor would (wrongly) throw on an already-handled failure.
    if ( !( describe( 42 ) == err::success ) ) return 1;
    if (    describe( 42 ) == err::failure   ) return 2;
    if (    describe( 42 ) != err::success   ) return 3;
    if ( !( describe( 42 ) != err::failure ) ) return 4;

    if (    describe( -1 ) == err::success   ) return 5;
    if ( !( describe( -1 ) == err::failure ) ) return 6;
    if ( !( describe( -1 ) != err::success ) ) return 7;
    if (    describe( -1 ) != err::failure   ) return 8;

    // Failure path: implicit conversion to the successful type throws the mapped exception.
    try
    {
        std::string const unused( describe( -1 ) );
        static_cast<void>( unused );
        return 9;
    }
    catch ( std::exception const & ) {}

    return 0;
}

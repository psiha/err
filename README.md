## err - yet another take on C++ error handling.

##### We have throw and std::error_code, what more could one possibly want?

std::error_code&co tried to solve or at least alleviate the problem of overhead of using exceptions (in terms of both source code verbosity and binary code size&speed) with functions which need to report failures that need not be 'exceptional' depending on the context of the call (thereby requiring try-catch blocks that convert exceptions to 'result codes').
Unfortunately, the way it was finally implemented, std::error_code did not quite accomplish what it set out to do:
* in terms of runtime efficiency: 'manual'/explicit EH (try-catch blocks) is no longer needed but the compiler still has to insert hidden EH as it has to treat the functions in question as still possibly throwing (in fact the implementation/codegen of functions which use std::error_code for error reporting becomes even fatter because it has to contain both return-on-error paths and handle exceptions)
* in terms of source code verbosity: again, try-catch blocks are gone but separate error_code objects have to be declared and passed to the desired function.

Enter _boost::err_: 'result objects' (as opposed to just result or error *codes*) which can be examined and branched upon (like with traditional result codes) or left unexamined to self-throw if they contain a failure result (i.e. unlike traditional result codes, they cannot be accidentally ignored). Thanks to latest C++ language features (such as r-value member function overloads) incorrect usage (that could otherwise for example lead to a result object destructor throwing during a stack-unwind) can be disallowed/detected at compile time.

Similar libraries and/or discussions where the concept (or a similar one) was first concieved of and/or so far discussed:
* https://github.com/ned14/boost.outcome
* http://cpptips.com/fallible
* https://github.com/ptal/std-expected-proposal
* https://github.com/ptal/expected
* https://github.com/adityaramesh/ccbase/blob/master/include/ccbase/error/expected.hpp
* http://boost.2283326.n4.nabble.com/system-filesystem-v3-Question-about-error-code-arguments-td2630789i40.html
* http://stackoverflow.com/questions/14923346/how-would-you-use-alexandrescus-expectedt-with-void-functions
* https://svn.boost.org/trac/boost/wiki/BestPracticeHandbook#a8.DESIGN:Stronglyconsiderusingconstexprsemanticwrappertransporttypestoreturnstatesfromfunctions
* http://2013.cppnow.org/session/non-allocating-stdfuturepromise

Boost.Err specific Boost.Devel discussion(s):
* http://boost.2283326.n4.nabble.com/err-RFC-td4681600.html

##### Other submodule requirements
(to be added to the include path _before_ the offical Boost distribution):
 * https://github.com/psiha/config

##### C++ In-The-Kernel Now!
##### Copyright © 2015 - 2016. Domagoj Saric. All rights reserved.

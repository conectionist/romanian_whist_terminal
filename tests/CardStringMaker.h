#ifndef TERMINAL_CARD_STRING_MAKER_H
#define TERMINAL_CARD_STRING_MAKER_H

#include <romanian_whist/Card.h>

#include <catch2/catch_tostring.hpp>

#include <string>

// Without this, a mismatched hand prints as "{ {?}, {?} }" - which says two
// hands differ but not how, on precisely the assertions this suite is made of.
//
// A near-copy of the engine's tests/CardStringMaker.h deliberately: that file
// sits in the engine's test directory, which is not an exported include path,
// so there is nothing to share short of the engine publishing a test-support
// target. Duplicating eight lines is the cheaper of the two.
//
// Include it at the top of every test file that compares Cards: one file that
// uses the primary template while another specialises it makes the program
// ill-formed, and no compiler has to say so.
namespace Catch
{
template<>
struct StringMaker<romanian_whist::Card>
{
    static std::string convert(const romanian_whist::Card& card)
    {
        return card.toString();
    }
};
}

#endif

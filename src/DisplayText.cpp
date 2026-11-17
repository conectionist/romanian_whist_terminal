#include "DisplayText.h"

#include <cstddef>
#include <optional>

namespace
{

struct CodePoint
{
    char32_t value;
    std::size_t length;
};

// Decodes the sequence starting at text[start], or returns nothing if it is not
// well-formed UTF-8. The second-byte ranges are the ones RFC 3629 allows, which
// is what rules out overlong forms, surrogates and anything past U+10FFFF.
std::optional<CodePoint> decode(std::string_view text, std::size_t start)
{
    const auto byteAt = [text](std::size_t index)
    {
        return static_cast<unsigned char>(text[index]);
    };

    const unsigned char lead = byteAt(start);
    if(lead < 0x80)
        return CodePoint{ lead, 1 };

    std::size_t length = 0;
    unsigned char secondLowest = 0x80;
    unsigned char secondHighest = 0xBF;

    if(lead >= 0xC2 && lead <= 0xDF)
        length = 2;
    else if(lead >= 0xE0 && lead <= 0xEF)
    {
        length = 3;

        if(lead == 0xE0)
            secondLowest = 0xA0;
        else if(lead == 0xED)
            secondHighest = 0x9F;
    }
    else if(lead >= 0xF0 && lead <= 0xF4)
    {
        length = 4;

        if(lead == 0xF0)
            secondLowest = 0x90;
        else if(lead == 0xF4)
            secondHighest = 0x8F;
    }
    else
        return std::nullopt;

    if(text.size() - start < length)
        return std::nullopt;

    const unsigned char second = byteAt(start + 1);
    if(second < secondLowest || second > secondHighest)
        return std::nullopt;

    char32_t value = static_cast<char32_t>(lead & (0xFFu >> (length + 1)));

    for(std::size_t i = 1 ; i < length ; i++)
    {
        const unsigned char next = byteAt(start + i);
        if((next & 0xC0) != 0x80)
            return std::nullopt;

        value = (value << 6) | static_cast<char32_t>(next & 0x3F);
    }

    return CodePoint{ value, length };
}

bool isControl(char32_t value)
{
    return value < 0x20 || (value >= 0x7F && value <= 0x9F);
}

} // namespace

bool DisplayText::isPrintable(std::string_view text)
{
    for(std::size_t i = 0 ; i < text.size() ; )
    {
        const std::optional<CodePoint> codePoint = decode(text, i);
        if(!codePoint || isControl(codePoint->value))
            return false;

        i += codePoint->length;
    }

    return true;
}

std::string DisplayText::escaped(std::string_view text)
{
    constexpr const char* HexDigits = "0123456789ABCDEF";

    std::string result;
    result.reserve(text.size());

    for(std::size_t i = 0 ; i < text.size() ; )
    {
        const std::optional<CodePoint> codePoint = decode(text, i);

        if(codePoint && !isControl(codePoint->value))
        {
            result += text.substr(i, codePoint->length);
            i += codePoint->length;
            continue;
        }

        const unsigned char byte = static_cast<unsigned char>(text[i]);

        result += "\\x";
        result += HexDigits[byte >> 4];
        result += HexDigits[byte & 0x0F];
        i++;
    }

    return result;
}

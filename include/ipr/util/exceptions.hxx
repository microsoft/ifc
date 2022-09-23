
#ifndef IPR_UTIL_EXCEPTIONS_H
#define IPR_UTIL_EXCEPTIONS_H

#include <exception>
#include <stdexcept>
#include <string>
#include <string_view>

namespace ipr
{
    class Unreachable : public std::exception
    {
    public:
        const char *what() const noexcept override
        {
            return "Code intended to be unreachable was executed";
        } 
    };

    class Not_yet_implemented : public std::logic_error
    {
    public:
        Not_yet_implemented(std::string_view what_is_not_implemented) :
            logic_error("'" + std::string(what_is_not_implemented) + "' is not yet implemented")
        {
        }
    };

    // We ran into something we did not expect.
    // This is likely to be an incorrect assumption on the part of the the programmer.
    // logic_error's are generally not recoverable. Asserts is the other alternative.
    class Unexpected : public std::logic_error
    {
    public:
        Unexpected(std::string_view what_is_not_expected) :
            logic_error("unexpected: '" + std::string(what_is_not_expected) + "'")
        {
        }
    };

    class Verification_error : public std::exception
    {
    public:
        Verification_error(std::string_view wrong_property_name) :
            error_message(
                "'" + std::string(wrong_property_name) + "' property was not filled correctly")
        {
        }

        const char *what() const noexcept override
        {
            return error_message.c_str();
        }

    private:
        std::string error_message;
    };

}  // namespace ipr

#endif  // IPR_UTIL_EXCEPTIONS_H

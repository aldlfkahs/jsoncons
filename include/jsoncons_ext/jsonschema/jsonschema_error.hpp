/// Copyright 2013-2023 Daniel Parker
// Distributed under the Boost license, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// See https://github.com/danielaparker/jsoncons for latest version

#ifndef JSONCONS_JSONSCHEMA_JSONSCHEMA_ERROR_HPP
#define JSONCONS_JSONSCHEMA_JSONSCHEMA_ERROR_HPP

#include <jsoncons/json_exception.hpp>
#include <system_error>

namespace jsoncons {
namespace jsonschema {

    class schema_error : public std::runtime_error, public virtual json_exception
    {
    public:
        schema_error(const std::string& message)
            : std::runtime_error(message)
        {
        }

        const char* what() const noexcept override
        {
            return std::runtime_error::what();
        }  
    };

    class validation_error : public std::runtime_error, public virtual json_exception
    {
    public:
        validation_error(const std::string& message)
            : std::runtime_error(message)
        {
        }

        const char* what() const noexcept override
        {
            return std::runtime_error::what();
        }  
    };

    class validation_output 
    {
        std::string keyword_;
        uri reference_;
        std::string instance_location_;
        std::string message_;
        std::vector<validation_output> nested_errors_;
    public:
        validation_output(std::string keyword,
                          uri reference,
                          std::string instance_location,
                          std::string message)
            : keyword_(std::move(keyword)), 
              reference_(std::move(reference)),
              instance_location_(std::move(instance_location)),
              message_(std::move(message))
        {
        }

        validation_output(const std::string& keyword,
                          const uri& reference,
                          const std::string& instance_location,
                          const std::string& message,
                          const std::vector<validation_output>& nested_errors)
            : keyword_(keyword),
              reference_(reference),
              instance_location_(instance_location), 
              message_(message),
              nested_errors_(nested_errors)
        {
        }

        const std::string& instance_location() const
        {
            return instance_location_;
        }

        const std::string& message() const
        {
            return message_;
        }

        const uri& reference() const
        {
            return reference_;
        }

        const std::string keyword_location() const
        {
            return "#" + std::string(reference_.fragment());
        }

        const std::string absolute_keyword_location() const
        {
            return reference_.string();
        }

        const std::string& keyword() const
        {
            return keyword_;
        }

        const std::vector<validation_output>& nested_errors() const
        {
            return nested_errors_;
        }
    };

} // namespace jsonschema
} // namespace jsoncons

#endif // JSONCONS_JSONSCHEMA_JSONSCHEMA_ERROR_HPP

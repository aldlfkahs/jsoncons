// Copyright 2013-2023 Daniel Parker
// Distributed under the Boost license, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// See https://github.com/danielaparker/jsoncons for latest version

#ifndef JSONCONS_JSONSCHEMA_COMMON_KEYWORDS_HPP
#define JSONCONS_JSONSCHEMA_COMMON_KEYWORDS_HPP

#include <jsoncons/config/jsoncons_config.hpp>
#include <jsoncons/uri.hpp>
#include <jsoncons/json.hpp>
#include <jsoncons_ext/jsonpointer/jsonpointer.hpp>
#include <jsoncons_ext/jsonschema/common/keyword_validators.hpp>
#include <cassert>
#include <set>
#include <sstream>
#include <iostream>
#include <cassert>
#include <unordered_set>
#if defined(JSONCONS_HAS_STD_REGEX)
#include <regex>
#endif

namespace jsoncons {
namespace jsonschema {


    template <class Json>
    class ref_keyword : public schema_keyword_base<Json>
    {
        using keyword_validator_type = std::unique_ptr<keyword_validator<Json>>;
        using schema_type = std::unique_ptr<schema<Json>>;

    public:
        ref_keyword<Json>(const uri& base_uri) : schema_keyword_base<Json>(base_uri)
        {}

        keyword_validator_type make_validator(const uri& base_uri, schema_registry<Json>& schemas) const override 
        {
            std::cout << "ref_keyword::make_validator base_uri: " << base_uri.string() << ", reference: " << this->reference().string() << "\n";
            schema<Json>* referred_schema = schemas.get_schema(this->reference());
            if (referred_schema == nullptr)
            {
                std::string message = this->reference().string() + " has undefined reference";
                JSONCONS_THROW(schema_error(message));

            }
            auto abs = referred_schema->reference().resolve(base_uri);
            auto validator = referred_schema->make_validator(abs, schemas);
            std::cout << "ref_keyword::make_validator schema reference: " << validator->reference().string() << "\n\n";

            return jsoncons::make_unique<ref_validator<Json>>(abs, std::move(validator));
        }

    private:

        void do_resolve_recursive_refs(const uri& /*base*/, bool /*has_recursive_anchor*/, 
            schema_registry<Json>& /*schemas*/) override
        {
            //std::cout << "ref_keyword location::do_resolve_recursive_refs: " << reference().string()
            //    << "\n  base: " << base.string() << ", has_recursive_anchor: " << has_recursive_anchor << "\n\n";

            //JSONCONS_ASSERT(referred_schema_)

            /*if (has_recursive_anchor)
            {
                referred_schema_->resolve_recursive_refs(base, has_recursive_anchor, schemas);
            }
            else
            {
                referred_schema_->resolve_recursive_refs(referred_schema_->reference(), referred_schema_->is_recursive_anchor(), schemas);
            }*/
        }
    };


    template <class Json>
    class recursive_ref_keyword : public schema_keyword_base<Json>
    {
        using keyword_validator_type = std::unique_ptr<keyword_validator<Json>>;
        using schema_type = std::unique_ptr<schema<Json>>;

        schema_type referred_schema_;

    public:
        recursive_ref_keyword(const uri& base_uri) : schema_keyword_base<Json>(base_uri), referred_schema_(nullptr)
        {}

        recursive_ref_keyword(const uri& base_uri, schema_type&& target)
            : schema_keyword_base<Json>(base_uri), referred_schema_(std::move(target)) {}

        keyword_validator_type make_validator(const jsoncons::uri& base_uri, schema_registry<Json>& schemas) const override 
        {
            //std::cout << "recursive_ref_keyword.make_validator " << "base_uri: << " << base_uri.string() << ", reference: " << this->reference().string() << "\n\n";

            auto uri = this->reference().resolve(base_uri);
            return jsoncons::make_unique<recursive_ref_validator<Json>>(uri, referred_schema_ ? referred_schema_->make_validator(base_uri, schemas) : nullptr);
        }

    private:

        void do_resolve_recursive_refs(const uri& base, bool has_recursive_anchor, schema_registry<Json>& schemas) override
        {
            uri relative("#");
            uri location;
            if (has_recursive_anchor)
            {
                location = relative.resolve(base);
            }
            else
            {
                location = this->reference();
            }
            referred_schema_ = schemas.get_schema(location)->make_validator(location, schemas);
            referred_schema_->resolve_recursive_refs(base, has_recursive_anchor, schemas);
            //std::cout << "recursive_ref_keyword::do_resolve_recursive_refs location: " << reference().string()
            //    << "\n  base: " << base.string() << ", has_recursive_anchor: " << has_recursive_anchor 
            //    << "\n  location: " << location.string() << "\n\n";
        }
    };


    // contentEncoding

    template <class Json>
    class content_encoding_keyword : public schema_keyword_base<Json>
    {
        using keyword_validator_type = std::unique_ptr<keyword_validator<Json>>;

        std::string content_encoding_;

    public:
        content_encoding_keyword(const uri& reference, const std::string& content_encoding)
            : schema_keyword_base<Json>(reference), 
              content_encoding_(content_encoding)
        {
        }

        keyword_validator_type make_validator(const uri& base_uri, schema_registry<Json>& /*schemas*/) const final 
        {
            return jsoncons::make_unique<content_encoding_validator<Json>>(this->reference().resolve(base_uri), content_encoding_);
        }

    private:
        void do_resolve_recursive_refs(const uri& /*base*/, bool /*has_recursive_anchor*/, schema_registry<Json>& /*schemas*/) override 
        {
        }
    };

    // contentMediaType

    template <class Json>
    class content_media_type_keyword : public schema_keyword_base<Json>
    {
        using keyword_validator_type = std::unique_ptr<keyword_validator<Json>>;

        std::string content_media_type_;

    public:
        content_media_type_keyword(const uri& reference, const std::string& content_media_type)
            : schema_keyword_base<Json>(reference), 
              content_media_type_(content_media_type)
        {
        }

        keyword_validator_type make_validator(const uri& base_uri, schema_registry<Json>& /*schemas*/) const final 
        {
            return jsoncons::make_unique<content_media_type_validator<Json>>(this->reference().resolve(base_uri), content_media_type_);
        }

    private:
        void do_resolve_recursive_refs(const uri& /*base*/, bool /*has_recursive_anchor*/, schema_registry<Json>& /*schemas*/)         override 
        {
        }
    };

    // format 

    template <class Json>
    class format_keyword : public schema_keyword_base<Json>
    {
        using keyword_validator_type = std::unique_ptr<keyword_validator<Json>>;

        format_checker format_check_;

    public:
        format_keyword(const uri& reference, const format_checker& format_check)
            : schema_keyword_base<Json>(reference), format_check_(format_check)
        {

        }

        keyword_validator_type make_validator(const uri& base_uri, schema_registry<Json>& /*schemas*/) const final 
        {
            return jsoncons::make_unique<format_validator<Json>>(this->reference().resolve(base_uri), format_check_);
        }

    private:
        void do_resolve_recursive_refs(const uri& /*base*/, bool /*has_recursive_anchor*/, schema_registry<Json>& /*schemas*/)         override 
        {
        }
    };

    // pattern 

#if defined(JSONCONS_HAS_STD_REGEX)
    template <class Json>
    class pattern_keyword : public schema_keyword_base<Json>
    {
        using keyword_validator_type = std::unique_ptr<keyword_validator<Json>>;

        std::string pattern_string_;
        std::regex regex_;

    public:
        pattern_keyword(const uri& reference,
            const std::string& pattern_string, const std::regex& regex)
            : schema_keyword_base<Json>(reference), 
              pattern_string_(pattern_string), regex_(regex)
        {
        }

        keyword_validator_type make_validator(const uri& base_uri, schema_registry<Json>& /*schemas*/) const final 
        {
            return jsoncons::make_unique<pattern_validator<Json>>(this->reference().resolve(base_uri), pattern_string_, regex_);
        }

    private:

        void do_resolve_recursive_refs(const uri& /*base*/, bool /*has_recursive_anchor*/, schema_registry<Json>& /*schemas*/)         override 
        {
        }
    };
#else
    template <class Json>
    class pattern_keyword : public schema_keyword_base<Json>
    {
        using keyword_validator_type = std::unique_ptr<keyword_validator<Json>>;

    public:
        pattern_keyword(const uri& reference)
            : schema_keyword_base<Json>(reference)
        {
        }

        keyword_validator_type make_validator(const uri& base_uri, schema_registry<Json>& /*schemas*/) const final 
        {
            return jsoncons::make_unique<pattern_validator<Json>>(this->reference().resolve(base_uri));
        }

    private:
        void do_resolve_recursive_refs(const uri& /*base*/, bool /*has_recursive_anchor*/, schema_registry<Json>& /*schemas*/)         override 
        {
        }
    };
#endif

    // maxLength

    template <class Json>
    class max_length_keyword : public schema_keyword_base<Json>
    {
        using keyword_validator_type = std::unique_ptr<keyword_validator<Json>>;

        std::size_t max_length_;
    public:
        max_length_keyword(const uri& reference, std::size_t max_length)
            : schema_keyword_base<Json>(reference), max_length_(max_length)
        {
        }

        keyword_validator_type make_validator(const uri& base_uri, schema_registry<Json>& /*schemas*/) const final 
        {
            return jsoncons::make_unique<max_length_validator<Json>>(this->reference().resolve(base_uri), max_length_);
        }

    private:

        void do_resolve_recursive_refs(const uri& /*base*/, bool /*has_recursive_anchor*/, schema_registry<Json>& /*schemas*/)         override 
        {
        }
    };

    // maxItems

    template <class Json>
    class max_items_keyword : public schema_keyword_base<Json>
    {
        using keyword_validator_type = std::unique_ptr<keyword_validator<Json>>;

        std::size_t max_items_;
    public:
        max_items_keyword(const uri& reference, std::size_t max_items)
            : schema_keyword_base<Json>(reference), max_items_(max_items)
        {
        }

        keyword_validator_type make_validator(const uri& base_uri, schema_registry<Json>& /*schemas*/) const final 
        {
            return jsoncons::make_unique<max_items_validator<Json>>(this->reference().resolve(base_uri), max_items_);
        }

    private:

        void do_resolve_recursive_refs(const uri& /*base*/, bool /*has_recursive_anchor*/, schema_registry<Json>& /*schemas*/)         override 
        {
        }
    };

    // minItems

    template <class Json>
    class min_items_keyword : public schema_keyword_base<Json>
    {
        using keyword_validator_type = typename schema_keyword<Json>::keyword_validator_type;

        std::size_t min_items_;
    public:
        min_items_keyword(const uri& reference, std::size_t min_items)
            : schema_keyword_base<Json>(reference), min_items_(min_items)
        {
        }

        keyword_validator_type make_validator(const uri& base_uri, schema_registry<Json>& /*schemas*/) const final 
        {
            return jsoncons::make_unique<min_items_validator<Json>>(this->reference().resolve(base_uri), min_items_);
        }

    private:

        void do_resolve_recursive_refs(const uri& /*base*/, bool /*has_recursive_anchor*/, schema_registry<Json>& /*schemas*/)         override 
        {
        }
    };

    // items

    template <class Json>
    class items_array_keyword : public schema_keyword_base<Json>
    {
        using schema_type = typename schema<Json>::schema_type;
        using keyword_type = typename schema<Json>::keyword_type;
        using schema_validator_type = typename schema<Json>::schema_validator_type;
        using keyword_validator_type = typename schema_keyword<Json>::keyword_validator_type;

        std::vector<schema_type> item_schemas_;
        schema_type additional_items_schema_;
    public:
        items_array_keyword(const uri& reference, 
            std::vector<schema_type>&& item_schemas,
            schema_type&& additional_items_schema)
            : schema_keyword_base<Json>(reference), 
              item_schemas_(std::move(item_schemas)), 
              additional_items_schema_(std::move(additional_items_schema))
        {
        }

        keyword_validator_type make_validator(const uri& base_uri, schema_registry<Json>& schemas) const final 
        {
            std::vector<schema_validator_type> item_validators;
            for (auto& item : item_schemas_)
            {
                item_validators.push_back(item->make_validator(base_uri, schemas));
            }
            schema_validator_type additional_items_validator; 
            
            if (additional_items_schema_)
            {
                additional_items_validator = additional_items_schema_->make_validator(base_uri, schemas);
            }

            return jsoncons::make_unique<items_array_validator<Json>>(this->reference().resolve(base_uri), std::move(item_validators),
                std::move(additional_items_validator));
        }

    private:

        void do_resolve_recursive_refs(const uri& base, bool has_recursive_anchor, schema_registry<Json>& schemas) override 
        {
            for (auto& item : item_schemas_)
            {
                item->resolve_recursive_refs(base, has_recursive_anchor, schemas);
            }
            if (additional_items_schema_)
            {
                additional_items_schema_->resolve_recursive_refs(base, has_recursive_anchor, schemas);
            }
        }
    };

    // contains

    template <class Json>
    class contains_keyword : public schema_keyword_base<Json>
    {
        using schema_validator_type = typename schema<Json>::schema_validator_type;
        using keyword_validator_type = typename schema_keyword<Json>::keyword_validator_type;
        using schema_type = typename schema<Json>::schema_type;

        schema_type validator_;
    public:
        contains_keyword(const uri& reference, 
            schema_type&& validator)
            : schema_keyword_base<Json>(reference), 
              validator_(std::move(validator))
        {
        }

        keyword_validator_type make_validator(const uri& base_uri, schema_registry<Json>& schemas) const final 
        {
            schema_validator_type validator;
            if (validator_)
            {
                validator = validator_->make_validator(base_uri, schemas);
            }

            return jsoncons::make_unique<contains_validator<Json>>(this->reference().resolve(base_uri),
                std::move(validator));
        }

    private:

        void do_resolve_recursive_refs(const uri& base, bool has_recursive_anchor, schema_registry<Json>& schemas) override 
        {
            if (validator_)
            {
                validator_->resolve_recursive_refs(base, has_recursive_anchor, schemas);
            }
        }
    };

    template <class Json>
    class items_object_keyword : public schema_keyword_base<Json>
    {
        using schema_validator_type = typename schema<Json>::schema_validator_type;
        using keyword_validator_type = typename schema_keyword<Json>::keyword_validator_type;
        using schema_type = typename schema<Json>::schema_type;

        schema_type items_validator_;
    public:
        items_object_keyword(const uri& reference, 
            schema_type&& items_validator)
            : schema_keyword_base<Json>(reference), 
              items_validator_(std::move(items_validator))
        {
        }

        keyword_validator_type make_validator(const uri& base_uri, schema_registry<Json>& schemas) const final 
        {
            schema_validator_type items_validator;
            if (items_validator_)
            {
                items_validator = items_validator_->make_validator(base_uri, schemas);
            }
            return jsoncons::make_unique<items_object_validator<Json>>(this->reference().resolve(base_uri),
                std::move(items_validator));
        }

    private:

        void do_resolve_recursive_refs(const uri& base, bool has_recursive_anchor, schema_registry<Json>& schemas) override 
        {
            if (items_validator_)
            {
                items_validator_->resolve_recursive_refs(base, has_recursive_anchor, schemas);
            }
        }
    };

    // uniqueItems

    template <class Json>
    class unique_items_keyword : public schema_keyword_base<Json>
    {
        using keyword_validator_type = std::unique_ptr<keyword_validator<Json>>;

        bool are_unique_;
    public:
        unique_items_keyword(const uri& reference, bool are_unique)
            : schema_keyword_base<Json>(reference), are_unique_(are_unique)
        {
        }

        keyword_validator_type make_validator(const uri& base_uri, schema_registry<Json>& /*schemas*/) const final 
        {
            return jsoncons::make_unique<unique_items_validator<Json>>(this->reference().resolve(base_uri),
                are_unique_);
        }

    private:

        void do_resolve_recursive_refs(const uri& /*base*/, bool /*has_recursive_anchor*/, schema_registry<Json>& /*schemas*/)         override 
        {
        }
    };

    // minLength

    template <class Json>
    class min_length_keyword : public schema_keyword_base<Json>
    {
        using keyword_validator_type = std::unique_ptr<keyword_validator<Json>>;

        std::size_t min_length_;

    public:
        min_length_keyword(const uri& reference, std::size_t min_length)
            : schema_keyword_base<Json>(reference), min_length_(min_length)
        {
        }

        keyword_validator_type make_validator(const uri& base_uri, schema_registry<Json>& /*schemas*/) const final 
        {
            return jsoncons::make_unique<min_length_validator<Json>>(this->reference().resolve(base_uri),
                min_length_);
        }

    private:

        void do_resolve_recursive_refs(const uri& /*base*/, bool /*has_recursive_anchor*/, schema_registry<Json>& /*schemas*/)         override 
        {
        }
    };

    // string 

    template <class Json>
    class string_keyword : public schema_keyword_base<Json>
    {
        using keyword_type = typename std::unique_ptr<schema_keyword<Json>>;
        using keyword_validator_type = typename std::unique_ptr<keyword_validator<Json>>;

        std::vector<keyword_type> keywords_;
    public:
        string_keyword(const uri& reference,
            std::vector<keyword_type>&& keywords)
            : schema_keyword_base<Json>(reference), keywords_(std::move(keywords))
        {
        }

        keyword_validator_type make_validator(const uri& base_uri, schema_registry<Json>& schemas) const final 
        {
            std::vector<keyword_validator_type> validators;
            for (auto& keyword : keywords_)
            {
                validators.emplace_back(keyword->make_validator(base_uri, schemas));
            }

            return jsoncons::make_unique<string_validator<Json>>(this->reference().resolve(base_uri),
                std::move(validators));
        }

    private:

        void do_resolve_recursive_refs(const uri& base, bool has_recursive_anchor, schema_registry<Json>& schemas) override 
        {
            for (auto& keyword : keywords_)
            {
                keyword->resolve_recursive_refs(base, has_recursive_anchor, schemas);
            }
        }
    };

    // not

    template <class Json>
    class not_keyword : public schema_keyword_base<Json>
    {
        using schema_validator_type = typename schema<Json>::schema_validator_type;
        using keyword_validator_type = typename schema_keyword<Json>::keyword_validator_type;
        using schema_type = typename schema<Json>::schema_type;

        schema_type rule_;

    public:
        not_keyword(const uri& reference,
            schema_type&& rule)
            : schema_keyword_base<Json>(reference), 
              rule_(std::move(rule))
        {
        }

        keyword_validator_type make_validator(const uri& base_uri, schema_registry<Json>& schemas) const final 
        {
            schema_validator_type rule;
            if (rule_)
            {
                rule = rule_->make_validator(base_uri, schemas);
            }

            return jsoncons::make_unique<not_validator<Json>>(this->reference().resolve(base_uri),
                std::move(rule));
        }

    private:

        void do_resolve_recursive_refs(const uri& base, bool has_recursive_anchor, schema_registry<Json>& schemas) override 
        {
            if (rule_)
            {
                rule_->resolve_recursive_refs(base, has_recursive_anchor, schemas);
            }
        }

    };

    template <class Json,class Criterion>
    class combining_keyword : public schema_keyword_base<Json>
    {
        using schema_validator_type = typename schema<Json>::schema_validator_type;
        using keyword_validator_type = typename schema_keyword<Json>::keyword_validator_type;
        using schema_type = typename schema<Json>::schema_type;

        std::vector<schema_type> schemas_;

    public:
        combining_keyword(const uri& reference,
             std::vector<schema_type>&& schemas)
            : schema_keyword_base<Json>(std::move(reference)),
              schemas_(std::move(schemas))
        {
        }

        keyword_validator_type make_validator(const uri& base_uri, schema_registry<Json>& schemas) const final 
        {
            std::vector<schema_validator_type> validators;
            for (auto& schema : schemas_)
            {
                validators.emplace_back(schema->make_validator(base_uri, schemas));
            }

            return jsoncons::make_unique<combining_validator<Json,Criterion>>(this->reference().resolve(base_uri),
                std::move(validators));
        }

    private:

        void do_resolve_recursive_refs(const uri& base, bool has_recursive_anchor, schema_registry<Json>& schemas) override 
        {
            for (auto& schema : schemas_)
            {
                schema->resolve_recursive_refs(base, has_recursive_anchor, schemas);
            }
        }
    };

    template <class Json,class T>
    class maximum_keyword : public schema_keyword_base<Json>
    {
        using keyword_validator_type = std::unique_ptr<keyword_validator<Json>>;

        T value_;

    public:
        maximum_keyword(const uri& reference, T value)
            : schema_keyword_base<Json>(reference), value_(value)
        {
        }

        keyword_validator_type make_validator(const uri& base_uri, schema_registry<Json>& /*schemas*/) const final 
        {
            return jsoncons::make_unique<maximum_validator<Json,T>>(this->reference().resolve(base_uri),
                value_);
        }

    private:

        void do_resolve_recursive_refs(const uri& /*base*/, bool /*has_recursive_anchor*/, schema_registry<Json>& /*schemas*/)         override
        {
        }
    };

    template <class Json,class T>
    class exclusive_maximum_keyword : public schema_keyword_base<Json>
    {
        using keyword_validator_type = std::unique_ptr<keyword_validator<Json>>;

        T value_;

    public:
        exclusive_maximum_keyword(const uri& reference, T value)
            : schema_keyword_base<Json>(reference), value_(value)
        {
        }

        keyword_validator_type make_validator(const uri& base_uri, schema_registry<Json>& /*schemas*/) const final 
        {
            return jsoncons::make_unique<exclusive_maximum_validator<Json,T>>(this->reference().resolve(base_uri),
                value_);
        }

    private:

        void do_resolve_recursive_refs(const uri& /*base*/, bool /*has_recursive_anchor*/, schema_registry<Json>& /*schemas*/)         override 
        {
        }
    };

    template <class Json,class T>
    class minimum_keyword : public schema_keyword_base<Json>
    {
        using keyword_validator_type = std::unique_ptr<keyword_validator<Json>>;

        T value_;

    public:
        minimum_keyword(const uri& reference, T value)
            : schema_keyword_base<Json>(reference), value_(value)
        {
        }

        keyword_validator_type make_validator(const uri& base_uri, schema_registry<Json>& /*schemas*/) const final 
        {
            //std::cout << "make_validator minimum " << this->reference().string() << ", base: " << base_uri.string() << ", resolve: " << this->reference().resolve(base_uri).string() << "\n\n"; 

            return jsoncons::make_unique<minimum_validator<Json,T>>(this->reference().resolve(base_uri),
                value_);
        }

    private:

        void do_resolve_recursive_refs(const uri& /*base*/, bool /*has_recursive_anchor*/, schema_registry<Json>& /*schemas*/)         override 
        {
        }
    };

    template <class Json,class T>
    class exclusive_minimum_keyword : public schema_keyword_base<Json>
    {
        using keyword_validator_type = std::unique_ptr<keyword_validator<Json>>;

        T value_;

    public:
        exclusive_minimum_keyword(const uri& reference, T value)
            : schema_keyword_base<Json>(reference), value_(value)
        {
        }

        keyword_validator_type make_validator(const uri& base_uri, schema_registry<Json>& /*schemas*/) const final 
        {
            return jsoncons::make_unique<exclusive_minimum_validator<Json,T>>(this->reference().resolve(base_uri),
                value_);
        }

    private:

        void do_resolve_recursive_refs(const uri& /*base*/, bool /*has_recursive_anchor*/, schema_registry<Json>& /*schemas*/)         override
        {
        }
    };

    template <class Json>
    class multiple_of_keyword : public schema_keyword_base<Json>
    {
        using keyword_validator_type = std::unique_ptr<keyword_validator<Json>>;

        double value_;

    public:
        multiple_of_keyword(const uri& reference, double value)
            : schema_keyword_base<Json>(reference), value_(value)
        {
        }

        keyword_validator_type make_validator(const uri& base_uri, schema_registry<Json>& /*schemas*/) const final 
        {
            return jsoncons::make_unique<multiple_of_validator<Json>>(this->reference().resolve(base_uri),
                value_);
        }

    private:

        void do_resolve_recursive_refs(const uri& /*base*/, bool /*has_recursive_anchor*/, schema_registry<Json>& /*schemas*/)         override 
        {
        }
    };

    template <class Json>
    class integer_keyword : public schema_keyword_base<Json>
    {
        using keyword_validator_type = typename std::unique_ptr<keyword_validator<Json>>;
        using keyword_type = typename std::unique_ptr<schema_keyword<Json>>;

        std::vector<keyword_type> keywords_;
    public:
        integer_keyword(const uri& reference, 
            std::vector<keyword_type>&& keywords)
            : schema_keyword_base<Json>(reference), keywords_(std::move(keywords))
        {
        }

        keyword_validator_type make_validator(const uri& base_uri, schema_registry<Json>& schemas) const final 
        {
            std::vector<keyword_validator_type> validators;
            for (auto& keyword : keywords_)
            {
                validators.emplace_back(keyword->make_validator(base_uri, schemas));
            }

            return jsoncons::make_unique<integer_validator<Json>>(this->reference().resolve(base_uri),
                std::move(validators));
        }

    private:

        void do_resolve_recursive_refs(const uri& base, bool has_recursive_anchor, schema_registry<Json>& schemas) override 
        {
            for (auto& validator : keywords_)
            {
                validator->resolve_recursive_refs(base, has_recursive_anchor, schemas);
            }
        }
    };

    template <class Json>
    class number_keyword : public schema_keyword_base<Json>
    {
        using keyword_validator_type = typename std::unique_ptr<keyword_validator<Json>>;
        using keyword_type = typename std::unique_ptr<schema_keyword<Json>>;

        std::vector<keyword_type> keywords_;
    public:
        number_keyword(const uri& reference, 
            std::vector<keyword_type>&& keywords)
            : schema_keyword_base<Json>(reference), keywords_(std::move(keywords))
        {
        }

        keyword_validator_type make_validator(const uri& base_uri, schema_registry<Json>& schemas) const final 
        {
            std::vector<keyword_validator_type> validators;
            for (auto& keyword : keywords_)
            {
                validators.emplace_back(keyword->make_validator(base_uri, schemas));
            }

            return jsoncons::make_unique<number_validator<Json>>(this->reference().resolve(base_uri),
                std::move(validators));
        }

    private:

        void do_resolve_recursive_refs(const uri& base, bool has_recursive_anchor, schema_registry<Json>& schemas) override 
        {
            for (auto& keyword : keywords_)
            {
                keyword->resolve_recursive_refs(base, has_recursive_anchor, schemas);
            }
        }
    };

    // null

    template <class Json>
    class null_keyword : public schema_keyword_base<Json>
    {
        using keyword_validator_type = typename std::unique_ptr<keyword_validator<Json>>;
        using keyword_type = typename std::unique_ptr<schema_keyword<Json>>;

    public:
        null_keyword(const uri& reference)
            : schema_keyword_base<Json>(reference)
        {
        }

        keyword_validator_type make_validator(const uri& base_uri, schema_registry<Json>& /*schemas*/) const final 
        {
            return jsoncons::make_unique<null_validator<Json>>(this->reference().resolve(base_uri));
        }

    private:

        void do_resolve_recursive_refs(const uri& /*base*/, bool /*has_recursive_anchor*/, schema_registry<Json>& /*schemas*/)         override 
        {
        }
    };

    template <class Json>
    class boolean_keyword : public schema_keyword_base<Json>
    {
        using keyword_validator_type = typename std::unique_ptr<keyword_validator<Json>>;
        using keyword_type = typename std::unique_ptr<schema_keyword<Json>>;

    public:
        boolean_keyword(const uri& reference)
            : schema_keyword_base<Json>(reference)
        {
        }

        keyword_validator_type make_validator(const uri& base_uri, schema_registry<Json>& /*schemas*/) const final 
        {
            return jsoncons::make_unique<boolean_validator<Json>>(this->reference().resolve(base_uri));
        }

    private:

        void do_resolve_recursive_refs(const uri& /*base*/, bool /*has_recursive_anchor*/, schema_registry<Json>& /*schemas*/)         override 
        {
        }
    };

    template <class Json>
    class required_keyword : public schema_keyword_base<Json>
    {
        using keyword_validator_type = std::unique_ptr<keyword_validator<Json>>;

        std::vector<std::string> items_;

    public:
        required_keyword(const uri& reference,
            const std::vector<std::string>& items)
            : schema_keyword_base<Json>(reference), items_(items)
        {
        }

        required_keyword(const required_keyword&) = delete;
        required_keyword(required_keyword&&) = default;
        required_keyword& operator=(const required_keyword&) = delete;
        required_keyword& operator=(required_keyword&&) = default;

        keyword_validator_type make_validator(const uri& base_uri, schema_registry<Json>& /*schemas*/) const final 
        {
            return jsoncons::make_unique<required_validator<Json>>(this->reference().resolve(base_uri),
                items_);
        }

    private:

        void do_resolve_recursive_refs(const uri& /*base*/, bool /*has_recursive_anchor*/, schema_registry<Json>& /*schemas*/)         override 
        {
        }

    };

    // maxProperties

    template <class Json>
    class max_properties_keyword : public schema_keyword_base<Json>
    {
        using keyword_validator_type = std::unique_ptr<keyword_validator<Json>>;

        std::size_t max_properties_;
    public:
        max_properties_keyword(const uri& reference, std::size_t max_properties)
            : schema_keyword_base<Json>(reference), max_properties_(max_properties)
        {
        }

        keyword_validator_type make_validator(const uri& base_uri, schema_registry<Json>& /*schemas*/) const final 
        {
            return jsoncons::make_unique<max_properties_validator<Json>>(this->reference().resolve(base_uri), max_properties_);
        }

    private:

        void do_resolve_recursive_refs(const uri& /*base*/, bool /*has_recursive_anchor*/, schema_registry<Json>& /*schemas*/)         override 
        {
        }

    };

    // minProperties

    template <class Json>
    class min_properties_keyword : public schema_keyword_base<Json>
    {
        using keyword_validator_type = std::unique_ptr<keyword_validator<Json>>;

        std::size_t min_properties_;
    public:
        min_properties_keyword(const uri& reference, std::size_t min_properties)
            : schema_keyword_base<Json>(reference), min_properties_(min_properties)
        {
        }

        keyword_validator_type make_validator(const uri& base_uri, schema_registry<Json>& /*schemas*/) const final 
        {
            return jsoncons::make_unique<min_properties_validator<Json>>(this->reference().resolve(base_uri), min_properties_);
        }

    private:

        void do_resolve_recursive_refs(const uri& /*base*/, bool /*has_recursive_anchor*/, schema_registry<Json>& /*schemas*/)         override 
        {
        }

    };

    template <class Json>
    class object_keyword : public schema_keyword_base<Json>
    {
        using keyword_validator_type = typename std::unique_ptr<keyword_validator<Json>>;
        using keyword_type = typename std::unique_ptr<schema_keyword<Json>>;
        using schema_type = typename schema<Json>::schema_type;
        using schema_validator_type = typename schema<Json>::schema_validator_type;

        std::vector<keyword_type> general_keywords_;

        std::map<std::string, schema_type> properties_;
    #if defined(JSONCONS_HAS_STD_REGEX)
        std::vector<std::pair<std::regex, schema_type>> pattern_properties_;
    #endif
        schema_type additional_properties_;

        std::map<std::string, keyword_type> dependent_required_;
        std::map<std::string, schema_type> dependent_schemas_;

        schema_type property_name_keyword_;

    public:
        object_keyword(const uri& reference,
            std::vector<keyword_type>&& general_keywords,
            std::map<std::string, schema_type>&& properties,
        #if defined(JSONCONS_HAS_STD_REGEX)
            std::vector<std::pair<std::regex, schema_type>>&& pattern_properties,
        #endif
            schema_type&& additional_properties,
            std::map<std::string, keyword_type>&& dependent_required,
            std::map<std::string, schema_type>&& dependent_schemas,
            schema_type&& property_name_keyword
        )
            : schema_keyword_base<Json>(std::move(reference)), 
              general_keywords_(std::move(general_keywords)),
              properties_(std::move(properties)),
        #if defined(JSONCONS_HAS_STD_REGEX)
              pattern_properties_(std::move(pattern_properties)),
        #endif
              additional_properties_(std::move(additional_properties)),
              dependent_required_(std::move(dependent_required)),
              dependent_schemas_(std::move(dependent_schemas)),
              property_name_keyword_(std::move(property_name_keyword))
        {
        }

        keyword_validator_type make_validator(const uri& base_uri, schema_registry<Json>& schemas) const final 
        {
            std::vector<keyword_validator_type> general_validators;
            for (auto& keyword : general_keywords_)
            {
                general_validators.emplace_back(keyword->make_validator(base_uri, schemas));
            }

            std::map<std::string, schema_validator_type> properties;
            for (auto& item : properties_)
            {
                properties.emplace(item.first, item.second->make_validator(base_uri, schemas));
            }


        #if defined(JSONCONS_HAS_STD_REGEX)
            std::vector<std::pair<std::regex, schema_validator_type>> pattern_properties;
            for (auto& item : pattern_properties_)
            {
                pattern_properties.emplace_back(item.first, item.second->make_validator(base_uri, schemas));
            }
        #endif
            schema_validator_type additional_properties;
            if (additional_properties_)
            {
                additional_properties = additional_properties_->make_validator(base_uri, schemas);
            }

            std::map<std::string, keyword_validator_type> dependent_required;
            for (auto& item : dependent_required_)
            {
                dependent_required.emplace(item.first, item.second->make_validator(base_uri, schemas));
            }

            std::map<std::string, schema_validator_type> dependent_schemas;
            for (auto& item : dependent_schemas_)
            {
                dependent_schemas.emplace(item.first, item.second->make_validator(base_uri, schemas));
            }

            schema_validator_type property_name_validator;
            if (property_name_keyword_)
            {
                property_name_validator = property_name_keyword_->make_validator(base_uri, schemas);
            }

            return jsoncons::make_unique<object_validator<Json>>(this->reference().resolve(base_uri),
                std::move(general_validators), std::move(properties),
        #if defined(JSONCONS_HAS_STD_REGEX)
                std::move(pattern_properties),
        #endif
                std::move(additional_properties),
                std::move(dependent_required),
                std::move(dependent_schemas),
                std::move(property_name_validator));
        }

    private:

        void do_resolve_recursive_refs(const uri& base, bool has_recursive_anchor, schema_registry<Json>& schemas) override 
        {
            for (auto& keyword : general_keywords_)
            {
                keyword->resolve_recursive_refs(base, has_recursive_anchor, schemas);
            }
            for (auto& item : properties_)
            {
                item.second->resolve_recursive_refs(base, has_recursive_anchor, schemas);
            }
    #if defined(JSONCONS_HAS_STD_REGEX)
            for (auto& item : pattern_properties_)
            {
                item.second->resolve_recursive_refs(base, has_recursive_anchor, schemas);
            }
    #endif
            for (auto& item : dependent_required_)
            {
                item.second->resolve_recursive_refs(base, has_recursive_anchor, schemas);
            }
            for (auto& item : dependent_schemas_)
            {
                item.second->resolve_recursive_refs(base, has_recursive_anchor, schemas);
            }
            if (additional_properties_)
                additional_properties_->resolve_recursive_refs(base, has_recursive_anchor, schemas);
            if (property_name_keyword_)
                property_name_keyword_->resolve_recursive_refs(base, has_recursive_anchor, schemas);
        }

    };

    template <class Json>
    class unevaluated_properties_keyword : public schema_keyword_base<Json>
    {
        using keyword_validator_type = typename schema_keyword<Json>::keyword_validator_type;
        using schema_type = typename schema<Json>::schema_type;
        using schema_validator_type = typename schema_validator<Json>::schema_validator_type;

        schema_type schema_;

    public:
        unevaluated_properties_keyword(const uri& reference,
            schema_type&& schema
        )
            : schema_keyword_base<Json>(std::move(reference)), 
              schema_(std::move(schema))
        {
        }

        keyword_validator_type make_validator(const uri& base_uri, schema_registry<Json>& schemas) const final 
        {
            schema_validator_type validator;
            if (schema_)
            {
                validator = schema_->make_validator(base_uri, schemas);
            }
            return jsoncons::make_unique<unevaluated_properties_validator<Json>>(this->reference().resolve(base_uri),
                std::move(validator));
        }

    private:

        void do_resolve_recursive_refs(const uri& base, bool has_recursive_anchor, schema_registry<Json>& schemas) override 
        {
            if (schema_)
            {
                schema_->resolve_recursive_refs(base, has_recursive_anchor, schemas);
            }
        }
    };

    // array

    template <class Json>
    class array_keyword : public schema_keyword_base<Json>
    {
        using keyword_type = typename schema<Json>::keyword_type;
        using keyword_validator_type = typename schema_keyword<Json>::keyword_validator_type;

        std::vector<keyword_type> keywords_;
    public:
        array_keyword(const uri& reference, std::vector<keyword_type>&& keywords)
            : schema_keyword_base<Json>(reference), keywords_(std::move(keywords))
        {
        }

        keyword_validator_type make_validator(const uri& base_uri, schema_registry<Json>& schemas) const final 
        {
            std::vector<keyword_validator_type> validators;
            for (auto& keyword : keywords_)
            {
                validators.emplace_back(keyword->make_validator(base_uri, schemas));
            }

            return jsoncons::make_unique<array_validator<Json>>(this->reference().resolve(base_uri),
                std::move(validators));
        }

    private:

        void do_resolve_recursive_refs(const uri& base, bool has_recursive_anchor, schema_registry<Json>& schemas) override 
        {
            for (auto& keyword : keywords_)
            {
                keyword->resolve_recursive_refs(base, has_recursive_anchor, schemas);
            }
        }
   };

    template <class Json>
    class conditional_keyword : public schema_keyword_base<Json>
    {
        using keyword_validator_type = typename schema_keyword<Json>::keyword_validator_type;
        using keyword_type = typename schema<Json>::keyword_type;
        using schema_type = typename schema<Json>::schema_type;
        using schema_validator_type = typename schema<Json>::schema_validator_type;

        schema_type if_schema_;
        schema_type then_schema_;
        schema_type else_schema_;

    public:
        conditional_keyword(const uri& reference,
          schema_type&& if_schema,
          schema_type&& then_schema,
          schema_type&& else_schema
        ) : schema_keyword_base<Json>(reference), 
              if_schema_(std::move(if_schema)), 
              then_schema_(std::move(then_schema)), 
              else_schema_(std::move(else_schema))
        {
        }

        keyword_validator_type make_validator(const uri& base_uri, schema_registry<Json>& schemas) const final 
        {
            schema_validator_type if_schema;
            if (if_schema_)
            {
                if_schema = if_schema_->make_validator(base_uri, schemas);
            }
            schema_validator_type then_schema;
            if (then_schema_)
            {
                then_schema = then_schema_->make_validator(base_uri, schemas);
            }
            schema_validator_type else_schema;
            if (else_schema_)
            {
                else_schema = else_schema_->make_validator(base_uri, schemas);
            }

            return jsoncons::make_unique<conditional_validator<Json>>(this->reference().resolve(base_uri), 
                std::move(if_schema), std::move(then_schema), std::move(else_schema));
        }

    private:

        void do_resolve_recursive_refs(const uri& base, bool has_recursive_anchor, schema_registry<Json>& schemas) override 
        {
            if (if_schema_)
            {
                if_schema_->resolve_recursive_refs(base, has_recursive_anchor, schemas);
            }
            if (then_schema_)
            {
                then_schema_->resolve_recursive_refs(base, has_recursive_anchor, schemas);
            }
            if (else_schema_)
            {
                else_schema_->resolve_recursive_refs(base, has_recursive_anchor, schemas);
            }
        }
    };

    // enum_validator

    template <class Json>
    class enum_keyword : public schema_keyword_base<Json>
    {
        using keyword_validator_type = std::unique_ptr<keyword_validator<Json>>;

        Json value_;

    public:
        enum_keyword(const uri& path, const Json& sch)
            : schema_keyword_base<Json>(path), value_(sch)
        {
        }

        keyword_validator_type make_validator(const uri& base_uri, schema_registry<Json>& /*schemas*/) const final
        {
            return jsoncons::make_unique<enum_validator<Json>>(this->reference().resolve(base_uri), Json(value_));
        }

    private:

        void do_resolve_recursive_refs(const uri& /*base*/, bool /*has_recursive_anchor*/, schema_registry<Json>& /*schemas*/)         override 
        {
        }
    };

    // const_validator

    template <class Json>
    class const_keyword : public schema_keyword_base<Json>
    {        
        using keyword_validator_type = std::unique_ptr<keyword_validator<Json>>;

        Json value_;

    public:
        const_keyword(const uri& path, const Json& sch)
            : schema_keyword_base<Json>(path), value_(sch)
        {
        }

        keyword_validator_type make_validator(const uri& base_uri, schema_registry<Json>& /*schemas*/) const final
        {
            return jsoncons::make_unique<const_validator<Json>>(this->reference().resolve(base_uri), Json(value_));
        }

    private:

        void do_resolve_recursive_refs(const uri& /*base*/, bool /*has_recursive_anchor*/, schema_registry<Json>& /*schemas*/)         override 
        {
        }
    };

    template <class Json>
    class type_keyword : public schema_keyword_base<Json>
    {
        using keyword_validator_type = typename schema_keyword<Json>::keyword_validator_type;
        using keyword_type = typename schema<Json>::keyword_type;

        std::vector<keyword_type> type_mapping_;
        std::vector<std::string> expected_types_;

    public:
        type_keyword(const type_keyword&) = delete;
        type_keyword& operator=(const type_keyword&) = delete;
        type_keyword(type_keyword&&) = default;
        type_keyword& operator=(type_keyword&&) = default;

        type_keyword(const uri& reference,
            std::vector<keyword_type>&& type_mapping,
            std::vector<std::string>&& expected_types
 )
            : schema_keyword_base<Json>(std::move(reference)),
              type_mapping_(std::move(type_mapping)),
              expected_types_(std::move(expected_types))
        {
        }

        keyword_validator_type make_validator(const uri& base_uri, schema_registry<Json>& schemas) const final 
        {
            std::vector<keyword_validator_type> type_mapping;
            for (auto& item : type_mapping_)
            {
                if (item)
                {
                    type_mapping.emplace_back(item->make_validator(base_uri, schemas));
                }
                else
                {
                    type_mapping.emplace_back(keyword_validator_type{});
                }
            }

            return jsoncons::make_unique<type_validator<Json>>(this->reference().resolve(base_uri),
                std::move(type_mapping), 
                std::vector<std::string>(expected_types_));
        }

    private:

        void do_resolve_recursive_refs(const uri& base, bool has_recursive_anchor, schema_registry<Json>& schemas) override 
        {
            for (auto& item : type_mapping_)
            {
                if (item)
                {
                    item->resolve_recursive_refs(base, has_recursive_anchor, schemas);
                }
            }
        }

    };

} // namespace jsonschema
} // namespace jsoncons

#endif // JSONCONS_JSONSCHEMA_KEYWORDS_HPP

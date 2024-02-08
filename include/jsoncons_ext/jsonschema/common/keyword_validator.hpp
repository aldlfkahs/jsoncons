// Copyright 2013-2023 Daniel Parker
// Distributed under the Boost license, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// See https://github.com/danielaparker/jsoncons for latest version

#ifndef JSONCONS_JSONSCHEMA_COMMON_KEYWORD_VALIDATOR_HPP
#define JSONCONS_JSONSCHEMA_COMMON_KEYWORD_VALIDATOR_HPP

#include <jsoncons/config/jsoncons_config.hpp>
#include <jsoncons/uri.hpp>
#include <jsoncons/json.hpp>
#include <jsoncons_ext/jsonpointer/jsonpointer.hpp>
#include <jsoncons_ext/jsonschema/jsonschema_error.hpp>
#include <unordered_set>

namespace jsoncons {
namespace jsonschema {

    // Interface for validation error handlers
    class error_reporter
    {
        bool fail_early_;
        std::size_t error_count_;
    public:
        error_reporter(bool fail_early = false)
            : fail_early_(fail_early), error_count_(0)
        {
        }

        virtual ~error_reporter() = default;

        void error(const validation_output& o)
        {
            ++error_count_;
            do_error(o);
        }

        std::size_t error_count() const
        {
            return error_count_;
        }

        bool fail_early() const
        {
            return fail_early_;
        }

    private:
        virtual void do_error(const validation_output& /* e */) = 0;
    };


    template <class Json>
    class schema;

    template <class Json>
    class schema_registry
    {
    public:
        virtual ~schema_registry() = default;
        virtual schema<Json>* get_schema(const jsoncons::uri& uri) = 0;
    };

    template <class Json>
    class validator_base 
    {
    public:
        virtual ~validator_base() = default;

        virtual const uri& reference() const = 0;

        void validate(const Json& instance, 
            const jsonpointer::json_pointer& instance_location,
            std::unordered_set<std::string>& evaluated_properties, 
            error_reporter& reporter, 
            Json& patch) const 
        {
            do_validate(instance, instance_location, evaluated_properties, reporter, patch);
        }

    private:
        virtual void do_validate(const Json& instance, 
            const jsonpointer::json_pointer& instance_location,
            std::unordered_set<std::string>& evaluated_properties, 
            error_reporter& reporter, 
            Json& patch) const = 0;
    };

    template <class Json>
    class keyword_validator : public validator_base<Json> 
    {
    public:
        using keyword_validator_type = std::unique_ptr<keyword_validator<Json>>;
    };

    template <class Json>
    class keyword_validator_base : public keyword_validator<Json>
    {
        uri reference_;
    public:
        using keyword_validator_type = std::unique_ptr<keyword_validator<Json>>;

        keyword_validator_base(const uri& reference)
            : reference_(reference)
        {
        }

        keyword_validator_base(const keyword_validator_base&) = delete;
        keyword_validator_base(keyword_validator_base&&) = default;
        keyword_validator_base& operator=(const keyword_validator_base&) = delete;
        keyword_validator_base& operator=(keyword_validator_base&&) = default;

        const uri& reference() const override
        {
            return reference_;
        }
    };

    template <class Json>
    using uri_resolver = std::function<Json(const jsoncons::uri & /*id*/)>;

    template <class Json>
    class schema_validator : public validator_base<Json>
    {
    public:
        using schema_validator_type = typename std::unique_ptr<schema_validator<Json>>;
        using keyword_validator_type = typename std::unique_ptr<keyword_validator<Json>>;

    public:
        schema_validator()
        {}

        virtual jsoncons::optional<Json> get_default_value() const = 0;

        virtual bool is_recursive_anchor() const = 0;
    };

    template <class Json>
    class recursive_ref_validator : public keyword_validator<Json>
    {
        using keyword_validator_type = std::unique_ptr<keyword_validator<Json>>;
        using schema_validator_type = std::unique_ptr<schema_validator<Json>>;

        uri base_uri_;
        schema_validator_type referred_schema_;

    public:
        recursive_ref_validator(const uri& base_uri) : base_uri_(base_uri), referred_schema_(nullptr)
        {}

        recursive_ref_validator(const uri& base_uri, schema_validator_type&& target)
            : base_uri_(base_uri), referred_schema_(std::move(target)) {}

        uri get_base_uri() const
        {
            return base_uri_;
        }

        const uri& reference() const override
        {
            return referred_schema_ ? referred_schema_->reference() : base_uri_;
        }

    private:

        void do_validate(const Json& instance, 
            const jsonpointer::json_pointer& instance_location,
            std::unordered_set<std::string>& evaluated_properties, 
            error_reporter& reporter, 
            Json& patch) const override
        {
            //std::cout << "recursive_ref_validator.do_validate " << "keywordLocation: << " << this->reference().string() << ", instanceLocation:" << instance_location.to_string() << "\n";

            if (referred_schema_ == nullptr)
            {
                reporter.error(validation_output("", 
                                                 this->reference(), 
                                                 instance_location.to_uri_fragment(), 
                                                 "Unresolved schema reference " + this->reference().string()));
                return;
            }

            referred_schema_->validate(instance, instance_location, evaluated_properties, reporter, patch);
        }
    };

    template <class Json>
    class boolean_schema_validator : public schema_validator<Json>
    {
    public:
        using schema_validator_type = typename std::unique_ptr<schema_validator<Json>>;
        using keyword_validator_type = typename std::unique_ptr<keyword_validator<Json>>;

        uri reference_;
        bool value_;

    public:
        boolean_schema_validator(const uri& reference, bool value)
            : reference_(reference), value_(value)
        {
        }

        jsoncons::optional<Json> get_default_value() const override
        {
            return jsoncons::optional<Json>{};
        }

        const uri& reference() const override
        {
            return reference_;
        }

        bool is_recursive_anchor() const final
        {
            return false;
        }

    private:

        void do_validate(const Json&, 
            const jsonpointer::json_pointer& instance_location,
            std::unordered_set<std::string>&, 
            error_reporter& reporter, 
            Json&) const final
        {
            if (!value_)
            {
                reporter.error(validation_output("false", 
                    this->reference(), 
                    instance_location.to_uri_fragment(), 
                    "False schema always fails"));
            }
        }
    };

    template <class Json>
    class object_schema_validator : public schema_validator<Json>
    {
    public:
        using schema_validator_type = typename std::unique_ptr<schema_validator<Json>>;
        using keyword_validator_type = typename std::unique_ptr<keyword_validator<Json>>;

        uri reference_;
        std::vector<keyword_validator_type> validators_;
        Json default_value_;
        bool is_recursive_anchor_;

    public:
        object_schema_validator(const uri& reference, std::vector<keyword_validator_type>&& validators, Json&& default_value,
            bool is_recursive_anchor = false)
            : reference_(reference),
              validators_(std::move(validators)),
              default_value_(std::move(default_value)),
              is_recursive_anchor_(is_recursive_anchor)
        {
        }

        jsoncons::optional<Json> get_default_value() const override
        {
            return default_value_;
        }

        const uri& reference() const override
        {
            return reference_;
        }

        bool is_recursive_anchor() const final
        {
            return is_recursive_anchor_;
        }

    private:

        void do_validate(const Json& instance, 
            const jsonpointer::json_pointer& instance_location,
            std::unordered_set<std::string>& evaluated_properties, 
            error_reporter& reporter, 
            Json& patch) const final
        {
            std::unordered_set<std::string> local_evaluated_properties;

            for (auto& validator : validators_)
            {
                validator->validate(instance, instance_location, local_evaluated_properties, reporter, patch);
                if (reporter.error_count() > 0 && reporter.fail_early())
                {
                    return;
                }
            }

            for (auto&& name : local_evaluated_properties)
            {
                evaluated_properties.emplace(std::move(name));
            }
        }
    };

    // schema_keyword

    template <class Json>
    class schema_keyword 
    {
    public:
        using keyword_validator_type = std::unique_ptr<keyword_validator<Json>>;

        virtual ~schema_keyword() = default;

        virtual const uri& reference() const = 0;

        virtual keyword_validator_type make_validator(const uri& base_uri, schema_registry<Json>& schemas) const = 0;

        void resolve_recursive_refs(const uri& base, bool has_recursive_anchor, schema_registry<Json>& schemas)
        {
            do_resolve_recursive_refs(base, has_recursive_anchor, schemas);
        }
    private:
        virtual void do_resolve_recursive_refs(const uri& base, bool has_recursive_anchor, schema_registry<Json>& schemas) = 0;
    };

    template <class Json>
    class schema_keyword_base : public schema_keyword<Json>
    {
        uri reference_;
    public:
        using keyword_validator_type = std::unique_ptr<keyword_validator<Json>>;

        schema_keyword_base(const uri& reference)
            : reference_(reference)
        {
        }

        schema_keyword_base(const schema_keyword_base&) = delete;
        schema_keyword_base(schema_keyword_base&&) = default;
        schema_keyword_base& operator=(const schema_keyword_base&) = delete;
        schema_keyword_base& operator=(schema_keyword_base&&) = default;

        const uri& reference() const override
        {
            return reference_;
        }
    };

    template <class Json>
    class schema 
    {
    public:
        using schema_type = typename std::unique_ptr<schema<Json>>;
        using schema_validator_type = typename std::unique_ptr<schema_validator<Json>>;
        using keyword_type = typename std::unique_ptr<schema_keyword<Json>>;

    public:
        schema()
        {}

        virtual ~schema() = default;

        virtual const uri& reference() const = 0;

        void resolve_recursive_refs(const uri& base, bool has_recursive_anchor, schema_registry<Json>& schemas)
        {
            do_resolve_recursive_refs(base, has_recursive_anchor, schemas);
        }

        virtual jsoncons::optional<Json> get_default_value() const = 0;

        virtual bool is_recursive_anchor() const = 0;

        virtual schema_validator_type make_validator(const uri& base_uri, schema_registry<Json>& schemas) const = 0;

    private:
        virtual void do_resolve_recursive_refs(const uri& base, bool has_recursive_anchor, schema_registry<Json>& schemas) = 0;
    };

    template <class Json>
    class boolean_schema : public schema<Json>
    {
    public:
        using schema_type = typename std::unique_ptr<schema<Json>>;
        using schema_validator_type = typename std::unique_ptr<schema_validator<Json>>;
        using keyword_type = typename std::unique_ptr<schema_keyword<Json>>;

        uri reference_;
        bool value_;

    public:
        boolean_schema(const uri& reference, bool value)
            : reference_(reference), value_(value)
        {
        }

        jsoncons::optional<Json> get_default_value() const override
        {
            return jsoncons::optional<Json>{};
        }

        const uri& reference() const override
        {
            return reference_;
        }

        bool is_recursive_anchor() const final
        {
            return false;
        }

        schema_validator_type make_validator(const uri& base_uri, schema_registry<Json>& /*schemas*/) const final
        {
            return jsoncons::make_unique<boolean_schema_validator<Json>>(reference_.resolve(base_uri), value_);
        }

    private:

        void do_resolve_recursive_refs(const uri& /*base*/, bool /*has_recursive_anchor*/, schema_registry<Json>& /*schemas*/) override
        {
        }
    };

    template <class Json>
    class object_schema : public schema<Json>
    {
    public:
        using schema_type = typename std::unique_ptr<schema<Json>>;
        using schema_validator_type = typename std::unique_ptr<schema_validator<Json>>;
        using keyword_validator_type = std::unique_ptr<keyword_validator<Json>>;
        using keyword_type = typename std::unique_ptr<schema_keyword<Json>>;

        uri reference_;
        std::vector<keyword_type> keywords_;
        Json default_value_;
        bool is_recursive_anchor_;

    public:
        object_schema(const uri& reference, std::vector<keyword_type>&& validators, Json&& default_value,
            bool is_recursive_anchor = false)
            : reference_(reference),
              keywords_(std::move(validators)),
              default_value_(std::move(default_value)),
              is_recursive_anchor_(is_recursive_anchor)
        {
        }

        jsoncons::optional<Json> get_default_value() const override
        {
            return default_value_;
        }

        const uri& reference() const override
        {
            return reference_;
        }

        bool is_recursive_anchor() const final
        {
            return is_recursive_anchor_;
        }

        schema_validator_type make_validator(const uri& base_uri, schema_registry<Json>& schemas) const final
        {
            std::vector<keyword_validator_type> validators;
            for (auto& keyword : keywords_)
            {
                validators.push_back(keyword->make_validator(base_uri, schemas));
            }

            return jsoncons::make_unique<object_schema_validator<Json>>(reference_.resolve(base_uri), std::move(validators), Json(default_value_), is_recursive_anchor_);
        }

    private:
        void do_resolve_recursive_refs(const uri& base, bool has_recursive_anchor, schema_registry<Json>& schemas) override
        {
            if (has_recursive_anchor)
            {
                for (auto& keyword : keywords_)
                {
                    keyword->resolve_recursive_refs(base, has_recursive_anchor, schemas);
                }
            }
            else if (is_recursive_anchor_)
            {
                for (auto& keyword : keywords_)
                {
                    keyword->resolve_recursive_refs(reference(), is_recursive_anchor_, schemas);
                }
            }
            else
            {
                for (auto& keyword : keywords_)
                {
                    keyword->resolve_recursive_refs(reference(), is_recursive_anchor_, schemas);
                }
            }
            //std::cout << "object_schema location::do_resolve_recursive_refs: " << reference().string() << ", " << is_recursive_anchor_
            //         << "\n  base: " << base.string() << ", has_recursive_anchor: " << has_recursive_anchor << "\n\n";
        }
    };

} // namespace jsonschema
} // namespace jsoncons

#endif // JSONCONS_JSONSCHEMA_KEYWORD_VALIDATOR_HPP

// 
// Copyright 2022 Clemens Cords
// Created on 07.02.22 by clem (mail@clemens-cords.com)
//

#include <include/type.hpp>
#include <include/symbol.hpp>

namespace jluna
{
    Type::Type(jl_datatype_t* value)
        : Proxy((jl_value_t*) value, value->name->name)
    {}

    Type::Type(Any* value, std::shared_ptr<ProxyValue>& owner, jl_sym_t* symbol)
        : Proxy(value, owner, symbol)
    {}

    Type::operator _jl_datatype_t*()
    {
        return get();
    }

    jl_datatype_t* Type::get() const
    {
        return (jl_datatype_t*) Proxy::operator const jl_value_t*();
    }

    Type Type::get_super_type() const
    {
        return Type(get()->super);
    }

    Symbol Type::get_symbol() const
    {
        return get()->name->name;
    }

    size_t Type::get_n_fields() const
    {
        static jl_function_t* get_n_fields = jl_find_function("jluna", "get_n_fields");
        return unbox<size_t>(jluna::safe_call(get_n_fields, get()));
    }

    std::vector<std::pair<Symbol, Type>> Type::get_fields() const
    {
        static jl_function_t* get_fields = jl_find_function("jluna", "get_fields");
        return unbox<std::vector<std::pair<Symbol, Type>>>(jluna::safe_call(get_fields, get()));
    }

    std::vector<std::pair<Symbol, Type>> Type::get_parameters() const
    {
        static jl_function_t* get_parameters = jl_find_function("jluna", "get_parameters");
        return unbox<std::vector<std::pair<Symbol, Type>>>(jluna::safe_call(get_parameters, get()));
    }

    size_t Type::get_n_parameters() const
    {
        static jl_function_t* get_n_fields = jl_find_function("jluna", "get_n_parameters");
        return unbox<size_t>(jluna::safe_call(get_n_fields, get()));
    }

    Any * Type::get_singleton_instance() const
    {
        return get()->instance;
    }

    bool Type::is_subtype_of(const Type& other) const
    {
        return jl_subtype((jl_value_t*) get(), (jl_value_t*) other.get());
    }

    bool Type::operator<(const Type& other) const
    {
        return this->is_subtype_of(other);
    }

    bool Type::is_supertype_of(const Type& other) const
    {
        return other.is_subtype_of(*this);
    }

    bool Type::operator>(const Type& other) const
    {
        return this->is_supertype_of(other);
    }

    bool Type::is_same_as(const Type& other) const
    {
        return jl_is_identical((jl_value_t*) get(), (jl_value_t*) other.get());
    }

    bool Type::operator==(const Type& other) const
    {
        return this->is_same_as(other);
    }

    bool Type::operator!=(const Type& other) const
    {
        return not this->is_same_as(other);
    }

    bool Type::is_primitive() const
    {
        return jl_is_primitivetype(get());
    }

    bool Type::is_struct_type() const
    {
        return jl_is_structtype(get());
    }

    bool Type::is_isbits() const
    {
        return jl_isbits(get());
    }

    bool Type::is_singleton() const
    {
        return get()->instance != nullptr;
    }

    bool Type::is_abstract_type() const
    {
        return jl_is_abstracttype(get());
    }

    bool Type::is_abstract_ref_type() const
    {
        return jl_is_abstract_ref_type((jl_value_t*) get());
    }

    bool Type::is_array_type() const
    {
        return jl_is_array_type(get());
    }

    bool Type::is_opaque_closure_type() const
    {
        return jl_is_opaque_closure_type(get());
    }

    bool Type::is_tuple_type() const
    {
        return jl_is_tuple_type(get());
    }

    bool Type::is_named_tuple_type() const
    {
        return jl_is_namedtuple_type(get());
    }

    bool Type::is_type_type() const
    {
        return jl_is_type_type((jl_value_t*) get());
    }

    bool Type::is_union_type() const
    {
        return jl_is_uniontype(get());
    }

    bool Type::is_unionall_type() const
    {
        return jl_is_unionall(get());
    }

    bool Type::is_mutable() const
    {
        return jl_is_mutable_datatype(get());
    }



}


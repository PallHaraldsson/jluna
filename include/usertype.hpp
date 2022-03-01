// 
// Copyright 2022 Clemens Cords
// Created on 22.02.22 by clem (mail@clemens-cords.com)
//

#pragma once

#include <julia.h>

#include <include/type.hpp>
#include <include/proxy.hpp>

namespace jluna
{
    /// @brief customizable wrapper for non-julia type T
    /// @note for information on how to use this class, visit https://github.com/Clemapfel/jluna/blob/master/docs/manual.md#usertypes
    template<typename T>
    class Usertype
    {
        static inline std::function<void(T&, Any*)> noop_set = [](T&, Any* ) {return;};

        public:
            /// @brief original type
            using original_type = T;

            /// @brief enable this type by giving it a name
            /// @param name: julia-side name
            static void enable(const std::string& name);

            /// @brief get julia-side name
            /// @returns name
            static std::string get_name();

            /// @brief set mutability, no by default
            /// @param bool
            /// @note this function will throw if called after implement()
            static void set_mutable(bool);

            /// @brief get mutability
            /// @returns bool
            static bool is_mutable();

            /// @brief add field
            /// @param name: julia-side name of field
            /// @param type: type of symbol. User the other overload if the type is a typevar, such as "P" (where P is a parameter)
            /// @param box_get: lambda with signature (T&) -> Any*
            /// @param unbox_set: lambda with signature (T&, Any*)
            /// @note this function will throw if called after implement()
            static void add_field(
                const std::string& name,
                const Type& type,
                std::function<Any*(T&)> box_get,
                std::function<void(T&, Any*)> unbox_set = noop_set
            );

            /// @brief push to state and eval, cannot be extended afterwards
            /// @param module: module the type will be set in
            /// @returns julia-side type
            static Type implement(Module module = Main);

            /// @brief is already implemented
            /// @brief true if implement was called, false otherwise
            static bool is_implemented();

            /// @brief box interface
            static Any* box(T&);

            /// @brief unbox interface
            static T unbox(Any*);

        private:
            static inline Proxy _template = Proxy(jl_nothing);

            static inline bool _implemented = false;
            static inline Any* _implemented_type = nullptr;

            static inline std::map<std::string, std::tuple<
                Type,                           // field type
                std::function<Any*(T&)>,        // getter
                std::function<void(T&, Any*)>   // setter
            >> _mapping = {};
    };

    /// @brief exception thrown when usertype is used before being implemented
    template<typename T>
    struct UsertypeNotFullyInitializedException : public std::exception
    {
        public:
            /// @brief ctor
            UsertypeNotFullyInitializedException();

            /// @brief what
            /// @returns message
            const char* what() const noexcept override final;

        private:
            std::string _msg;
    };

    /// @brief exception thrown implementation is called twice
    template<typename T>
    struct UsertypeAlreadyImplementedException : public std::exception
    {
        public:
            /// @brief ctor
            UsertypeAlreadyImplementedException();

            /// @brief what
            /// @returns message
            const char* what() const noexcept override final;

        private:
            std::string _msg;
    };
}

#include ".src/usertype.inl"
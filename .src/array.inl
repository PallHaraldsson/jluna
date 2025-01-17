// 
// Copyright 2022 Clemens Cords
// Created on 12.01.22 by clem (mail@clemens-cords.com)
//

namespace jluna
{
    namespace detail
    {
        template<typename Value_t, size_t N>
        struct to_julia_type_aux<Array<Value_t, N>>
        {
            static inline const std::string type_name = "Array{" + to_julia_type_aux<Value_t>::type_name + ", " + std::to_string(N) + "}";
        };
    }

    template<Boxable V, size_t R>
    Array<V, R>::Array(Any* value, jl_sym_t* symbol)
        : Proxy(value, symbol)
    {
        static jl_datatype_t* array_t = (jl_datatype_t*) jl_eval_string("return Base.Array");
        jl_assert_type(value, array_t);
    }

    template<Boxable V, size_t R>
    Array<V, R>::Array(Proxy* proxy)
        : Proxy(*proxy)
    {
        static jl_datatype_t* array_t = (jl_datatype_t*) jl_eval_string("return Base.Array");
        jl_assert_type(proxy->operator Any*(), array_t);
    }

    template<Boxable T, size_t Rank>
    size_t Array<T, Rank>::get_dimension(int index) const
    {
        static jl_function_t* size = jl_get_function(jl_base_module, "size");

        jl_gc_pause;
        auto out = jl_unbox_uint64(jl_call2(size, _content->value(), jl_box_int32(index + 1)));
        jl_gc_unpause;

        return out;
    }

    template<Boxable T, size_t Rank>
    void Array<T, Rank>::throw_if_index_out_of_range(int index, size_t dimension) const
    {
        if (index < 0)
        {
            std::stringstream str;
            str << "negative index " << index << ", only indices >= 0 are permitted" << std::endl;
            throw std::out_of_range(str.str().c_str());
        }

        size_t dim = get_dimension(dimension);

        if (index >= dim)
        {
            std::string dim_id;

            if (dimension == 0)
                dim_id = "1st dimension";
            else if (dimension == 1)
                dim_id = "2nd dimension";
            else if (dimension == 3)
                dim_id = "3rd dimension";
            else if (dimension < 11)
                dim_id = std::to_string(dimension) + "th dimension";
            else
                dim_id = "dimension " + std::to_string(dimension);

            std::stringstream str;
            str << "0-based index " << index << " out of range for array of length " << dim << " along " << dim_id << std::endl;
            throw std::out_of_range(str.str().c_str());
        }
    } //°

    template<Boxable V, size_t R>
    auto Array<V, R>::operator[](size_t i)
    {
        if (i >= get_n_elements())
        {
            std::stringstream str;
            str << "0-based index " << i << " out of range for array of length " << get_n_elements() << std::endl;
            throw std::out_of_range(str.str().c_str());
        }

        return Iterator(i, this);
    }

    template<Boxable V, size_t R>
    Vector<V> Array<V, R>::operator[](const std::vector<size_t>& range) const
    {
        static jl_function_t* getindex = jl_get_function(jl_base_module, "getindex");
        static jl_function_t* vector = jl_get_function(jl_base_module, "Vector");

        jl_gc_pause;

        jl_array_t* out = (jl_array_t*) jl_call2(vector, jl_undef_initializer(), jl_box_uint64(range.size()));

        for (size_t i = 0; i < range.size(); ++i)
        {
            if (range.at(i)+1 > get_n_elements())
            {
                std::stringstream str;
                str << "[C++][EXCEPTION] 0-based index " << range.at(i) << " out of range for array of length " << get_n_elements() << std::endl;
                throw std::out_of_range(str.str());
            }

            jl_arrayset(out, jl_box_uint64(range.at(i)+1), i);
        }

        return Vector<V>(jl_call2(getindex, _content->value(), (Any*) out));

        jl_gc_unpause;
    }

    template<Boxable V, size_t R>
    Vector<V> Array<V, R>::operator[](const GeneratorExpression& gen) const
    {
        std::vector<size_t> vec;
        vec.reserve(gen.size());
        for (auto it : gen)
            vec.push_back(unbox<size_t>(it));

        return operator[](vec);
    }

    template<Boxable V, size_t R>
    template<Boxable T>
    Vector<V> Array<V, R>::operator[](std::initializer_list<T>&& list) const
    {
        // cast
        std::vector<size_t> index;
        index.reserve(list.size());
        for (auto& e : list)
            index.push_back(e);

        return operator[](index);
    }

    template<Boxable V, size_t R>
    template<Unboxable T>
    T Array<V, R>::operator[](size_t i) const
    {
        if (i >= get_n_elements())
        {
            std::stringstream str;
            str << "0-based index " << i << " out of range for array of length " << get_n_elements() << std::endl;
            throw std::out_of_range(str.str().c_str());
        }

        return unbox<T>(jl_arrayref((jl_array_t*) _content->value(), i));
    }

    template<Boxable V, size_t R>
    template<Unboxable T, typename... Args, std::enable_if_t<sizeof...(Args) == R and (std::is_integral_v<Args> and ...), bool>>
    T Array<V, R>::at(Args... in) const
    {
        {
            size_t i = 0;
            (throw_if_index_out_of_range(in, i++), ...);
        }

        std::array<size_t, R> indices = {size_t(in)...};
        size_t index = 0;
        size_t mul = 1;

        for (size_t i = 0; i < R; ++i)
        {
            index += (indices.at(i)) * mul;
            size_t dim = get_dimension(i);
            mul *= dim;
        }

        return operator[]<T>(index);
    }

    template<Boxable V, size_t R>
    template<typename... Args, std::enable_if_t<sizeof...(Args) == R and (std::is_integral_v<Args> and ...), bool>>
    auto Array<V, R>::at(Args... in)
    {
        {
            size_t i = 0;
            (throw_if_index_out_of_range(in, i++), ...);
        }

        std::array<size_t, R> indices = {size_t(in)...};
        size_t index = 0;
        size_t mul = 1;

        for (size_t i = 0; i < R; ++i)
        {
            index += (indices.at(i)) * mul;
            size_t dim = get_dimension(i);
            mul *= dim;
        }

        return operator[](index);
    }

    template<Boxable V, size_t R>
    template<Boxable T>
    void Array<V, R>::set(size_t i, T value)
    {
        jl_arrayset((jl_array_t*) _content->value(), box<T>(value), i);
    } //°

    template<Boxable V, size_t R>
    auto Array<V, R>::front()
    {
        return operator[](0);
    }

    template<Boxable V, size_t R>
    auto Array<V, R>::begin()
    {
        return Iterator(0, this);
    }

    template<Boxable V, size_t R>
    auto Array<V, R>::begin() const
    {
        return ConstIterator(0, const_cast<Array<V, R>*>(this));
    }

    template<Boxable V, size_t R>
    auto Array<V, R>::end()
    {
        return Iterator(get_n_elements(), this);
    }

    template<Boxable V, size_t R>
    auto Array<V, R>::end() const
    {
        return ConstIterator(get_n_elements(), const_cast<Array<V, R>*>(this));
    }

    template<Boxable V, size_t R>
    template<Unboxable T>
    T Array<V, R>::front() const
    {
        return operator[](0);
    }

    template<Boxable V, size_t R>
    auto Array<V, R>::back()
    {
        return operator[](get_n_elements() - 1);
    }

    template<Boxable V, size_t R>
    template<Unboxable T>
    T Array<V, R>::back() const
    {
        return operator[]<T>(get_n_elements() - 1);
    } //°

    template<Boxable V, size_t R>
    size_t Array<V, R>::get_n_elements() const
    {
        return reinterpret_cast<const jl_array_t*>(this->operator const Any*())->length;
    } //°

    template<Boxable V, size_t R>
    bool Array<V, R>::empty() const
    {
        return reinterpret_cast<const jl_array_t*>(this->operator const Any*())->length == 0;
    } //°

    template<Boxable V, size_t R>
    void* Array<V, R>::data()
    {
        return reinterpret_cast<jl_array_t*>(this->operator Any*())->data;
    } //°

    // ###

    template<Boxable V>
    Vector<V>::Vector()
        : Array<V, 1>(box<std::vector<V>>(std::vector<V>()), nullptr)
    {}

    template<Boxable V>
    Vector<V>::Vector(const std::vector<V>& vec)
        : Array<V, 1>(box<std::vector<V>>(vec), nullptr)
    {}

    template<Boxable V>
    Vector<V>::Vector(Proxy* owner)
        : Array<V, 1>(owner)
    {
        static jl_datatype_t* vector_t = (jl_datatype_t*) jl_eval_string("return Vector");
        jl_assert_type(owner->operator Any*(), vector_t);
    }

    template<Boxable V>
    Vector<V>::Vector(jl_value_t* value, jl_sym_t* symbol)
        : Array<V, 1>(value, symbol)
    {
        static jl_datatype_t* vector_t = (jl_datatype_t*) jl_eval_string("return Vector");
        jl_assert_type(value, vector_t);
    }

    template<Boxable V>
    Vector<V>::Vector(const GeneratorExpression& gen)
        : Vector([gen = std::ref(gen)]() -> std::vector<V> {

            std::vector<V> vec;
            vec.reserve(gen.get().size());
            for (auto it : gen.get())
                vec.push_back(unbox<V>(it));

            return vec;
        }())
    {}

    template<Boxable V>
    void Vector<V>::insert(size_t pos, V value)
    {
        static jl_value_t* insert = jl_get_function(jl_base_module, "insert!");

        jl_gc_pause;
        jl_call3(insert, _content->value(), jl_box_uint64(pos + 1), box(value));
        jl_gc_unpause;
        forward_last_exception();
    }

    template<Boxable V>
    void Vector<V>::erase(size_t pos)
    {
        static jl_value_t* deleteat = jl_get_function(jl_base_module, "deleteat!");

        jl_gc_pause;
        jl_call2(deleteat, _content->value(), jl_box_uint64(pos + 1));
        jl_gc_unpause;
        forward_last_exception();
    }

    template<Boxable V>
    template<Boxable T>
    void Vector<V>::push_front(T value)
    {
        static jl_value_t* pushfirst = jl_get_function(jl_base_module, "pushfirst!");

        jl_gc_pause;
        jl_call2(pushfirst, _content->value(), box(value));
        jl_gc_unpause;
        forward_last_exception();
    }

    template<Boxable V>
    template<Boxable T>
    void Vector<V>::push_back(T value)
    {
        static jl_value_t* push = jl_get_function(jl_base_module, "push!");

        jl_gc_pause;
        jl_call2(push, _content->value(), box(value));
        jl_gc_unpause;
        forward_last_exception();
    }
}
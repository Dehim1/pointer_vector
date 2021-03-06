#pragma once
#include <cstdint>
#include <algorithm>

//Vector type with manual memory management that can be converted to and from a void*.
template <class T>
class pointer_vector
{
	uint8_t* data_;

	void reallocate_copy_destroy(size_t n_bytes_to_allocate)
	{
		//reallocate
		uint8_t* new_data = reinterpret_cast<uint8_t*>(new uint8_t[n_bytes_to_allocate]);
		//copy
		*reinterpret_cast<uint8_t**>(new_data) = new_data + n_bytes_to_allocate;
		*reinterpret_cast<uint8_t**>(new_data + sizeof(uint8_t*)) = new_data + (*reinterpret_cast<uint8_t**>(data_ + sizeof(uint8_t*)) - data_);
		std::copy(data_ + 2 * sizeof(uint8_t*), *reinterpret_cast<uint8_t**>(data_ + sizeof(uint8_t*)), new_data + 2 * sizeof(uint8_t*));
		//destroy
		delete[] data_;
		data_ = new_data;
	}

	size_t allocated_size() const
	{
		return (*reinterpret_cast<uint8_t**>(data_) - data_ - 2 * sizeof(uint8_t*)) / sizeof(T);
	}

public:
	void reserve(size_t size)
	{
		if (allocated_size() < size)
			reallocate_copy_destroy(2 * sizeof(uint8_t*) + size * sizeof(T));
	}

	void resize(size_t size)
	{
		reserve(size);
		auto iter = end();
		*reinterpret_cast<uint8_t**>(data_ + sizeof(uint8_t*)) = (data_ + 2 * sizeof(uint8_t*) + size * sizeof(T));
		for (; iter < end(); ++iter)
			new (iter) T();
		for (--iter; iter >= end(); --iter)
			iter->~T();
	}

	template<class... _Args>
	void resize(size_t size, _Args&&... args)
	{
		reserve(size);
		auto iter = end();
		*reinterpret_cast<uint8_t**>(data_ + sizeof(uint8_t*)) = (data_ + 2 * sizeof(uint8_t*) + size * sizeof(T));
		for (; iter < end(); ++iter)
			new (iter) T(args...);
		for (--iter; iter >= end(); --iter)
			iter->~T();
	}

	template<class... _Args>
	void push_back(_Args&&... args)
	{
		if (end() == *reinterpret_cast<T**>(data_))
			reallocate_copy_destroy(2 * size_t(*reinterpret_cast<uint8_t**>(data_) - data_ - sizeof(uint8_t*)));
		new (*reinterpret_cast<T**>(data_ + sizeof(uint8_t*))) T(args...);
		++*reinterpret_cast<T**>(data_ + sizeof(uint8_t*));
	}

	void pop_back()
	{
		--*reinterpret_cast<T**>(data_ + sizeof(uint8_t*));
		end()->~T();
	}

	void clear()
	{
		for (auto& ref : *this)
			ref.~T();
		*reinterpret_cast<uint8_t**>(data_ + sizeof(uint8_t*)) = data_ + 2 * sizeof(uint8_t*);
	}

	size_t size() const
	{
		return end() - begin();
	}

	bool empty() const
	{
		return *reinterpret_cast<uint8_t**>(data_ + sizeof(uint8_t*)) == data_ + 2 * sizeof(uint8_t*);
	}

	T* begin()
	{
		return reinterpret_cast<T*>(data_ + 2 * sizeof(uint8_t*));
	}

	T* end()
	{
		return *reinterpret_cast<T**>(data_ + sizeof(uint8_t*));
	}

	T& back()
	{
		return *(end() - 1);
	}

	T& operator[](size_t idx)
	{
		return *(begin() + idx);
	}

	pointer_vector() {}
	~pointer_vector() {}

	template<class T2>
	pointer_vector(const pointer_vector<T2>& rhs)
	{
		this->initialize_this(rhs.size());
		auto& to_iter = *reinterpret_cast<T**>(data_ + sizeof(uint8_t*));
		for (auto& ref : rhs)
			new (to_iter++) T(ref);
	}

	template<class T2>
	pointer_vector(const pointer_vector<T2>& rhs, float growth_factor)
	{
		this->initialize_this(size_t(std::ceil(rhs.size()*growth_factor)));
		auto& to_iter = *reinterpret_cast<T**>(data_ + sizeof(uint8_t*));
		for (auto& ref : rhs)
			new (to_iter++) T(ref);
	}

	pointer_vector(void* pv)
	{
		data_ = reinterpret_cast<uint8_t*>(pv);
	}

	operator void*()
	{
		return reinterpret_cast<void*>(data_);
	}

	void initialize_this()
	{
		data_ = new uint8_t[2 * sizeof(uint8_t*) + sizeof(T)];
		*reinterpret_cast<uint8_t**>(data_) = data_ + 2 * sizeof(uint8_t*) + sizeof(T); //End of allocated storage pointer
		*reinterpret_cast<uint8_t**>(data_ + sizeof(uint8_t*)) = data_ + 2 * sizeof(uint8_t*); //End of used storage pointer
	}

	void initialize_this(size_t size)
	{
		data_ = new uint8_t[2 * sizeof(uint8_t*) + (++size) * sizeof(T)];
		*reinterpret_cast<uint8_t**>(data_) = data_ + 2 * sizeof(uint8_t*) + size * sizeof(T); //End of allocated storage pointer
		*reinterpret_cast<uint8_t**>(data_ + sizeof(uint8_t*)) = data_ + 2 * sizeof(uint8_t*); //End of used storage pointer
	}

	void delete_this()
	{
		for (auto& ref : *this)
			ref.~T();
		delete[] data_;
	}

	//const

	const T* begin() const
	{
		return reinterpret_cast<const T*>(data_ + 2 * sizeof(uint8_t*));
	}

	const T* end() const
	{
		return *reinterpret_cast<const T**>(data_ + sizeof(uint8_t*));
	}

	const T& back() const
	{
		return *(end() - 1);
	}

	const T& operator[](size_t idx) const
	{
		return *(begin() + idx);
	}

	operator const void*() const
	{
		return reinterpret_cast<const void*>(data_);
	}
};

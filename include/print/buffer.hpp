#pragma once

#include <array.hpp>
#include <storage.hpp>
#include <list.hpp>
#include <number.hpp>

#include <posix/io.hpp>

namespace print {

template<nuint BufferSize>
class buffer {
	list<array<uint1a, BufferSize>> buff_{};
	handle<posix::file> handle_;

public:

	buffer(handle<posix::file> handle) : handle_{ handle } {}

	~buffer() {
		flush();
	}

	buffer& flush() {
		handle_->write_from(buff_);
		buff_.clear();
		return *this;
	}

	template<typename... Types>
	requires (sizeof...(Types) > 1)
	buffer& operator() (Types&&... values) {
		((*this)(forward<Types>(values)), ...);
		return *this;
	}

	template<basic_range Range>
	buffer& operator () (Range&& r) {
		auto iterator = range_iterator(r);
		auto elements_to_put = range_size(r);

		while(elements_to_put > buff_.available()) {
			auto available = buff_.available();
			buff_.put_back_copied_elements_of(
				iterator_and_sentinel {
					iterator, iterator + available
				}.as_range()
			);
			flush();
			iterator += available;
			elements_to_put -= available;
		}

		buff_.put_back_copied_elements_of(
			iterator_and_sentinel {
				iterator, iterator + elements_to_put
			}.as_range()
		);
		return *this;
	}

	template<nuint Size>
	buffer& operator () (const char (&str_array)[Size]) {
		(*this) (span{ str_array, Size - 1 });
		return *this;
	};

	buffer& number(unsigned_integer auto number, number_base base = 10) {
		nuint count = 0;
		::number{ number }.for_each_digit(base, [&](nuint) { ++count; });
		char digits[count];
		count = 0;
		::number{ number }.for_each_digit(base, [&](nuint digit) {
			digits[count++] = (char)digit + (digit <= 9 ? '0' : 'A' - 10);
		});
		(*this)(span{ digits, count });
		return *this;
	}

	buffer& operator () (const char ch) {
		(*this)(array{ ch });
		return *this;
	}

	buffer& operator () (unsigned_integer auto number) {
		this->number(number, 10);
		return *this;
	}

	buffer& hex(unsigned_integer auto number) {
		(*this)("0x");
		this->number(number, 16);
		return *this;
	}

	buffer& bin(unsigned_integer auto number) {
		(*this)("0b");
		this->number(number, 2);
		return *this;
	}

	buffer& operator () (signed_integer auto number) {
		if(number < 0) {
			(*this)("-");
			(*this)((nuint)-number);
		}
		else {
			(*this)((nuint)number);
		}
		return *this;
	};

	buffer& operator () (float number) {
		(*this)((int32)number);
		(*this)(".");
		float fract =
			::number{        number }.absolute() -
			::number{ (int32)number }.absolute();
		(*this)((nuint) (fract * 10000.0F));
		return *this;
	}

	buffer& operator () (double number) {
		(*this)((int64)number);
		(*this)(".");
		double fract =
			::number{        number }.absolute() -
			::number{ (int64)number }.absolute();
		(*this)((nuint) (fract * 100000000.0));
		return *this;
	}

	buffer& operator () (bool b) {
		b ? (*this)("true") : (*this)("false");
		return *this;
	}


}; // class buffer

} // namespace print
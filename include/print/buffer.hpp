#pragma once

#include <array.hpp>
#include <storage.hpp>
#include <list.hpp>
#include <number.hpp>

#include <posix/io.hpp>

namespace print {

template<nuint BufferSize>
class buffer {
	list<array<storage<char>, BufferSize>> buff_{};
	handle<posix::file> handle_;

public:

	buffer(handle<posix::file> handle) : handle_{ handle } {}

	void flush() {
		handle_->write_from(buff_);
		buff_.clear();
	}

	~buffer() {
		flush();
	}

	template<typename... Types>
	requires (sizeof...(Types) > 1)
	void operator() (Types&&... values) {
		((*this)(forward<Types>(values)), ...);
	}

	template<basic_range Range>
	void operator () (Range&& r) {
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
	}

	template<nuint Size>
	void operator () (const char (&str_array)[Size]) {
		(*this)(c_string{ str_array });
	};

	void number(unsigned_integer auto number, number_base base = 10) {
		nuint count = 0;
		::number{ number}.for_each_digit(base, [&](nuint) { ++count; });
		char digits[count];
		count = 0;
		::number{ number }.for_each_digit(base, [&](nuint digit) {
			digits[count++] = (char)digit + (digit <= 9 ? '0' : 'A' - 10);
		});
		(*this)(span{ digits, count });
	}

	void operator () (unsigned_integer auto number) {
		this->number(number, 10);
	}

	void hex(unsigned_integer auto number) {
		(*this)("0x");
		this->number(number, 16);
	}

	void operator () (signed_integer auto number) {
		if(number < 0) {
			(*this)("-");
			(*this)((nuint)-number);
		}
		else {
			(*this)((nuint)number);
		}
	};

	void operator () (float number) {
		(*this)((int32)number);
		(*this)(".");
		float fract =
			::number{        number }.absolute() -
			::number{ (int32)number }.absolute();
		(*this)((nuint) (fract * 10000.0F));
	}

	void operator () (double number) {
		(*this)((int64)number);
		(*this)(".");
		double fract =
			::number{        number }.absolute() -
			::number{ (int64)number }.absolute();
		(*this)((nuint) (fract * 100000000.0));
	}

	void operator () (bool b) {
		b ? (*this)("true") : (*this)("false");
	}


}; // class buffer

} // namespace print
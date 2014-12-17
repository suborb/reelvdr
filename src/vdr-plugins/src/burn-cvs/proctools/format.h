#ifndef VGS_PROCTOOLS_FORMATTER_H
#define VGS_PROCTOOLS_FORMATTER_H

#include <string>
#include <sstream>
#include <iostream>
#include <iomanip>

namespace proctools
{

	class format
	{
		friend const std::string& str( const format& value );

	public:
		template<typename T>
		struct prec_t
		{
			prec_t(const T& val, int width): m_val(val), m_width(width) {}
			const T& m_val;
			int m_width;
		};

		template<typename T>
		static prec_t<T> prec(const T& val, int width);

		template<typename T>
		struct width_t
		{
			width_t(const T& val, int width): m_val(val), m_width(width) {}
			const T& m_val;
			int m_width;
		};

		template<typename T>
		static width_t<T> width(const T& val, int width);

		template<typename T, typename Elem>
		struct fill_t
		{
			fill_t(const T& val, Elem ch): m_val(val), m_ch(ch) {}
			const T& m_val;
			Elem m_ch;
		};

		template<typename T, typename Elem>
		static fill_t<T,Elem> fill(const T& val, Elem ch);

		template<typename T>
		struct base_t
		{
			base_t(const T& val, int base): m_val(val), m_base(base) {}
			const T& m_val;
			int m_base;
		};

		template<typename T>
		static base_t<T> base(const T& val, int base);

		format(const std::string& format): m_text(format), m_index(0) {}

		template<typename T>
		format& operator%(const T& object);

		operator const std::string&() const { return m_text; }

	private:
		std::string m_text;
		int m_index;
	};

	template<typename T>
	std::ostream& operator<<(std::ostream& s, const format::prec_t<T>& obj)
	{
		return s << std::setprecision(obj.m_width) << obj.m_val;
	}

	template<typename T>
	format::prec_t<T> format::prec(const T& val, int width)
	{
		return prec_t<T>(val, width);
	}

	template<typename T>
	std::ostream& operator<<(std::ostream& s, const format::width_t<T>& obj)
	{
		return s << std::setw(obj.m_width) << obj.m_val;
	}

	template<typename T>
	format::width_t<T> format::width(const T& val, int width)
	{
		return width_t<T>(val, width);
	}

	template<typename T, typename Elem>
	std::ostream& operator<<(std::ostream& s, const format::fill_t<T, Elem>& obj)
	{
		return s << std::setfill(obj.m_ch) << obj.m_val;
	}

	template<typename T, typename Elem>
	format::fill_t<T, Elem> format::fill(const T& val, Elem ch)
	{
		return fill_t<T, Elem>(val, ch);
	}

	template<typename T>
	std::ostream& operator<<(std::ostream& s, const format::base_t<T>& obj)
	{
		return s << std::setbase(obj.m_base) << obj.m_val;
	}

	template<typename T>
	format::base_t<T> format::base(const T& val, int base)
	{
		return base_t<T>(val, base);
	}

	template<typename T>
	format& format::operator%(const T& object)
	{
		std::stringstream stream, item;
		stream << object;
		item << "{" << m_index << "}";

		std::string::size_type pos;
		if ((pos = m_text.find(item.str())) != std::string::npos)
			m_text.replace(pos, item.str().length(), stream.str());

		++m_index;
		return *this;
	}

	inline
	const std::string& str( const format& value )
	{
		return value.m_text;
	}

};

#endif // VGS_PROCTOOLS_FORMATTER_H

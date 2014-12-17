#ifndef VGS_PROCTOOLS_PROPERTY_H
#define VGS_PROCTOOLS_PROPERTY_H

#include <string>
#include <sstream>
#include <stdexcept>
#include <map>
#include <boost/utility/addressof.hpp>

namespace proctools
{

	// --- property_base -----------------------------------------------------

	class property_base
	{
	public:
		virtual void set( const std::string& value ) = 0;
		virtual std::string as_string() = 0;

		const std::string& name() const { return m_name; }

	protected:
		property_base( const std::string& name_ ): m_name( name_ ) {}
		virtual ~property_base() {}

	private:
		std::string m_name;
	};

	// --- property ----------------------------------------------------------

	template<typename Ty>
	class property;

	template<typename Ty>
	std::ostream& operator<<(std::ostream& os, const property<Ty>& value);

	class property_bag;

	template<typename Ty>
	class property: public property_base
	{
	public:
		typedef Ty value_type;

		property( const std::string& name_, const Ty& value_ );

		property& operator=(const value_type& value);

		template<typename From>
		property& operator=(const From& value);

		operator const value_type&() const { return m_value; }
		operator value_type&() { return m_value; }

		virtual void set(const std::string& value);
		virtual std::string as_string();

		friend std::ostream& operator<< <>(std::ostream& os, const property<Ty>& value);

	private:
		property(const value_type& value): m_value( value ) { }
		Ty m_value;
	};

	template<typename Ty>
	property<Ty>& property<Ty>::operator=(const Ty& value)
	{
		m_value = value;
		return *this;
	}

	template<typename Ty> template<typename From>
	property<Ty>& property<Ty>::operator=(const From& value)
	{
		std::stringstream str;
		str << value;
		if ((str >> m_value).fail())
			throw std::invalid_argument("property<Ty>::operator=(const From& value)");
		return *this;
	}

	template<> template<typename From>
	property<std::string>& property<std::string>::operator=(const From& value)
	{
		std::ostringstream str;
		str << value;
		m_value = str.str();
		return *this;
	}

	template<typename Ty>
	void property<Ty>::set(const std::string& value)
	{
		*this = value;
	}

	template<typename Ty>
	std::string property<Ty>::as_string()
	{
		std::stringstream str;
		str << m_value;
		return str.str();
	}

	template<typename Ty>
	std::ostream& operator<<(std::ostream& os, const property<Ty>& value)
	{
		return os << value.m_value;
	}

	// --- property_bag ------------------------------------------------------

	class property_bag
	{
	public:
		typedef std::map<std::string, property_base*> property_map;

		const std::string& append( const std::string& name_, property_base* property_ );

		property_map::iterator begin() { return m_values.begin(); }
		property_map::iterator end() { return m_values.end(); }

		bool set(const std::string& name, const std::string& value);

		bool operator==(const property_bag& other);
		bool operator!=(const property_bag& other);

	protected:
		property_bag() {}
		virtual ~property_bag() {}

		property_bag& operator=(const property_bag& object);

	private:
		property_bag(const property_bag& object); // property bags must be constructed prior to copying

		property_map m_values;
	};

#define PROCTOOLS_INIT_PROPERTY( property_, default_ ) \
	property_( append( #property_, &property_ ), default_ )

	// --- property ----------------------------------------------------------

	template<typename Ty>
	property<Ty>::property( const std::string& name_, const Ty& value_ ):
			property_base( name_ ),
			m_value( value_ )
	{
	}

}

#endif // VGS_PROCTOOLS_PROPERTY_H

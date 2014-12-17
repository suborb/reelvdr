#include "property.h"
#include <algorithm>

namespace proctools
{

	using namespace std;

	// --- property ----------------------------------------------------------

	template<>
	std::string property<std::string>::as_string()
	{
		return m_value;
	}

	// --- property_bag helper functors --------------------------------------

	struct bag_setter
	{
		bag_setter(property_bag::property_map& target): m_target( target ) { }
		void operator()(const property_bag::property_map::value_type& item);
		property_bag::property_map& m_target;
	};

	void bag_setter::operator()(const property_bag::property_map::value_type& item)
	{
		m_target[item.first]->set(item.second->as_string());
	}

	// --- property_bag ------------------------------------------------------

	property_bag::property_bag(const property_bag& object)
	{
		*this = object;
	}

	property_bag& property_bag::operator=(const property_bag& object)
	{
		for_each(object.m_values.begin(), object.m_values.end(), bag_setter( m_values ));
		return *this;
	}

	bool property_bag::set(const string& name, const string& value)
	{
		property_map::iterator it = m_values.find(name);
		if (it == m_values.end())
			return false;
		it->second->set(value);
		return true;
	}

	const std::string& property_bag::append( const std::string& name_, property_base* property_ )
	{
		m_values[ name_ ] = property_;
		return name_;
	}

}

#ifndef VGS_PROCTOOLS_ENVIRONMENT_H
#define VGS_PROCTOOLS_ENVIRONMENT_H

#include <map>
#include <string>

namespace proctools
{

	class environment
	{
	public:
		void set();
		void put(const std::string& name, const std::string& value);

	private:
		std::map<std::string,std::string> m_variables;
	};

	inline void environment::put(const std::string& name,
								 const std::string& value)
	{
		m_variables[name] = value;
	}

};

#endif // VGS_PROCTOOLS_ENVIRONMENT_H

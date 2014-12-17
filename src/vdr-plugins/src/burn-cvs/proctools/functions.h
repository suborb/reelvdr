#ifndef VGS_PROCTOOLS_FUNCTIONS_H
#define VGS_PROCTOOLS_FUNCTIONS_H

#include <sstream>
#include <numeric>

namespace proctools 
{

	template<typename Type>
	void deleter(Type* object)
	{
		delete object;
	}

	template<typename To, typename From>
	To convert(const From& from)
	{
		std::stringstream str;
		str << from;

		To result;
		str >> result;
		return result;
	}

	template<typename InIt, typename Ty, typename Fn>
	Ty sum(InIt first, InIt last, Ty val, Fn func)
	{
		Ty result( val );
		for (InIt cur = first; cur != last; ++cur)
			result += func(*cur);
		return result;
	}
	
	template<typename InIt, typename Ty, typename Fn>
	Ty sum(InIt first, InIt last, Ty val, Fn func, Ty sep)
	{
		Ty result( val );
		for (InIt cur = first; cur != last; ) {
			result += func(*cur);
			if (++cur != last)
				result += sep;
		}
		return result;
	}

};

#endif // VGS_PROCTOOLS_FUNCTIONS_H

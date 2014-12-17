#ifndef VGS_PROCTOOLS_SINGLETON_H
#define VGS_PROCTOOLS_SINGLETON_H

namespace proctools
{

	template<typename T>
	class startable
	{
	public:
		static void start() { if (m_instance == NULL) m_instance = new T();}
		static void stop() { delete m_instance; m_instance = NULL; }

	protected:
		static T* m_instance;
	};

	template<typename T>
	T* startable<T>::m_instance = NULL;

};

#endif // VGS_PROCTOOLS_SINGLETON_H

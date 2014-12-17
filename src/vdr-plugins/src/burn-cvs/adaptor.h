/*
 * See the files COPYING and README for copyright information and how to reach
 * the author.
 *
 *  $Id: adaptor.h,v 1.4 2006/09/16 18:33:36 lordjaxom Exp $
 */

#ifndef VDR_BURN_ADAPTOR_H
#define VDR_BURN_ADAPTOR_H

#include <algorithm>
#include <iterator>
#include <vdr/epg.h>
#include <vdr/tools.h>

namespace vdr_burn
{
	namespace adaptor
	{

		class component_iterator:
				public std::iterator<std::forward_iterator_tag, tComponent>
		{
		public:
			component_iterator(): m_components(0), m_index(0) {}
			explicit component_iterator(const cComponents* components):
					m_components(components), m_index(0) {}

			bool operator==(const component_iterator& rhs);
			bool operator!=(const component_iterator& rhs) { return !(*this == rhs); }
			component_iterator& operator++();
			component_iterator operator++(int);

			reference operator*() { return *m_components->Component(m_index); }
			pointer operator->() { return m_components->Component(m_index); }

		private:
			const cComponents* m_components;
			int m_index;
		};

		inline
		bool component_iterator::operator==(const component_iterator& rhs)
		{
			if (m_components != rhs.m_components)
				return false;
			if (m_components == 0)
				return true;
			return m_index == rhs.m_index;
		}

		inline
		component_iterator& component_iterator::operator++()
		{
			if (m_index < m_components->NumComponents() - 1)
				++m_index;
			else
				m_components = 0;
			return *this;
		}

		inline
		component_iterator component_iterator::operator++(int)
		{
			component_iterator temp( *this );
			++*this;
			return temp;
		}

		template<typename FwdIt, typename Pred>
		FwdIt find_nth(FwdIt first, FwdIt last, int nth, Pred pred)
		{
			while ((first = std::find_if(first, last, pred)) != last && --nth)
				++first;
			return first;
		}

        template<typename Item>
		class list_iterator: public std::iterator<std::bidirectional_iterator_tag, Item>
		{
        public:
		    typedef std::iterator<std::bidirectional_iterator_tag, Item> base_type;
		    typedef typename base_type::reference reference;
		    typedef typename base_type::pointer pointer;

            list_iterator(cList<Item>& list): m_item( list.First() ) { }
            list_iterator(): m_item( 0 ) { }
            list_iterator(const list_iterator<Item>& right): m_item( right.m_item ) { }

            list_iterator<Item>& operator=(const list_iterator<Item>& right) { m_item = right.m_item; }

            reference operator*() { return *m_item; }
            pointer operator->() { return m_item; }

            list_iterator<Item>& operator++() { m_item = static_cast<pointer>( m_item->Next() ); return *this; }
            list_iterator<Item> operator++(int) { list_iterator<Item> result( *this ); ++*this; return result; }
            list_iterator<Item>& operator--() { m_item = static_cast<pointer>( m_item->Prev() ); return *this; }
            list_iterator<Item> operator--(int) { list_iterator<Item> result( *this ); --*this; return result; }

            bool operator==(const list_iterator<Item>& right) { return m_item == right.m_item; }
            bool operator!=(const list_iterator<Item>& right) { return m_item != right.m_item; }

        private:
            Item* m_item;
		};

	}
}

#endif // VDR_BURN_ADAPTOR_H

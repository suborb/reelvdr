/*
 * See the files COPYING and README for copyright information and how to reach
 * the author.
 *
 *  $Id: filter.h,v 1.5 2006/09/16 18:33:36 lordjaxom Exp $
 */

#ifndef VDR_BURN_FILTER_H
#define VDR_BURN_FILTER_H

#include <algorithm>
#include "common.h"
#include <iterator>

namespace vdr_burn
{

	template<typename FwdIt, typename Pred>
	class filter_iterator:
			public std::iterator<std::forward_iterator_tag, typename FwdIt::value_type>
	{
	public:
		typedef typename FwdIt::value_type value_type;
		typedef typename FwdIt::reference reference;
		typedef typename FwdIt::pointer pointer;

		filter_iterator(FwdIt first, FwdIt last, Pred pred);

		bool operator==(const filter_iterator& rhs) { return m_current == rhs.m_current; }
		bool operator!=(const filter_iterator& rhs) { return m_current != rhs.m_current; }
		filter_iterator& operator++() { move_next(); return *this; }
		filter_iterator operator++(int) { filter_iterator temp( *this ); move_next(); return temp; }

		reference operator*() { return *m_current; }
		pointer operator->() { return &*m_current; }

	protected:
		void move_next();

	private:
		FwdIt m_current;
		FwdIt m_last;
		Pred m_pred;
	};

	template<typename FwdIt, typename Pred>
	filter_iterator<FwdIt, Pred>::filter_iterator(FwdIt first, FwdIt last, Pred pred):
			m_current( first ),
			m_last( last ),
			m_pred( pred )
	{
		if (m_current != m_last && !m_pred(*m_current))
			move_next();
	}

	template<typename FwdIt, typename Pred>
	void filter_iterator<FwdIt, Pred>::move_next()
	{
		m_current = std::find_if(++m_current, m_last, m_pred);
	}

	class track_predicate
	{
	public:
		enum used_filter
		{
			unused = 0,
			used   = 1,
			all    = 2
		};

		track_predicate(track_info::streamtype content, used_filter used_):
				m_content( content ), m_used( used_ ) {}

		bool operator()(const track_info& track)
		{
			return (m_content == track_info::streamtype( 0 ) ||
					m_content == track.type) &&
				   (m_used == all || m_used == track.used);
		}

	private:
		track_info::streamtype m_content;
		used_filter m_used;
	};

	template<typename List, typename It>
	class track_filter_t
	{
	public:
		typedef filter_iterator<It, track_predicate> iterator;

		track_filter_t(List& list, track_info::streamtype content, track_predicate::used_filter used_);
		track_filter_t(List& list, track_predicate::used_filter used_);

		iterator begin();
		iterator end();

	private:
		List& m_list;
		track_info::streamtype m_content;
		track_predicate::used_filter m_used;
	};

	template<typename List, typename It>
	track_filter_t<List, It>::track_filter_t(List& list, track_info::streamtype content, track_predicate::used_filter used_):
			m_list( list ),
			m_content( content ),
			m_used( used_ )
	{
	}

	template<typename List, typename It>
	track_filter_t<List, It>::track_filter_t(List& list, track_predicate::used_filter used_):
			m_list( list ),
			m_content( track_info::streamtype( 0 ) ),
			m_used( used_ )
	{
	}

	template<typename List, typename It>
	typename track_filter_t<List, It>::iterator track_filter_t<List, It>::begin()
	{
		return iterator( m_list.begin(), m_list.end(), track_predicate( m_content, m_used ) );
	}

	template<typename List, typename It>
	typename track_filter_t<List, It>::iterator track_filter_t<List, It>::end()
	{
		return iterator( m_list.end(), m_list.end(), track_predicate( m_content, m_used ) );
	}

	typedef track_filter_t<track_info_list, track_info_list::iterator> track_filter;
	typedef track_filter_t<const track_info_list, track_info_list::const_iterator>
			const_track_filter;

}

#endif // VDR_BURN_FILTER_H

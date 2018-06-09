#pragma once

#include <vector>
#include <algorithm>

template <typename T, typename COMPARE=std::less<T> >
struct sorted_vector {
	typedef typename std::vector<T>::iterator       iterator;
	typedef typename std::vector<T>::const_iterator const_iterator;

	iterator		begin()       { return v.begin();	}
	iterator		end()         { return v.end();		}
	const_iterator	begin() const { return v.begin();	}
	const_iterator	end()   const { return v.end();		}

	std::vector<T>	v;
	COMPARE			cmp;

	sorted_vector (COMPARE const& c = COMPARE()): v(), cmp(c) {}

	template <typename ITERATOR>
	sorted_vector (ITERATOR first, ITERATOR last, COMPARE const& c = COMPARE()): v(first, last), cmp(c) {
		std::sort(v.begin(), v.end(), cmp);
	}


	size_t size () const {	return v.size(); }

	template <typename T>
	const_iterator find (T const& val) const {
		const_iterator i = std::lower_bound(v.begin(), v.end(), val, cmp);
		return i == v.end() || cmp(val, *i) ? v.end() : i;
	}
	template <typename T>
	iterator find (T const& val) {
		iterator i = std::lower_bound(v.begin(), v.end(), val, cmp);
		return i == v.end() || cmp(val, *i) ? v.end() : i;
	}

	template <typename T>
	bool contains (T const& val) {
		return find(val) != end();
	}

	iterator insert (T val) {
		iterator i = std::lower_bound(v.begin(), v.end(), val, cmp);
		if (i == v.end() || cmp(val, *i))
			return v.emplace(i, std::move(val));
		return v.end();
	}

	iterator find_or_insert (T val) {
		iterator i = std::lower_bound(v.begin(), v.end(), val, cmp);
		if (i == v.end() || cmp(val, *i))
			v.emplace(i, std::move(val));
		return i;
	}

	iterator erase (iterator it) {
		return v.erase(it);
	}

	template <typename U>
	bool try_erase (U const& val) {
		auto it = find(val);
		if (it == v.end())
			return false; // not found, not erased

		erase(it);
		return true; // found, erased
	}
	template <typename U>
	bool try_extract (U const& val, T* out) {
		auto it = find(val);
		if (it == v.end())
			return false; // not found, not erased

		*out = std::move(*it);
		erase(it);
		return true; // found, extracted, erased
	}
};

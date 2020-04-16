/*  Usage:
 *  std::list<int> mylist;
 *  list_sort(mylist);
 *  // or:
 *  list_sort(mylist, [](int left, int right) { return left > right; });
 */
#ifndef LIPH_LIST_SORT_HPP
#define LIPH_LIST_SORT_HPP

#include <list>
#include <functional>

template<typename T, typename A, typename Comp>
auto list_merge_ranges(std::list<T, A> &list, typename std::list<T, A>::iterator first, typename std::list<T, A>::iterator mid,
       typename std::list<T, A>::iterator last, Comp comp) {
    auto it1 = first;
    auto it2 = mid;

    if(first == mid || mid == last)
        return std::make_pair(first, last);

    auto new_first = comp(*it2, *it1) ? it2 : it1;

    while(it1 != mid) {
        while(it2 != last && comp(*it2, *it1)) {
            ++it2;
        }

        list.splice(it1, list, mid, it2);
        mid = it2;

        if(it2 == last) {
            return std::make_pair(new_first, last);
        }

        while(it1 != mid && !comp(*it2, *it1)) {
            ++it1;
        }
    }

    return std::make_pair(new_first, last);
}


template<typename T, typename A>
auto list_merge_ranges(std::list<T, A> &list1, typename std::list<T, A>::iterator first1, typename std::list<T, A>::iterator last1,
       std::list<T, A> &list2, typename std::list<T, A>::iterator first2, typename std::list<T, A>::iterator last2) {
    return list_merge_ranges(list1, first1, last1, list2, first2, last2, std::less<T>());
}


template<typename T, typename A, typename Comp>
auto list_sort(std::list<T, A> &list, typename std::list<T, A>::iterator first, typename std::list<T, A>::size_type count, Comp comp) {
    if(first == list.end())
        return std::make_pair(first, first);

    if(count <= 1) {
        if(count == 0)
            return std::make_pair(first, first);
        auto last = first;
        return std::make_pair(first, ++last);
    }

    auto [new_first, mid] = list_sort(list, first, count / 2, comp);
    auto [new_mid, last] = list_sort(list, mid, count - count / 2, comp);
    return list_merge_ranges(list, new_first, new_mid, last, comp);
}


template<typename T, typename A>
auto list_sort(std::list<T, A> &list, typename std::list<T, A>::iterator first, typename std::list<T, A>::size_type count) {
    return list_sort(list, first, count, std::less<T>());
}


template<typename T, typename A, typename Comp>
void list_sort(std::list<T, A> &list, Comp comp) {
    list_sort(list, list.begin(), list.size(), comp);
}


template<typename T, typename A>
void list_sort(std::list<T, A> &list) {
    list_sort(list, std::less<T>());
}

#endif

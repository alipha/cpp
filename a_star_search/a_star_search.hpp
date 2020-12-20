#ifndef LIPH_A_STAR_SEARCH_HPP
#define LIPH_A_STAR_SEARCH_HPP

#include <deque>
#include <functional>
#include <iterator>
#include <optional>
#include <queue>
#include <set>
#include <stdexcept>
#include <type_traits>
#include <unordered_set>
#include <utility>
#include <vector>


template<typename From, typename To>
concept ConvertibleTo = std::is_convertible_v<From, To>;


template<typename T>
concept LessThanComparable = requires(const T &x) {
    { x < x } -> ConvertibleTo<bool>;
};


template<typename T>
concept InputIterator = requires(T it) {
    *it;
    ++it;
    { it != it } -> ConvertibleTo<bool>;
};


template<typename T>
concept Container = requires(T c) {
    { std::begin(c) } -> InputIterator;
    { std::end(c) } -> InputIterator;
};


template<typename T>
concept UnorderedSetStorable = requires(const T &x) {
    x == x;
    std::hash<T>()(x);
};


template<typename T>
concept SetAndUnorderedSetStorable = LessThanComparable<T> && UnorderedSetStorable<T>;


template<typename T>
concept HasAdditionalCost = requires(const T &x) {
    { x.additional_cost() + x.estimated_remaining_cost() } -> LessThanComparable;
};


template<typename T>
concept HasAccruedCost = requires(const T &x) {
    { x.accrued_cost() + x.estimated_remaining_cost() } -> LessThanComparable;
};


template<typename T>
concept SearchProblemState = requires(const T &s) {
    requires HasAdditionalCost<T> || HasAccruedCost<T>;
    { s.done() } -> ConvertibleTo<bool>;
    { s.next_states() } -> Container
};


namespace detail {


template<typename T>
struct cost_type { 
    static_assert(sizeof(T) && false, "SearchProblemState must implement either additional_cost or accrued_cost"); 
};

template<HasAdditionalCost T>
struct cost_type<T> {
   using type = decltype(std::declval<T&>().additional_cost());
};

template<HasAccruedCost T>
struct cost_type<T> {
    using type = decltype(std::declval<T&>().accrued_cost());
};

template<typename T>
using cost_t = cost_type<T>::type;



template<typename T>
struct container_type {
    using type = std::deque<T>;
};

template<typename T>
    requires LessThanComparable<typename T::state_type>
struct container_type<T> {
    struct less { bool operator()(const T &left, const T &right) const { return left.state < right.state; } };
    using type = std::set<T, less>;
};

template<typename T>
    requires UnorderedSetStorable<typename T::state_type>
struct container_type<T> {
    struct hash { std::size_t operator()(const T &x) const { return std::hash<typename T::state_type>{}(x.state); } };
    struct equal_to { bool operator()(const T &left, const T &right) const { return left.state == right.state; } };
    using type = std::unordered_set<T, hash, equal_to>;
};

template<typename T>
    requires SetAndUnorderedSetStorable<typename T::state_type>
struct container_type<T> {
struct hash { std::size_t operator()(const T &x) const { return std::hash<typename T::state_type>{}(x.state); } };
    struct equal_to { bool operator()(const T &left, const T &right) const { return left.state == right.state; } };
    using type = std::unordered_set<T, hash, equal_to>;
};

template<typename T>
using container_t = container_type<T>::type;



template<SearchProblemState State>
class searcher {
public:
    using cost_type = cost_t<State>;

    struct link_type {
        using state_type = State;
        State state;
        mutable cost_type accrued_cost;
        mutable const link_type *prev;
    };

private:
    using container_type = container_t<link_type>;
    using iterator = typename container_t<link_type>::const_iterator; 

    static constexpr bool is_ordered_container = !std::is_same_v<container_type, std::deque<link_type>>;

    struct next_link_type {
        const link_type *link;
        cost_type accrued_cost;
        cost_type estimated_total_cost;

        bool operator>(const next_link_type &other) const {
            return other.estimated_total_cost < estimated_total_cost;
        }
    };

public:
    const link_type *run(const State &initial_state) {
        const link_type *new_state = add_state(states.end(), link_type{initial_state, cost_type{}, nullptr});
        add_next_states(cost_type{}, new_state);

        while(!next_states.empty() && !next_states.top().link->state.done()) {
            next_link_type next = next_states.top();
            next_states.pop();
            add_next_states(next.accrued_cost, next.link);
        }

        if(next_states.empty())
            return nullptr;

        return next_states.top().link;
    }

    void add_next_states(cost_type prev_cost, const link_type *prev_link) {
        for(auto &next : prev_link->state.next_states()) {
            cost_type accrued = accrued_cost(prev_cost, next);
            
            iterator it = existing_state(next);
            if(it != states.end() && it->accrued_cost < accrued)
                continue;

            const link_type *new_state = add_state(it, link_type{next, accrued, prev_link});
            
            cost_type estimated = accrued + next.estimated_remaining_cost();

            if(estimated < accrued)
                throw std::logic_error("estimated_remaining_cost cannot be negative");
            if(!(accrued < estimated) && !next.done())
                throw std::logic_error("estimated_remaining_cost cannot be zero unless the done state is reached");
            next_states.push(next_link_type{new_state, accrued, estimated});
        }
    }

private:
    template<HasAdditionalCost T>
    static cost_type accrued_cost(const cost_type &prev_cost, const T &current_state) {
        return prev_cost + current_state.additional_cost();
    }

    template<HasAccruedCost T>
    static cost_type accrued_cost(const cost_type &, const T &state) {
        return state.accrued_cost();
    }


    const link_type *add_state(iterator existing_it, link_type &&state) requires is_ordered_container {
        if(existing_it == states.end())
            return &*states.insert(std::move(state)).first;
        
        existing_it->accrued_cost = std::move(state.accrued_cost);
        existing_it->prev = state.prev;
        return &*existing_it;
    }

    const link_type *add_state(iterator, link_type &&state) requires !is_ordered_container {
        states.push_back(std::move(state));
        return &states.back();
    }


    template<bool IsOrdered = is_ordered_container>
    iterator existing_state(const State &new_state) const requires is_ordered_container {
        // TODO: make find work with different types
        return states.find(link_type{new_state, cost_type{}, nullptr});
    }
    
    iterator existing_state(const State &) const requires !is_ordered_container {
        return states.end();
    }


    container_type states;
    std::priority_queue<next_link_type, std::vector<next_link_type>, std::greater<next_link_type>> next_states; 
};


}  // namespace detail



template<SearchProblemState State>
std::deque<State> a_star_search(const State &initial_state) {
    detail::searcher<State> s;
    const typename detail::searcher<State>::link_type *link = s.run(initial_state);

    if(!link)
        return {};

    std::deque<State> states;
    while(link) {
        states.push_front(link->state);
        link = link->prev;
    }

    return states;
}


#endif

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


namespace detail {


template<typename T>
concept HasAdditionalCost = requires(const T &x) { x.additional_cost(); };

template<typename T, typename Param>
concept HasAdditionalCostWithParam = requires(const T &x, Param p) { x.additional_cost(p); };


template<typename T>
concept HasAccruedCost = requires(const T &x) { x.accrued_cost(); };

template<typename T, typename Param>
concept HasAccruedCostWithParam = requires(const T &x, Param p) { x.accrued_cost(p); };


template<typename T>
concept HasEstimatedRemainingCost = requires(const T &x) { x.estimated_remaining_cost(); };

template<typename T, typename Param>
concept HasEstimatedRemainingCostWithParam = requires(const T &x, Param p) { x.estimated_remaining_cost(p); };


template<typename T>
concept HasDone = requires(const T &x) {
    { x.done() } -> ConvertibleTo<bool>;
};

template<typename T, typename Param>
concept HasDoneWithParam = requires(const T &x, Param p) {
    { x.done(p) } -> ConvertibleTo<bool>;
};


template<typename T>
concept HasNextStates = requires(const T &x) { 
    { x.next_states() } -> Container;
};

template<typename T, typename Param>
concept HasNextStatesWithParam = requires(const T &x, Param p) { 
    { x.next_states(p) } -> Container;
};

} // namespace detail


template<typename T, typename GlobalState>
concept SearchProblemState = requires(const T &s) {
    requires detail::HasAdditionalCost<T> || detail::HasAdditionalCostWithParam<T, GlobalState> 
          || detail::HasAccruedCost<T>    || detail::HasAccruedCostWithParam<T, GlobalState>;
    requires detail::HasDone<T>           || detail::HasDoneWithParam<T, GlobalState>;
    requires detail::HasNextStates<T>     || detail::HasNextStatesWithParam<T, GlobalState>;
};


namespace detail {


struct no_global_state {};


template<typename GlobalState, typename T>
struct cost_type { 
    static_assert(sizeof(T) && false, "SearchProblemState must implement either additional_cost or accrued_cost"); 
};

template<typename GlobalState, HasAdditionalCost T>
struct cost_type<GlobalState, T> {
   using type = decltype(std::declval<T&>().additional_cost());
};

template<typename GlobalState, HasAccruedCost T>
struct cost_type<GlobalState, T> {
    using type = decltype(std::declval<T&>().accrued_cost());
};

template<typename GlobalState, HasAdditionalCostWithParam<GlobalState> T>
struct cost_type<GlobalState, T> {
   using type = decltype(std::declval<T&>().additional_cost(std::declval<GlobalState&>()));
};

template<typename GlobalState, HasAccruedCostWithParam<GlobalState> T>
struct cost_type<GlobalState, T> {
    using type = decltype(std::declval<T&>().accrued_cost(std::declval<GlobalState&>()));
};

template<typename GlobalState, typename T>
using cost_t = cost_type<GlobalState, T>::type;


template<typename T>
struct state_equal_to {
    using is_transparent = void;

    bool operator()(const T &left, const T &right) const { 
        return left.state == right.state; 
    }

    bool operator()(const typename T::state_type &left, const T &right) const { 
        return left == right.state; 
    }

    bool operator()(const T &left, const typename T::state_type &right) const { 
        return left.state == right; 
    }
};

template<typename T>
struct state_hash {
    using is_transparent = void;

    std::size_t operator()(const T &x) const { 
        return std::hash<typename T::state_type>{}(x.state); 
    }

    std::size_t operator()(const T::state_type &x) const { 
        return std::hash<typename T::state_type>{}(x); 
    }
};


template<typename T>
struct container_type {
    using type = std::deque<T>;
};

template<typename T>
    requires LessThanComparable<typename T::state_type>
struct container_type<T> {
    struct less {
        using is_transparent = void;

        bool operator()(const T &left, const T &right) const { 
            return left.state < right.state; 
        } 

        bool operator()(const typename T::state_type &left, const T &right) const { 
            return left < right.state; 
        } 

        bool operator()(const T &left, const typename T::state_type &right) const { 
            return left.state < right; 
        } 
    };
    using type = std::set<T, less>;
};

template<typename T>
    requires UnorderedSetStorable<typename T::state_type>
struct container_type<T> {
    using hash = state_hash<T>;
    using equal_to = state_equal_to<T>;
    using type = std::unordered_set<T, hash, equal_to>;
};

template<typename T>
    requires SetAndUnorderedSetStorable<typename T::state_type>
struct container_type<T> {
    using hash = state_hash<T>;
    using equal_to = state_equal_to<T>;
    using type = std::unordered_set<T, hash, equal_to>;
};

template<typename T>
using container_t = container_type<T>::type;



template<typename GlobalState, SearchProblemState<GlobalState> State>
class searcher {
public:
    using cost_type = cost_t<GlobalState, State>;

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
    searcher(GlobalState &global_state) : global_state(global_state) {}

    const link_type *run(const State &initial_state) {
        const link_type *new_state = add_state(states.end(), link_type{initial_state, cost_type{}, nullptr});
        add_next_states(cost_type{}, new_state);

        while(!next_states.empty() && !done(next_states.top().link->state)) {
            next_link_type next = next_states.top();
            next_states.pop();
            add_next_states(next.accrued_cost, next.link);
        }

        if(next_states.empty())
            return nullptr;

        return next_states.top().link;
    }

    void add_next_states(cost_type prev_cost, const link_type *prev_link) {
        for(auto &next : get_next_states(prev_link->state)) {
            cost_type accrued = accrued_cost(prev_cost, next);
            
            iterator it = existing_state(next);
            if(it != states.end() && it->accrued_cost < accrued)
                continue;

            const link_type *new_state = add_state(it, link_type{next, accrued, prev_link});
            
            cost_type estimated = accrued + estimated_remaining_cost(next);

            if(estimated < accrued)
                throw std::logic_error("estimated_remaining_cost cannot be negative");
            if(!(accrued < estimated) && !done(next))
                throw std::logic_error("estimated_remaining_cost cannot be zero unless the done state is reached");
            next_states.push(next_link_type{new_state, accrued, estimated});
        }
    }

private:
    cost_type accrued_cost(const cost_type &prev_cost, const HasAdditionalCost &current_state) {
        return prev_cost + current_state.additional_cost();
    }

    cost_type accrued_cost(const cost_type &, const HasAccruedCost &state) {
        return state.accrued_cost();
    }

    cost_type estimated_remaining_cost(const HasEstimatedRemainingCost &state) {
        return state.estimated_remaining_cost();
    }

    auto get_next_states(const HasNextStates &state) { return state.next_states(); }

    bool done(const HasDone &state) { return state.done(); }


    cost_type accrued_cost(const cost_type &prev_cost, const HasAdditionalCostWithParam<GlobalState> &current_state) {
        return prev_cost + current_state.additional_cost(global_state);
    }

    cost_type accrued_cost(const cost_type &, const HasAccruedCostWithParam<GlobalState> &state) {
        return state.accrued_cost(global_state);
    }

    cost_type estimated_remaining_cost(const HasEstimatedRemainingCostWithParam<GlobalState> &state) {
        return state.estimated_remaining_cost(global_state);
    }

    auto get_next_states(const HasNextStatesWithParam<GlobalState> &state) {
        return state.next_states(global_state);
    }

    bool done(const HasDoneWithParam<GlobalState> &state) { return state.done(global_state); }



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


    iterator existing_state(const State &new_state) const requires is_ordered_container {
        return states.find(new_state);
    }
    
    iterator existing_state(const State &) const requires !is_ordered_container {
        return states.end();
    }


    GlobalState &global_state;
    container_type states;
    std::priority_queue<next_link_type, std::vector<next_link_type>, std::greater<next_link_type>> next_states; 
};


}  // namespace detail



template<typename GlobalState, SearchProblemState<GlobalState> State>
std::deque<State> a_star_search(const State &initial_state, GlobalState &global_state) {
    detail::searcher<GlobalState, State> s(global_state);
    const typename detail::searcher<GlobalState, State>::link_type *link = s.run(initial_state);

    if(!link)
        return {};

    std::deque<State> states;
    while(link) {
        states.push_front(link->state);
        link = link->prev;
    }

    return states;
}


template<SearchProblemState<detail::no_global_state> State>
std::deque<State> a_star_search(const SearchProblemState<detail::no_global_state> &initial_state) {
    return a_star_state(initial_state, detail::no_global_state{});
}


#endif

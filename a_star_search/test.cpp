#include "a_star_search.hpp"

#include <iostream>
#include <cstddef>
#include <deque>
#include <tuple>
#include <vector>


template<typename T>
class matrix {
public:
    matrix(std::size_t width, std::size_t height, const T &value = T{}) 
        : w(width), h(height), data(w * h, value) {}

    const T &operator()(std::size_t x, std::size_t y) const { return data[y * w + x]; }

    T &operator()(std::size_t x, std::size_t y) { return data[y * w + x]; }

    std::size_t width() const { return w; }
    std::size_t height() const { return h; }

private:
    std::size_t w;
    std::size_t h;
    std::vector<T> data;
};


struct point {
    std::size_t x;
    std::size_t y;
};


enum class cell_type : unsigned char { space, obstacle };


namespace detail {


class shortest_path_state {
public:
    shortest_path_state(const matrix<cell_type> *grid, point current, point dest) 
        : grid(grid), cur(current), dest(dest) {}

    bool done() const { return estimated_remaining_cost() == 0; }

    std::size_t additional_cost() const { return 1; }

    std::size_t estimated_remaining_cost() const {
        return (cur.x < dest.x ? dest.x - cur.x : cur.x - dest.x)
             + (cur.y < dest.y ? dest.y - cur.y : cur.y - dest.y);
    };

    std::vector<shortest_path_state> next_states() const {
        std::vector<shortest_path_state> states;
        add_next_state(states, -1,  0);
        add_next_state(states,  1,  0);
        add_next_state(states,  0, -1);
        add_next_state(states,  0,  1);
        return states;
    }

    point current() const { return cur; }
    point destination() const { return dest; }

    // define < so that states are checked to see if they are already visited
    // (states will be stored in a std::set)
    bool operator<(const shortest_path_state &other) const {
        return std::tie(cur.x, cur.y) < std::tie(other.cur.x, other.cur.y);
    }

    /*  // could define == and specialize std::hash to store shortest_path_state objects in
        // a std;:unordered_set instead of a std::set
    bool operator==(const shortest_path_state &other) const {
        return std::tie(cur.x, cur.y) == std::tie(other.cur.x, other.cur.y);
    }
    */

private:
    void add_next_state(std::vector<shortest_path_state> &states, int x_change, int y_change) const {
        if(    (x_change < 0 && cur.x == 0)
            || (x_change > 0 && cur.x == grid->width() - 1)
            || (y_change < 0 && cur.y == 0)
            || (y_change > 0 && cur.y == grid->height() - 1)
            || ((*grid)(cur.x + x_change, cur.y + y_change) == cell_type::obstacle))
            return;

        states.push_back(shortest_path_state{grid, point{cur.x + x_change, cur.y + y_change}, dest});
    }

    const matrix<cell_type> *grid;
    point cur;
    point dest;
};


} // namespace detail

/* // see above comment about ==
namespace std {
    template<>
    struct hash<detail::shortest_path_state> {
        size_t operator()(const detail::shortest_path_state &state) {
            return (state.current().x * 16777619) ^ state.current().y;
        }
    };
}
*/

std::vector<point> shortest_path_search(const matrix<cell_type> &grid, point start, point dest) {
    detail::shortest_path_state initial_state{&grid, start, dest};
    std::deque<detail::shortest_path_state> path = a_star_search(initial_state);

    std::vector<point> result;
    for(detail::shortest_path_state &state : path)
        result.push_back(state.current());

    return result;
}



template<std::size_t Width, std::size_t Height>
matrix<cell_type> arrays_to_matrix(const int (&obstacles)[Height][Width]) {
    matrix<cell_type> result(Width, Height);

    for(std::size_t y = 0; y < Height; ++y)
        for(std::size_t x = 0; x < Width; ++x)
            result(x, y) = obstacles[y][x] ? cell_type::obstacle : cell_type::space;

    return result;
}


int main() {
    int obstacles[][8] = 
        {{0, 0, 0, 0, 1, 0, 0, 0},
         {0, 1, 0, 0, 0, 0, 1, 0},
         {0, 1, 1, 0, 1, 1, 0, 0},
         {0, 0, 0, 1, 0, 0, 0, 0},
         {0, 0, 0, 1, 0, 0, 1, 0},
         {0, 1, 0, 0, 0, 1, 0, 0},
         {0, 0, 0, 1, 1, 0, 0, 0}};

    matrix<cell_type> grid = arrays_to_matrix(obstacles);
    std::vector<point> path = shortest_path_search(grid, point{0, 0}, point{6, 5});
    
    std::cout << path.size() << '\n';
    for(point p : path)
        std::cout << p.x << ", " << p.y << '\n';
}

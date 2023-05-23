#include <map>
#include <iostream>
using namespace std;

template<typename T>
struct S {
    inline static map<T, T> m;
    inline static T i;
    static void init() { m.emplace(T(), T()); i = 7; cout << "init" << endl; }

    static bool dummy;

    template <typename U, U>
    struct dummy_value {};

    typedef dummy_value<map<T, T> &, m> 
        dummy_map_value_type;   // force m to get evaluated

    typedef dummy_value<bool &, dummy>
        dummy_value_type;  // force dummy to get evaluated*/
};

template <typename T>
bool S<T>::dummy = (S<T>::init(), true);


int main() {
    S<int> s;
    cout << s.i << endl;
}


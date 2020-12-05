#include "compute_property.hpp"
#include "../shapes/all_shapes.hpp"
#include "../properties/all_properties.hpp"
#include "../multi_dispatch/multi_dispatch.hpp"


namespace impl {
    
    double compute_property(const circle &c, const area &) { return 3.1415926 * c.radius * c.radius; }
    double compute_property(const square &s, const area &) { return s.width * s.width; }
    double compute_property(const rectangle &r, const area &) { return (r.x2 - r.x1) * (r.y2 - r.y1); }
    
    double compute_property(const circle &c, const circumference &) { return 2 * 3.1415926 * c.radius; }
    double compute_property(const square &s, const circumference &) { return 4 * s.width; }
    double compute_property(const rectangle &r, const circumference &) { return 2 * (r.x2 - r.x1 + r.y2 - r.y1); }
}


double compute_property(const shape &s, const property &p) {
    return multi_dispatch<shapes, properties>([](auto &s, auto &p) {
        return impl::compute_property(s, p);
    }, s, p);
}


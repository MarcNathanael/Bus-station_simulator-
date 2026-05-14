#include <cassert>
#include <iostream>
#include "../src/core/Voiture.h"

int main() {
    Voiture v(1, 1, 3);  // id=1, coop=1, destination=3

    assert(v.get_id() == 1);
    assert(v.get_places_libres() == 32);
    assert(!v.est_pleine());
    assert(v.embarquer(5));
    assert(v.get_places_libres() == 27);

    std::cout << "Tests Voiture OK" << std::endl;
    return 0;
}
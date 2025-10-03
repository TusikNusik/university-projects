#include <bits/stdc++.h>

#include "err.h"

size_t min(size_t z, size_t t) {
    if(z < t)
        return z;
    return t;
}

void syserr(const std::string& x) {
    std::cerr<<"ERROR "<<x<<std::endl;
    exit(1);
}

void error(const std::string& x) {
    std::cerr<<"ERROR "<<x<<std::endl;
}

void komerr(std::vector<uint8_t>& x, size_t rozmiar) {
    std::string z = "";

    for (size_t i = 0; i < min(rozmiar, x.size()); i++) {
        uint8_t byte = x[i];
        char hi = "0123456789ABCDEF"[byte >> 4];
        char lo = "0123456789ABCDEF"[byte & 0x0F];
        z += hi;
        z += lo;
    }

    std::cerr<<"ERROR MSG "<<z<<std::endl;
}
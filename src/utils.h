#ifndef UTILS_H_
#define UTILS_H_

#include "defs.h"
#include <cassert>
#include <fstream>
#include <string>

float dis(const point_t<float>& a, const point_t<float>& b) {
    float dist = 0.f;
    for (int i = 0; i < DIM; ++i) {
        float foo = a.coordinates[i] - b.coordinates[i];
        dist += foo * foo;
    }
    return dist;
}

template<typename T>
int read_vecs(const std::string& file, T*& vecs, int d) {
    int n, dim;
    auto ifs = std::ifstream(file, std::ios::binary);
    assert(ifs.is_open());

    ifs.read((char*)&dim, sizeof(int));
    assert(dim == d);
    ifs.seekg(0, std::ios::end);
    auto fsize = ifs.tellg();
    n = static_cast<int>(fsize / (sizeof(int) + dim * sizeof(T)));
    ifs.seekg(0, std::ios::beg);

    vecs = new T[n * d];
    for (int i = 0; i < n; i++) {
        ifs.ignore(sizeof(int));
        ifs.read((char*)&vecs[i * d], dim * sizeof(T));
    }
    ifs.close();
    return n;
}

template<typename T>
void write_vecs(const std::string& file, const T* vecs, int n, int dim) {
    auto ofs = std::ofstream(file, std::ios::binary);
    assert(ofs.is_open());

    for (int i = 0; i < n; i++) {
        ofs.write((char*)&dim, sizeof(int));
        ofs.write((char*)&vecs[i * dim], dim * sizeof(T));
    }
    ofs.close();
}

#endif // UTILS_H_
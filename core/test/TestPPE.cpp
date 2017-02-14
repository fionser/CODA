#include "core/PPE/Context.hpp"
#include "core/PPE/PubKey.hpp"
#include "core/PPE/SecKey.hpp"
#include "core/PPE/EncVec.hpp"
#include "core/PPE/EncMat.hpp"

#include <NTL/vec_ZZ_p.h>
#include <NTL/mat_ZZ_p.h>
#include <iostream>
#include <fstream>

int test_EncVec() {
    std::vector<long> Ms = {2048, 4096};
    std::vector<long> Ps = {13, 23};
    std::vector<long> Rs = {1, 1};
    ppe::Context context(Ms, Ps, Rs);
    context.buildModChain(7);
    ppe::SecKey sk(context);
    sk.GenSecKey(64);
    sk.addSome1DMatrices();
    ppe::PubKey pk(sk);
    NTL::ZZ P(1);
    for (long p : Ps)
        P *= p;

    Vector vec;
    vec.SetLength(10);
    for (int i = 0; i < vec.length(); i++)
        vec[i] = NTL::to_ZZ(i * 10);

    ppe::EncVec encVec(pk);
    encVec.pack(vec);
    {
        Vector vec2;
        encVec.unpack(vec2, sk);
        if (vec != vec2)
            return -1;
    } // Test packing and unpacking

    {
        ppe::EncVec encVec2(encVec);
        encVec2.add(encVec);
        ppe::EncVec encVec3(encVec);
        encVec3.add(vec);

        Vector vec2, vec3;
        encVec2.unpack(vec2, sk);
        encVec3.unpack(vec3, sk);

        NTL::vec_ZZ_p vec_p, vec_p_2;
        NTL::ZZ_p::init(P);
        NTL::conv(vec_p, vec);
        NTL::add(vec_p_2, vec_p, vec_p);

        NTL::vec_ZZ_p computed;
        NTL::conv(computed, vec2);
        if (computed != vec_p_2)
            return -1;
        NTL::conv(computed, vec3);
        if (computed != vec_p_2)
            return -1;
    } // Test addition

    {
        ppe::EncVec encVec2(encVec);
        ppe::EncVec encVec3(encVec);
        encVec2.sub(encVec);
        encVec3.sub(vec);
        Vector vec2, vec3;
        encVec2.unpack(vec2, sk);
        encVec3.unpack(vec3, sk);
        for (auto itr = vec2.begin(); itr != vec2.end(); itr++)
            if (*itr != 0) return -1;
        for (auto itr = vec3.begin(); itr != vec3.end(); itr++)
            if (*itr != 0) return -1;
    } // Test subtraction

    return 0;
}

int test_io() {
    std::vector<long> Ms = {2048, 4096};
    std::vector<long> Ps = {13, 23};
    std::vector<long> Rs = {1, 1};
    ppe::Context context(Ms, Ps, Rs);
    context.buildModChain(7);

    ppe::SecKey sk(context);
    sk.GenSecKey(64);
    sk.addSome1DMatrices();

    ppe::PubKey pk(sk);
    Vector vec;
    vec.SetLength(7);
    for (int i = 0; i < vec.length(); i++)
        vec[i] = NTL::to_ZZ(i * 10);
    ppe::EncVec encVec(pk);
    encVec.pack(vec);

    {
        std::ofstream out("./tmp.context", std::ios::binary);
        if (!out.is_open()) return -1;
        if (!context.dump(out)) { out.close(); return -1; }
        out.close();
    }
    {
        std::ofstream out("./tmp.sk", std::ios::binary);
        if (!out.is_open()) return -1;
        if (!sk.dump(out)) { out.close(); return -1; }
        out.close();
    }
    {
        std::ofstream out("./tmp.pk", std::ios::binary);
        if (!out.is_open()) return -1;
        if (!pk.dump(out)) { out.close(); return -1; }
        out.close();
    }

    ppe::Context context2;
    ppe::SecKey sk2;
    ppe::PubKey pk2;
    {
        std::ifstream in("./tmp.context", std::ios::binary);
        if (!context2.restore(in)) { in.close(); return -1; }
        in.close();
    }
    {
        std::ifstream in("./tmp.sk", std::ios::binary);
        if (!sk2.restore(in, context2)) { in.close(); return -1; }
        in.close();
    }
    {
        std::ifstream in("./tmp.pk", std::ios::binary);
        if (!pk2.restore(in, context2)) { in.close(); return -1; }
        in.close();
    }

    Vector vec2;
    ppe::EncVec encVec2(pk2);
    encVec2.pack(vec);
    encVec2.unpack(vec2, sk);

    std::cout << vec2 << "\n";
    if (vec != vec2) return -1;
    return 0;
}

int test_EncMat() {
    std::vector<long> Ms = {2048, 4096};
    std::vector<long> Ps = {13, 23};
    std::vector<long> Rs = {1, 1};
    ppe::Context context(Ms, Ps, Rs);
    context.buildModChain(7);
    ppe::SecKey sk(context);
    sk.GenSecKey(64);
    sk.addSome1DMatrices();
    ppe::PubKey pk(sk);
    NTL::ZZ P(1);
    for (long p : Ps)
        P *= p;

    Matrix mat;

    mat.SetDims(10, 10);
    for (long r = 0; r < 10; r++) {
        for (long c = 0; c < 10; c++) {
            mat[r][c] = NTL::to_ZZ(r * 10 + c);
        }
    }

    ppe::EncMat encMat(pk);
    encMat.pack(mat);
    {
        Matrix mat2;
        encMat.unpack(mat2, sk);
        NTL::ZZ_p::init(P);
        NTL::mat_ZZ_p computed, groud_truth;
        NTL::conv(groud_truth, mat);
        NTL::conv(computed, mat2);
        if (groud_truth != computed)
            return -1;
    } // test pack & unpack
    {
        ppe::EncMat encMat2(encMat);
        encMat2.add(mat);
        Matrix mat2;
        encMat2.unpack(mat2, sk);

        NTL::ZZ_p::init(P);
        NTL::mat_ZZ_p computed, groud_truth;
        NTL::conv(groud_truth, mat);
        groud_truth *= 2;

        NTL::conv(computed, mat2);
        if (groud_truth != computed)
            return -1;

        ppe::EncMat encMat3(encMat);
        encMat3.add(encMat);
        encMat3.unpack(mat2, sk);
        NTL::conv(computed, mat2);
        if (groud_truth != computed)
            return -1;
    } // test add
    {
        ppe::EncMat encMat2(encMat);
        encMat2.sub(encMat);
        Matrix mat2;
        encMat2.unpack(mat2, sk);
        for (long r = 0; r < mat2.NumRows(); r++)
            for (long c = 0; c < mat2.NumCols(); c++)
                if (mat2[r][c] != 0) return -1;

        ppe::EncMat encMat3(encMat);
        encMat3.sub(mat);
        encMat2.unpack(mat2, sk);
        for (long r = 0; r < mat2.NumRows(); r++)
            for (long c = 0; c < mat2.NumCols(); c++)
                if (mat2[r][c] != 0) return -1;
    } // test sub
    return 0;
}

int main() {
    if (test_EncVec() != 0)
        return -1;
    // if (test_io() != 0)
    //     return -1;
    if (test_EncMat() != 0)
        return -1;
    std::cout << "passed\n";
    return 0;
}

#include "core/PPE/Context.hpp"
#include "core/PPE/PubKey.hpp"
#include "core/PPE/SecKey.hpp"
#include "core/PPE/EncVec.hpp"

#include <iostream>
#include <fstream>

int test_algebra() {
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
    Vector vec2;
    encVec.unpack(vec2, sk);
    if (vec != vec2)
        return -1;
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

int main() {
    if (test_algebra() != 0)
        return -1;
    // if (test_io() != 0)
    //     return -1;
    std::cout << "passed\n";
    return 0;
}

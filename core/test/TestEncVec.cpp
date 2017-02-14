#include "core/algebra/EncVec.hpp"
#include "core/algebra/EncMat.hpp"
#include "HElib/FHE.h"
#include "HElib/EncryptedArray.h"
#include "core/coda.hpp"
#include <NTL/vector.h>
#include <NTL/mat_ZZ.h>
int test1(core::sk_ptr sk, core::pk_ptr pk, const EncryptedArray *ea) {
    core::EncVec encVec(pk);
    NTL::vec_ZZ slots;
    slots.SetLength(7);
    for (int i = 0; i < slots.length(); i++)
        slots[i] = NTL::to_ZZ(i + 1);
    encVec.pack(slots);

    long width = 4;
    long idx = 4;
    auto tmp = encVec.replicate(idx, width);
    NTL::vec_ZZ _slots;
    tmp.unpack(_slots, sk, true); // [5, 5, 5, 5]
    if (_slots.length() != width)
        return -1;
    for (auto t : _slots) {
        if (t != slots[idx]) return -1;
    }

    core::EncMat encMat(pk);
    NTL::mat_ZZ matrix;
    matrix.SetDims(3, 4);
    for (int r = 0; r < matrix.NumRows(); r++)
        for (int c = 0; c < matrix.NumCols(); c++)
            matrix.put(r, c, NTL::to_ZZ(1));
    encMat.pack(matrix);

    core::EncMat encMat2(pk);
    NTL::mat_ZZ matrix2;
    matrix2.SetDims(4, 5);
    for (int r = 0; r < matrix2.NumRows(); r++)
        for (int c = 0; c < matrix2.NumCols(); c++)
            matrix2.put(r, c, NTL::to_ZZ(1));
    encMat2.pack(matrix2);

    encMat.dot(encMat2);
    NTL::mat_ZZ _mat;
    encMat.unpack(_mat, sk, false);
    NTL::mat_ZZ zM, zM2, zM3;
    matrix *= matrix2;
    if (matrix != _mat)
        return -1;
    return 0;
}

int test2(core::sk_ptr sk, core::pk_ptr pk, const EncryptedArray *ea) {
    long p = pk->getContext().alMod.getPPowR();
    NTL::mat_ZZ X;
    X.SetDims(4, 4);
    NTL::ZZ range = NTL::to_ZZ(3);
    for (int r = 0; r < X.NumRows(); r++) {
        for (int c = r; c < X.NumCols(); c++) {
            auto rnd = NTL::RandomBnd(range);
            X.put(r, c, rnd);
            X.put(c, r, rnd);
        }
    }

    NTL::vec_ZZ v;
    v.SetLength(4);
    for (int c = 0; c < v.length(); c++)
        v[c] = NTL::RandomBnd(range);

    core::EncMat encMat(pk);
    core::EncVec encVec(pk);
    encMat.pack(X);
    encVec.pack(v);

    NTL::ZZ_p::init(NTL::to_ZZ(p));
    NTL::mat_ZZ_p Xp;
    NTL::vec_ZZ_p Vp;
    NTL::conv(Xp, X);
    NTL::conv(Vp, v);
    long T = 4;
    for (int t = 0; t < T; t++) {
        encVec = encMat.sym_dot(encVec); // only works for symmetric
        NTL::mul(Vp, Xp, Vp);
    }

    NTL::vec_ZZ_p modV;
    encVec.unpack(v, sk);
    NTL::conv(modV, v);
    if (modV != Vp)
        return -1;
    return 0;
}

int test3(core::sk_ptr sk, core::pk_ptr pk, const EncryptedArray *ea) {
    long p = pk->getContext().alMod.getPPowR();
    NTL::mat_ZZ X, Y;
    X.SetDims(4, 4);
    Y.SetDims(4, 4);
    NTL::ZZ range = NTL::to_ZZ(3);
    for (int r = 0; r < X.NumRows(); r++) {
        for (int c = 0; c < X.NumCols(); c++) {
            X.put(r, c, NTL::RandomBnd(range));
            Y.put(r, c, NTL::RandomBnd(range));
        }
    }

    NTL::ZZ_p::init(NTL::to_ZZ(p));
    NTL::mat_ZZ_p Xp, Yp;
    NTL::conv(Xp, X);
    NTL::conv(Yp, Y);

    core::EncMat encMat(pk), encMat2(pk);
    encMat.pack(X);
    encMat2.pack(Y);

    long T = 4;
    for (int t = 0; t < T; t++) {
        encMat.dot(encMat2);
        NTL::mul(Xp, Xp, Yp);
    }
    encMat.unpack(X, sk);
    NTL::conv(Yp, X);
    if (Xp != Yp)
        return -1;
    return 0;
}

int testIO(core::sk_ptr sk, core::pk_ptr pk, const EncryptedArray *ea) {
    core::EncVec encVec(pk);
    NTL::vec_ZZ slots;
    slots.SetLength(7);
    for (int i = 0; i < slots.length(); i++)
        slots[i] = NTL::to_ZZ(i + 1);
    encVec.pack(slots);
    {
        std::ofstream out("./tmp.ctx", std::ios::binary);
        if (!encVec.dump(out)) {
            out.close();
            return -1;
        }
        out.close();

        std::ifstream in("./tmp.ctx", std::ios::binary);
        core::EncVec encVec2(pk);
        if (!encVec2.restore(in)) {
            in.close();
            return -1;
        }
        in.close();

        NTL::vec_ZZ slots2;
        encVec2.unpack(slots2, sk);
        if (slots2 != slots)
            return -1;
    } // Test EncVec

    core::EncMat encMat(pk);
    NTL::mat_ZZ mat;
    mat.SetDims(7, 7);
    for (long r = 0; r < mat.NumRows(); r++)
        for (long c = 0; c < mat.NumCols(); c++) mat[r][c] = 1;
    encMat.pack(mat);
    {
        std::ofstream out("./tmp.ctx", std::ios::binary);
        if (!encMat.dump(out)) {
            out.close();
            return -1;
        }
        out.close();

        std::ifstream in("./tmp.ctx", std::ios::binary);
        core::EncMat encMat2(pk);
        if (!encMat2.restore(in)) {
            in.close();
            return -1;
        }
        in.close();

        NTL::mat_ZZ mat2;
        encMat2.unpack(mat2, sk);
        if (mat != mat2)
            return -1;
    } // Test EncMat
    return 0;
}

int main() {
    core::context_ptr context = std::make_shared<FHEcontext>(2048, 8191, 1);
    buildModChain(*context, 14);
    core::sk_ptr sk = std::make_shared<FHESecKey>(*context);
    sk->GenSecKey(64);
    addSome1DMatrices(*sk);
    core::pk_ptr pk = std::make_shared<FHEPubKey>(*sk);
    auto ea = context->ea;
    if (test1(sk, pk, ea) != 0) {
        std::cout << "test1 fail\n";
        return -1;
    }
    if (test2(sk, pk, ea) != 0) {
        std::cout << "test2 fail\n";
        return -1;
    }
    if (test3(sk, pk, ea) != 0) {
        std::cout << "test3 fail\n";
        return -1;
    }
    if (testIO(sk, pk, ea) != 0) {
        std::cout << "test3 fail\n";
        return -1;
    }
    std::cout << "passed\n";
    return 0;
}

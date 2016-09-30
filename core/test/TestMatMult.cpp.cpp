//
// Created by riku on 2016/09/30.
//
#include "HElib/EncryptedArray.h"
#include "HElib/FHE.h"
#include "HElib/NumbTh.h"
#include "HElib/replicate.h"
#include "HElib/timing.h"
#include <vector>
typedef std::vector<long> Vec_t;
typedef std::vector<Vec_t> Mat_t;
typedef std::vector<Ctxt> EMat_t;

Mat_t MatMult(Mat_t &a, Mat_t b) {
    size_t d = a.size();
    Mat_t res(d, Vec_t(d));
    for (size_t i = 0; i < d; i++) {
        for (size_t j = 0; j < d; j++) {
            long sum = 0;
            for (size_t k = 0; k < d; k++) {
                sum += a[i][k] * b[k][j];
            }
            res[i][j] = sum;
        }
    }
    return res;
}

class StopReplicate {};

class MatMultHandler : public ReplicateHandler {
public:
    MatMultHandler(const FHEPubKey &pk, const EMat_t &b)
            : res(pk), b(b)
    {
        res.clear();
    }

    void handle(const Ctxt& ctxt) override {
        if (pos >= bound) {
            res.reLinearize();
            throw StopReplicate();
        }
        auto tmp(ctxt);
        tmp *= b.at(pos);
        if (pos > 0)
            res += tmp;
        else
            res = tmp;
        pos++;
    }

    void setBound(long b) { if (b >= 0) bound = b; }
    void reset() { pos = 0; res.clear(); }
    Ctxt result() { return res; }

private:
    Ctxt res;
    const EMat_t &b;
    long bound = 0;
    long pos = 0;
};

#ifdef FHE_THREADS
#include "HElib/multicore.h"
EMat_t MatMult(const EMat_t &a, const EMat_t &b,
               long dim, const FHEcontext &context) {
    MultiTask pool(8);
    EMat_t res;
    std::mutex lock;
    pool.exec1(dim, [&](long first, long last) {
        for (long i = first; i < last; ++i) {
            Ctxt sum(pk);
            for (long j = 0; j < dim; j++) {
                Ctxt tmp(a.at(i));
                replicate(*ea, tmp, j, dim);
                tmp *= b.att(j);
                sum += tmp;
            }
            sum.reLinearize();

            lock.lock();
            res.emplace_back(sum);
            lock.unlock();
        }
    });
    return res;
}

#else
EMat_t MatMult(const EMat_t &a, const EMat_t &b,
               long dim, const FHEcontext &context) {
    auto ea = context.ea;
    const FHEPubKey& pk = a.front().getPubKey();
    if (dim <= 0 || dim > ea->size()) dim = ea->size();

    EMat_t res;
    res.reserve(dim);
    for (size_t i = 0; i < dim; i++) {
        Ctxt sum(pk);
        for (size_t j = 0; j < dim; j++) {
            Ctxt tmp(a.at(i));
            replicate(*ea, tmp, j, dim);
            tmp *= b.at(j);
            sum += tmp;
        }
        sum.reLinearize();
        res.emplace_back(sum);
    }
    return res;
}
#endif

void random(const long sze, const EncryptedArray &ea, Mat_t &m) {
    m.resize(sze);
    for (size_t i = 0; i < sze; i++)
        m.at(i).resize(ea.size());
    for (auto &row : m) {
        for (size_t i = 0; i < sze; i++)
            row.at(i) = NTL::RandomBnd(10);
        for (size_t i = sze; i < ea.size(); i++)
            row.at(i) = 0;
    }
}

void print(const Mat_t &a, int dim) {
    std::cout << "[\n";
    for (int r = 0; r < dim; ++r) {
        std::cout << "[ ";
        for (int i = 0; i < dim; i++)
            std::cout << a.at(r).at(i) << " ";
        std::cout << "];\n";
    }
    std::cout << "]\n";
}

Mat_t decrypt(const EMat_t &emat, int dim, const FHESecKey &sk) {
    auto ea = sk.getContext().ea;
    Mat_t mat(ea->size());
    auto sze = emat.size();
    std::vector<long> position(dim);
    for (int i = 0; i < dim; ++i) position[i] = i;
    NTL::ZZX poly;
    for (size_t i = 0; i < sze; i++) {
        ea->decrypt(emat.at(i), sk, mat.at(i));
        //sk.Decrypt(poly, emat.at(i));
        //ea->decodeSlots(mat.at(i), poly, position);
    }
    return mat;
}

int main(int argc, char *argv[]) {
    long m = 1031, p = 2, r = 1, L = 6;
    long dim = 0;
    ArgMapping amap;
    amap.arg("m", m, "m");
    amap.arg("p", p, "p");
    amap.arg("r", r, "r");
    amap.arg("L", L, "L");
    amap.arg("D", dim, "dimension");
    amap.parse(argc, argv);

    FHEcontext context(m, p, r);
    buildModChain(context, L);
    FHESecKey sk(context);
    sk.GenSecKey(64);
    addSome1DMatrices(sk);
    FHEPubKey pk = sk;
    EncryptedArray ea(context);

    if (dim <= 0 || dim > ea.size()) dim = ea.size();
    EMat_t emat(dim, pk);

    Mat_t mat(dim);
    random(dim, ea, mat);
    print(mat, dim);
    for (long i = 0; i < dim; i++) {
        ea.encrypt(emat.at(i), pk, mat.at(i));
    }

    FHE_NTIMER_START(MatMult);
    auto m1 = MatMult(emat, emat, dim, context);
    FHE_NTIMER_STOP(MatMult);

    FHE_NTIMER_START(decrypt);
    auto result = decrypt(m1, dim, sk);
    FHE_NTIMER_STOP(decrypt);
    print(result, dim);
//
    printNamedTimer(std::cout, "MatMult");
    printNamedTimer(std::cout, "decrypt");
    return 0;
}
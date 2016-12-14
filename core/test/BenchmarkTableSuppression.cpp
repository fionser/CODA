#include "../include/HElib/FHE.h"
#include "../include/HElib/NumbTh.h"
#include "cryptopp/hrtimer.h"

#include <fstream>
#include <memory>
class EncryptedTable {
public:
    EncryptedTable(size_t table_size) : table_size_(table_size) {}

    ~EncryptedTable() {}

    bool add(const EncryptedTable &oth) {
        if (table_size_ != oth.table_size_) return false;
        if (parts_.size() != oth.parts_.size()) return false;
        for (size_t i = 0; i < parts_.size(); i++)
            parts_.at(i)->operator+=(*(oth.parts_.at(i)));
        return true;
    }

    bool randomize(const FHEPubKey &pk) {
        if (parts_.empty() || table_size_ == 0)
            return false;
        const long pr = pk.getContext().alMod.getPPowR();
        const long phim = pk.getContext().zMStar.getPhiM();
        // clear up random_shares_
        std::vector<std::vector<long>> (parts_.size(), std::vector<long>()).swap(random_shares_);
        size_t next = 0;
        for (size_t i = 0; i < parts_.size(); i++) {
           random_shares_[i];
           NTL::ZZX poly;
           poly.SetLength(phim);
           for (long j = 0; j < phim && next < table_size_; ++j, ++next) {
               long rnd = half_domain(NTL::RandomBnd(pr), pr);
               NTL::SetCoeff(poly, j, rnd);
               random_shares_[i].push_back(rnd);
           }
           parts_[i]->addConstant(poly);
        }
        return true;
    }

    bool encrypt(const std::vector<long>& table, const FHEPubKey &pk) {
        long m = pk.getContext().zMStar.getM();
        long phim = pk.getContext().zMStar.getPhiM();
        if ((phim << 1) != m) return false;

        long parts_need = (table.size() + phim - 1) / phim;
        for (long i = 0; i < parts_need; ++i) {
            NTL::ZZX poly;
            poly.SetLength(phim);
            long j = i * phim;
            for (long k = 0; k < phim && j + k < table.size(); k++) {
                NTL::SetCoeff(poly, k, table.at(j + k));
            }

            if (i >= parts_.size()) {
                auto part = std::make_shared<Ctxt>(pk);
                pk.Encrypt(*part, poly);
                parts_.push_back(part);
            } else {
                pk.Encrypt(*(parts_.at(i)), poly);
            }
        }

        return true;
    }

    const std::vector<long> decrypt(const FHESecKey &sk) {
        const long pr = sk.getContext().alMod.getPPowR();
        std::vector<long> table(table_size_, 0);
        NTL::ZZX poly;
        size_t next = 0;

        for (auto &part : parts_) {
            sk.Decrypt(poly, *part);
            const long degree =  NTL::deg(poly);
            for (long i = 0; i <= degree; i++) {
                if (next >= table_size_) break;
                table.at(next++) = half_domain(NTL::to_long(poly[i]), pr);
            }
        }
        return table;
    }

    bool dump(const std::string &file) const {
        std::ofstream fout(file, std::ios::binary);
        if (!fout.is_open())
            return false;
        fout << parts_.size();
        for (const auto &part : parts_)
            fout << *part;
        fout.close();
        return true;
    }

    bool restore(const std::string &file, const FHEPubKey &pk) {
        std::ifstream fin(file, std::ios::binary);
        if (!fin.is_open())
            return false;
        size_t parts;
        fin >> parts;
        for (size_t i = 0; i < parts; i++) {
            auto part = std::make_shared<Ctxt>(pk);
            fin >> *part;
            parts_.push_back(part);
        }
        fin.close();
        return true;
    }

private:
    long half_domain(long val, long pr) const {
        long half = pr >> 1;
        if (val >= half)
            return val - pr;
        return val;
    }
private:
    std::vector<std::shared_ptr<Ctxt>> parts_;
    std::vector<std::vector<long>> random_shares_;
    size_t table_size_;
};

int main(int argc, char *argv[]) {
    long m, p, r = 1, L;

    ArgMapping mapping;
    mapping.arg("m", m, "m");
    mapping.arg("p", p, "p");
    mapping.arg("r", r, "r");
    mapping.arg("L", L, "L");
    mapping.parse(argc, argv);

    FHEcontext context(m, p, r);
    buildModChain(context, L);
    FHESecKey sk(context);
    sk.GenSecKey(64);
    const FHEPubKey &pk = sk;
    auto ea = context.ea;

    std::vector<long> table = { 1, 2, 3, 4, 5, 6, 7,8,9,10,11};
    // std::vector<long> table2 = {2, 3, 4, 5, 6, 7,8,9,10,11, 0};
    {
        EncryptedTable enc_table(11);
        enc_table.encrypt(table, pk);
        enc_table.randomize(pk);
        enc_table.dump("encrypted_table.ctx");
        // enc_table.encrypt(table2, pk);
        // enc_table.dump("encrypted_table2.ctx");
    }
    {
        EncryptedTable enc_table(11), enc_table2(11);
        enc_table.restore("encrypted_table.ctx", pk);
        // enc_table2.restore("encrypted_table2.ctx", pk);
        // enc_table.add(enc_table2);
        auto table = enc_table.decrypt(sk);
    }
    return 0;
}


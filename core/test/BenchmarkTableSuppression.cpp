#include "../include/HElib/FHE.h"
#include "../include/HElib/NumbTh.h"
#include "../include/HElib/EncryptedArray.h"
#include "cryptopp/hrtimer.h"

#include <fstream>
#include <memory>
#include <cassert>
static long half_domain(long val, long pr) {
    long half = pr >> 1;
    if (val >= half)
        return val - pr;
    return val;
}

struct OneRow {
    std::vector<long> bits_;
};

class EncryptedRow {
public:
    EncryptedRow() {}

    ~EncryptedRow() {}

    EncryptedRow(const EncryptedRow &oth) {
        if (oth.parts_.empty())
            return;
        parts_.clear();
        for (size_t i = 0; i < oth.parts_.size(); ++i) {
            auto part = std::make_shared<Ctxt>(*(oth.parts_[i]));
            parts_.push_back(part);
        }
    }

    EncryptedRow& operator=(const EncryptedRow &oth) {
        if (oth.parts_.empty())
            return *this;
        parts_.clear();
        for (size_t i = 0; i < oth.parts_.size(); ++i) {
            auto part = std::make_shared<Ctxt>(*(oth.parts_[i]));
            parts_.push_back(part);
        }
        return *this;
    }

    EncryptedRow& operator*=(const long c) {
        if (parts_.empty()) {
            std::cerr << "Empty EncryptedRow\n";
            return *this;
        }
        for (auto &part : parts_)
            part->multByConstant(NTL::to_ZZ(c));
        return *this;
    }

    EncryptedRow& operator+=(const EncryptedRow &b) {
        if (b.parts_.empty())
            return *this;

        if (parts_.empty()) {
            *this = b;
            return *this;
        }

        if (parts_.size() != b.parts_.size()) {
            std::cerr << "Mismatch adding size: " << parts_.size() << " " << b.parts_.size() << "\n";
            return *this;
        }

        for (size_t i = 0; i < parts_.size(); ++i)
            parts_[i]->operator+= (*(b.parts_[i]));
        return *this;
    }

    EncryptedRow& operator+=(const OneRow &y) {
        if (parts_.empty()) {
            std::cerr << "Not implement yet.\n";
            return *this;
        }

        const EncryptedArray *ea = parts_.front()->getPubKey().getContext().ea;
        apply_to_each_part(y, [ea](std::shared_ptr<Ctxt> Xs, const std::vector<long> &Ys) {
                           NTL::ZZX poly;
                           ea->encode(poly, Ys);
                           Xs->addConstant(poly);
                           });
        return *this;
    }

    EncryptedRow& operator^=(const OneRow &y) {
        apply_to_each_part(y, [this](std::shared_ptr<Ctxt> Xs, const std::vector<long> &Ys) {
                           XOR(Xs, Ys);
                           } );
        return *this;
    }

    long get_pr() const {
        if (parts_.empty())
            return 0;
        return parts_.front()->getPubKey().getContext().alMod.getPPowR();
    }

    std::vector<std::shared_ptr<Ctxt>> parts_;

private:
    typedef std::function<void (std::shared_ptr<Ctxt>, const std::vector<long> &)> PartHandler;

    void apply_to_each_part(const OneRow &y, PartHandler fn) {
        if (parts_.empty())
            return;
        const EncryptedArray *ea = parts_.front()->getPubKey().getContext().ea;
        const long slots = ea->size();
        const long parts_need = (y.bits_.size() + slots - 1) / slots;
        if (parts_need != parts_.size())
            return;
        auto itr = y.bits_.cbegin();
        auto end_point = y.bits_.cend();
        for (long p = 0; p < parts_need; ++p) {
            std::vector<long> bits(ea->size(), 0);
            auto itr2 = bits.begin();
            while (itr2 != bits.end() && itr != end_point) {
                *itr2 ++ = *itr++;
            }
            fn(parts_.at(p), bits);
        }
    }

    void XOR(std::shared_ptr<Ctxt> Xs, const std::vector<long> &Ys) const {
        const EncryptedArray *ea = Xs->getPubKey().getContext().ea;
        if (std::find(Ys.begin(), Ys.end(), 1) == Ys.end())
            return;
        std::vector<long> one(ea->size(), 0);
        for (size_t i = 0; i < Ys.size() && i < ea->size(); i++)
           one[i] = Ys[i] == 1 ? 1 : 0;
        NTL::ZZX masking;
        ea->encode(masking, one);

        Ctxt origin(*Xs);
        Xs->addConstant(masking);
        origin.multByConstant(masking * 2);
        Xs->operator-=(origin);
    }
};

std::vector<OneRow> binary_decompose(const std::vector<long> &table, const long how_many_bits) {
    std::vector<OneRow> rows;
    if (how_many_bits <= 0)
        return rows;
    auto get_bit = [](long v, long b) -> long {
        return (v >> b) & 1;
    };

    for (long b = 0; b < how_many_bits; b++) {
        OneRow row = { .bits_ = std::vector<long>(table.size()) };
        for (size_t i = 0; i < row.bits_.size(); i++) {
            row.bits_[i] = get_bit(table[i], b);
        }
        rows.push_back(row);
    }
    // the most significant bit comes first
    std::reverse(rows.begin(), rows.end());
    return rows;
}

class EncryptedTableSuppression {
public:
    EncryptedTableSuppression(size_t table_size, long threshold) : table_size_(table_size), threshold_(threshold) {}

    ~EncryptedTableSuppression() {}

    bool encrypt(const std::vector<long> &table,
                 const long how_many_bits,
                 const FHEPubKey &pk) {
        if (how_many_bits <= 0)
            return false;

        auto rows = binary_decompose(table, how_many_bits);
        for (auto& row : rows) {
            auto enc_row = encrypt_one_row(row, pk);
            rows_.push_back(enc_row);
        }
        return true;
    }

    std::vector<OneRow> decrypt(const long table_size, const FHESecKey &sk) const {
        std::vector<OneRow> rows;
        for (auto &row : rows_) {
            rows.push_back(decrypt_one_row(row, table_size, sk));
        }
        return rows;
    }

    long get_pr() const {
        if (rows_.empty()) return 0;
        return rows_.front().get_pr();
    }

    void compare(const std::vector<long> &random_share) {
        if (rows_.empty())
            return;
        const long pr = get_pr();
        auto Ys = binary_decompose(random_share, rows_.size());
        EncryptedRow accumulated; // sum_{j < i} x_i xor y_i
        const size_t N = Ys.front().bits_.size();
        for (size_t i = 0; i < Ys.size(); i++) {
            EncryptedRow Xi(rows_[i]);
            OneRow minus_y_minus_one = Ys[i];
            for (auto &y : minus_y_minus_one.bits_)
                y = y == 1 ? -2 : -1;
            rows_[i] += minus_y_minus_one; // x_i - y_i - 1
            rows_[i] += accumulated;
            // rows_[i] *= NTL::RandomBnd(pr);

            Xi ^= Ys[i];
            Xi *= 3;
            accumulated += Xi;
        }
    }

private:
    OneRow decrypt_one_row(const EncryptedRow &row, const long table_size, const FHESecKey &sk) const {
        const EncryptedArray *ea = sk.getContext().ea;
        const long pr = sk.getContext().alMod.getPPowR();
        OneRow ret;

        NTL::ZZX poly;
        long slots_decoded = 0;
        for (auto &p : row.parts_) {
            sk.Decrypt(poly, *p);
            long slots_to_decode = std::min(table_size - slots_decoded, ea->size());
            std::vector<long> positions(slots_to_decode, 0);
            for (long s = 0; s < slots_to_decode; s++)
                positions[s] = s;
            std::vector<long> slots;
            ea->decodeSlots(slots, poly, positions);
            for (auto &slot : slots)
                slot = half_domain(slot, pr);
            ret.bits_.insert(ret.bits_.end(), slots.begin(), slots.end());
        }
        return ret;
    }

    EncryptedRow encrypt_one_row(const OneRow &row, const FHEPubKey &pk) const {
        const EncryptedArray *ea = pk.getContext().ea;
        const long slots = ea->size();
        const long parts_need = (row.bits_.size() + slots - 1) / slots;
        auto itr = row.bits_.cbegin();
        auto end_point = row.bits_.cend();
        EncryptedRow ret;
        for (long i = 0; i < parts_need; i++) {
            std::vector<long> poly(slots, 0);
            auto itr2 = poly.begin();
            while (itr2 != poly.end() && itr != end_point) {
                *itr2++ = *itr++;
            }
            auto part = std::make_shared<Ctxt>(pk);
            ea->encrypt(*part, pk, poly);
            ret.parts_.push_back(part);
        }
        return ret;
    }

private:
    std::vector<EncryptedRow> rows_;
    size_t table_size_;
    long threshold_;
};

class EncryptedTable {
public:
    EncryptedTable(size_t table_size) : table_size_(table_size) {}

    EncryptedTable(const EncryptedTable &oth) : table_size_(oth.table_size_) {
        if (oth.parts_.empty())
            return;
        for (const auto &part : oth.parts_) {
            auto p = std::make_shared<Ctxt>(*part);
            parts_.push_back(p);
        }
    }

    EncryptedTable& operator=(const EncryptedTable &oth) {
        if (oth.parts_.empty())
            return *this;
        for (const auto &part : oth.parts_) {
            auto p = std::make_shared<Ctxt>(*part);
            parts_.push_back(p);
        }
        return *this;
    }

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
        const long pr = pk.getContext().alMod.getPPowR() >> 1;
        const long phim = pk.getContext().zMStar.getPhiM();
        // clear up random_shares_
        std::vector<std::vector<long>> (parts_.size(), std::vector<long>()).swap(random_shares_);
        size_t next = 0;
        for (size_t i = 0; i < parts_.size(); i++) {
           random_shares_[i];
           NTL::ZZX poly;
           poly.SetLength(phim);
           for (long j = 0; j < phim && next < table_size_; ++j, ++next) {
               //long rnd = half_domain(NTL::RandomBnd(pr), pr);
               long rnd = NTL::RandomBnd(pr);
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
                // table.at(next++) = NTL::to_long(poly[i]);
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
        if (!fin.is_open()) {
            std::cout << "Can not open " << file << "\n";
            return false;
        }
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
    private:
    std::vector<std::shared_ptr<Ctxt>> parts_;
    std::vector<std::vector<long>> random_shares_;
    size_t table_size_;
};

static void print(const std::vector<OneRow> &rows, const long table_size, long first = -1) {
    // if (rows.front()size() >= 50) return;
    std::vector<bool> greater(rows.front().bits_.size(), false);
    for (auto &row : rows) {
        for (size_t i = 0; i < row.bits_.size() && i < table_size; ++i) {
            if (row.bits_[i] == 0) {
                if (greater[i])
                    std::cerr << "more than one zero!\n";
                greater[i] = true;
            }
        }
    }
    if (first <= 0)
        first = greater.size();
    for (long i = 0; i < first; ++i)
        std::cout << greater[i] << " ";
    std::cout << "\n";
}

int main(int argc, char *argv[]) {
    long m, p, r = 1, L;
    long table_size = 10;
    long parties_nr = 10;
    ArgMapping mapping;
    mapping.arg("m", m, "m");
    mapping.arg("p", p, "p");
    mapping.arg("r", r, "r");
    mapping.arg("L", L, "L");
    mapping.arg("P", parties_nr, "P");
    mapping.arg("S", table_size, "Table Size");
    mapping.parse(argc, argv);

    FHEcontext context(m, p, r);
    buildModChain(context, L);
    FHESecKey sk(context);
    sk.GenSecKey(64);
    const FHEPubKey &pk = sk;
    auto ea = context.ea;
    printf("slots %ld kappa %f\n", ea->size(), context.securityLevel());

    CryptoPP::Timer timer;
    timer.StartTimer();
    std::vector<long> table(table_size, 1);
    for (int i = 0; i < table_size; i++)
        table[i] = i + 1;

    // EncryptedTableSuppression suppression(table_size, 10);
    // timer.StartTimer();
    // suppression.encrypt(table, 8, pk);
    // std::cout << "Encryption for suppression " << timer.ElapsedTimeAsDouble() << "\n";
    //
    // timer.StartTimer();
    // std::vector<long> threshold(table_size, 11);
    // suppression.compare(threshold);
    // std::cout << "Suppression " << timer.ElapsedTimeAsDouble() << "\n";
    //
    // timer.StartTimer();
    // auto rows = suppression.decrypt(table_size, sk);
    // std::cout << "Decryption for suppression " << timer.ElapsedTimeAsDouble() << "\n";
    // print(rows, table_size, 15);

    for (long t = 0; t < parties_nr; ++t) {
        EncryptedTable enc_table(table.size());
        enc_table.encrypt(table, pk);
        std::stringstream builder;
        builder << "enc_table/encrypted_table" << t << ".ctx";
        enc_table.dump(builder.str());
    }
    std::cout << "Encryption & Dump " << timer.ElapsedTimeAsDouble() << "\n";

    timer.StartTimer();
    EncryptedTable result(table_size);
    result.restore("enc_table/encrypted_table0.ctx", pk);
    for (long tt = 1; tt < parties_nr; ++tt) {
        EncryptedTable enc_table(table_size);
        std::stringstream builder;
        builder << "enc_table/encrypted_table" << tt << ".ctx";
        enc_table.restore(builder.str(), pk);
        result.add(enc_table);
    }
    std::cout << "Restore & Add " << timer.ElapsedTimeAsDouble() << "\n";

    auto dec = result.decrypt(sk);
    std::cout << dec.front() << " " << dec.back() << "\n";
    return 0;
}


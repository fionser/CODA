#include "../include/HElib/FHE.h"
#include "../include/HElib/NumbTh.h"
#include "../include/HElib/EncryptedArray.h"
#include "cryptopp/hrtimer.h"

#include <fstream>
#include <memory>
#include <cassert>
#include <string>
static long half_domain(long val, long pr) {
    long half = pr >> 1;
    if (val >= half)
        return val - pr;
    return val;
}

struct OneRow {
    std::vector<long> bits_;
};

struct RandomShares {
    std::vector<long> server_share;
    std::vector<long> client_share;
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

    EncryptedRow& operator+=(const std::vector<NTL::ZZX> &polys) {
        if (parts_.empty()) {
            std::cerr << "Not implement yet.\n";
            return *this;
        }
        if (polys.size() != parts_.size()) {
             std::cerr << "Mismatch: EncryptedRow& operator+=(const std::vector<NTL::ZZX> &polys)" << std::endl;
            return *this;
        }

        for (size_t i = 0; i < parts_.size(); ++i)
            parts_.at(i)->addConstant(polys.at(i));
        return *this;
    }

    EncryptedRow& operator*=(const std::vector<NTL::ZZX> &polys) {
        if (parts_.empty()) {
            std::cerr << "Not implement yet.\n";
            return *this;
        }
        if (polys.size() != parts_.size()) {
            std::cerr << "Mismatch: EncryptedRow& operator*=(const std::vector<NTL::ZZX> &polys)" << std::endl;
            return *this;
        }

        for (size_t i = 0; i < parts_.size(); ++i)
            parts_.at(i)->multByConstant(polys.at(i));
        return *this;
    }

    EncryptedRow& operator^=(const OneRow &y) {
        apply_to_each_part(y, [this](std::shared_ptr<Ctxt> Xs, const std::vector<long> &Ys) {
                           XOR(Xs, Ys);
                           } );
        return *this;
    }

    std::vector<NTL::ZZX> convert(const std::vector<long> &slots) {
        std::vector<NTL::ZZX> polys;
        OneRow one_row = { .bits_ = slots };
        apply_to_each_part(one_row, [&polys, this](std::shared_ptr<Ctxt> part, const std::vector<long> &partial_slots) {
                           NTL::ZZX poly;
                           const EncryptedArray *ea = part->getPubKey().getContext().ea;
                           ea->encode(poly, partial_slots);
                           polys.push_back(poly);
                           });
        return polys;
    }

    long get_pr() const {
        if (parts_.empty())
            return 0;
        return parts_.front()->getPubKey().getContext().alMod.getPPowR();
    }

    bool dump(std::ostream& out) const {
        out << parts_.size();
        for (auto &p : parts_)
            out << *p;
        return true;
    }

    bool restore(std::istream &in, const FHEPubKey &pk) {
        if (in.eof())
            return false;
        size_t parts;
        in >> parts;
        parts_.clear();
        for (size_t i = 0; i < parts; ++i) {
            auto part = std::make_shared<Ctxt>(pk);
            in >> *part;
            parts_.push_back(part);
        }
        return true;
    }

    std::vector<std::shared_ptr<Ctxt>> parts_;

private:
    typedef std::function<void (std::shared_ptr<Ctxt>, const std::vector<long> &)> PartHandler;

    void apply_to_each_part(const OneRow &y, PartHandler fn) {
        if (parts_.empty()) {
            std::cerr << "apply_to_each_part: parts_.empty()" << std::endl;
            return;
        }
        const EncryptedArray *ea = parts_.front()->getPubKey().getContext().ea;
        const long slots = ea->size();
        const long parts_need = (y.bits_.size() + slots - 1) / slots;
        if (parts_need != parts_.size()) {
            std::cerr << parts_need << "≠" << parts_.size() << "\n";
            return;
        }
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
    EncryptedTableSuppression(size_t table_size) : table_size_(table_size) {}

    ~EncryptedTableSuppression() {}

    bool encrypt(const std::vector<long> &table,
                 const long how_many_bits,
                 const FHEPubKey &pk) {
        if (how_many_bits <= 0)
            return false;
        rows_.clear();
        auto rows = binary_decompose(table, how_many_bits);
        for (const auto& row : rows) {
            auto enc_row = encrypt_one_row(row, pk);
            rows_.push_back(enc_row);
        }
        return true;
    }

    std::vector<OneRow> decrypt(const long table_size, const FHESecKey &sk) const {
        std::vector<OneRow> ret;
        auto ea = sk.getContext().ea;
        std::vector<long> slots;
        for (const auto &row : rows_) {
            OneRow onerow;
            size_t i = 0;
            onerow.bits_ = std::vector<long>(table_size, 0);
            for (auto &p : row.parts_) {
                ea->decrypt(*p, sk, slots);
                for (size_t k = 0; k < ea->size() && i < table_size; k++, i++)
                    onerow.bits_[i] = slots[k];
            }
            ret.push_back(onerow);
        }
        return ret;
    }

    std::vector<long> open(const long table_size, const FHESecKey &sk) const {
        std::vector<long> table(table_size, 0);
        auto ea = sk.getContext().ea;
        if (secrets_.size() != rows_.size()) {
            std::cerr << "Mismatch secrets_.size() and rows_.size()\n";
            return table;
        }

        for (size_t row = 0; row < rows_.size(); row++) {
            auto zero_positions = decrypt_one_row(rows_.at(row), table_size, sk);
            for (size_t p = 0; p < zero_positions.size(); p++) {
                if (zero_positions.at(p).empty()) // no decrypted 0 in this part
                    continue;
                if (secrets_.at(row).parts_.size() != zero_positions.size()) {
                    std::cerr << "Mismatch row parts size\n";
                    continue;
                }

                NTL::ZZX part;
                std::vector<long> server_shares;
                sk.Decrypt(part, *(secrets_.at(row).parts_.at(p)));
                ea->decodeSlots(server_shares, part, zero_positions.at(p));
                for (size_t j = 0; j < server_shares.size(); ++j) {
                    auto index = p * ea->size() + zero_positions[p][j];
                    if (index >= table_size) break;
                    table.at(index) = server_shares[j];
                }
            }
        }
        return table;
    }

    long get_pr() const {
        if (rows_.empty()) return 0;
        return rows_.front().get_pr();
    }

    bool compare(const std::vector<long> &server_shares) {
        if (rows_.empty())
            return false;
        auto Ys = binary_decompose(server_shares, rows_.size());
        EncryptedRow accumulated; // sum_{j < i} x_j xor y_j
        const size_t N = Ys.front().bits_.size();
        for (size_t i = 0; i < Ys.size(); i++) {
            EncryptedRow Xi(rows_[i]);
            OneRow minus_y_minus_one = Ys[i];
            for (auto &y : minus_y_minus_one.bits_)
                y = y == 1 ? -2 : -1;
            rows_[i] += minus_y_minus_one; // x_i - y_i - 1
            rows_[i] += accumulated; // x_i - y_i - 1 + sum_{j < i} x_j xor y_j

            Xi ^= Ys[i];
            Xi *= 3;
            accumulated += Xi;
        }
        // std::random_shuffle(rows_.begin(), rows_.end());
        return hide_shares(server_shares);
    }

    bool hide_shares(const std::vector<long> &server_shares) {
        if (rows_.empty())
            return false;
        std::vector<NTL::ZZX> polys = rows_.front().convert(server_shares);
        long pr = get_pr();
        secrets_ = rows_;
        for (auto &s : secrets_) {
            std::vector<long> rnds(server_shares.size(), 0);
            for (auto &rnd : rnds) {
                while (rnd == 0) {
                    rnd = NTL::RandomBnd(pr);
                }
            }
            auto randomness = rows_.front().convert(rnds);
            s *= randomness;
            s += polys; // hide shares
        }

        for (auto &row : rows_) {
            std::vector<long> rnds(server_shares.size(), 0);
            for (auto &rnd : rnds) {
                while (rnd == 0) {
                    rnd = NTL::RandomBnd(pr);
                }
            }
            auto randomness = rows_.front().convert(rnds);
            row *= randomness;
        }

        return true;
    }

    bool dump(const std::string &file) const {
        std::ofstream fout(file, std::ios::binary);
        if (!fout.is_open())
            return false;
        fout << rows_.size() << " ";
        for (const auto &row : rows_)
            row.dump(fout);
        fout << secrets_.size() << " ";
        for (const auto &row : secrets_)
            row.dump(fout);
        fout.close();
        return true;
    }

    bool restore(const std::string &file, const FHEPubKey &pk) {
        std::ifstream fin(file, std::ios::binary);
        if (!fin.is_open()) {
            std::cout << "Can not open " << file << "\n";
            return false;
        }
        rows_.clear();
        secrets_.clear();
        size_t rows1 = 0;
        size_t rows2 = 0;
        fin >> rows1;
        for (size_t i = 0; i < rows1; i++) {
            EncryptedRow row;
            row.restore(fin, pk);
            rows_.push_back(row);
        }

        fin >> rows2;
        for (size_t i = 0; i < rows2; i++) {
            EncryptedRow row;
            row.restore(fin, pk);
            secrets_.push_back(row);
        }
        fin.close();
        return true;
    }


// private:
    std::vector<std::vector<long>> decrypt_one_row(const EncryptedRow &row,
                                                   const long table_size, const FHESecKey &sk) const {
        const EncryptedArray *ea = sk.getContext().ea;
        const long pr = sk.getContext().alMod.getPPowR();
        std::vector<std::vector<long>> zero_positions;
        NTL::ZZX poly;
        long slots_decoded = 0;
        for (size_t p = 0; p < row.parts_.size(); ++p) {
            const auto &part = row.parts_.at(p);
            if (part->log_of_ratio() > 0) {
                std::cerr << "Warning! the decryption might fail\n";
            }

            sk.Decrypt(poly, *part);
            long slots_to_decode = std::min(table_size - slots_decoded, ea->size());
            std::vector<long> positions(slots_to_decode, 0);
            for (long s = 0; s < slots_to_decode; s++)
                positions[s] = s;
            std::vector<long> slots;
            ea->decodeSlots(slots, poly, positions);
            std::vector<long> zposition;
            for (long ss = 0; ss < slots_to_decode; ss++) {
                if (slots.at(ss) == 0)
                    zposition.push_back(ss);
            }
            zero_positions.push_back(zposition);
            slots_decoded += slots_to_decode;
        }
        return zero_positions;
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
    std::vector<EncryptedRow> secrets_;
    size_t table_size_;
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
        if (parts_.empty()) {
            for (const auto &p : oth.parts_)
                parts_.push_back(std::make_shared<Ctxt>(*p));
            return true;
        }

        if (parts_.size() != oth.parts_.size()) return false;
        for (size_t i = 0; i < parts_.size(); i++)
            parts_.at(i)->operator+=(*(oth.parts_.at(i)));
        return true;
    }

    bool randomize(const FHEPubKey &pk) {
        if (parts_.empty() || table_size_ == 0)
            return false;
        const long pr = pk.getContext().alMod.getPPowR() >> 4;
        const long phim = pk.getContext().zMStar.getPhiM();
        // clear up random_shares_
        std::vector<long> ().swap(random_shares_);
        size_t next = 0;
        for (size_t i = 0; i < parts_.size(); i++) {
           random_shares_[i];
           NTL::ZZX poly;
           poly.SetLength(phim);
           for (long j = 0; j < phim && next < table_size_; ++j, ++next) {
               //long rnd = half_domain(NTL::RandomBnd(pr), pr);
               long rnd = NTL::RandomBnd(pr);
               NTL::SetCoeff(poly, j, rnd);
               random_shares_.push_back(rnd);
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

    bool dump_random_share(const std::string &file) const {
        std::ofstream fout(file, std::ios::binary);
        if (!fout.is_open())
            return false;
        fout << random_shares_.size();
        for (size_t i = 0; i < random_shares_.size(); i++)
            fout << " " << random_shares_[i];
        // fout.write(reinterpret_cast<const char *>(random_shares_.data()), sizeof(long) * random_shares_.size());
        return true;
    }

    std::vector<long> restore_random_share(const std::string &file) {
        std::ifstream fin(file, std::ios::binary);
        if (!fin.is_open()) {
            std::cout << "Can not open " << file << "\n";
            return  random_shares_;
        }

        long element_nr;
        fin >> element_nr;
        std::vector<long> (element_nr, 0).swap(random_shares_);
        for (long i = 0; i < element_nr; ++i)
            fin >> random_shares_[i];
        // for (long i = 0; i < element_nr; ++i) {
        //     fin.read(reinterpret_cast<char *>(&random_shares_[i]), sizeof(long));
        // }
        return random_shares_;
    }
private:
    std::vector<std::shared_ptr<Ctxt>> parts_;
    std::vector<long> random_shares_;
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

RandomShares poly_packing(long table_size, long parties_nr, long threshold) {
    FHEcontext context(8192, 1031, 3);
    buildModChain(context, 3);
    FHESecKey sk(context);
    sk.GenSecKey(64);
    const FHEPubKey &pk = sk;
    auto ea = context.ea;
    printf("slots %ld kappa %f\n", ea->size(), context.securityLevel());

    std::vector<long> table(table_size, 1);
    for (int i = 0; i < table_size; i++) {
        long coin = NTL::RandomBnd(100);
        table[i] = coin > 90 ? threshold + 1 : threshold - 1;
    }

    CryptoPP::Timer timer;
    timer.StartTimer();
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
    for (long tt = 0; tt < parties_nr; ++tt) {
        EncryptedTable enc_table(table_size);
        std::stringstream builder;
        builder << "enc_table/encrypted_table" << tt << ".ctx";
        enc_table.restore(builder.str(), pk);
        result.add(enc_table);
    }
    result.randomize(pk);
    result.dump("enc_table/randomized.ctx");
    result.dump_random_share("enc_table/random_share.txt");
    std::cout << "Restore & Add " << timer.ElapsedTimeAsDouble() << "\n";

    timer.StartTimer();
    EncryptedTable random_share_a(table_size);
    random_share_a.restore("enc_table/randomized.ctx", pk);
    auto client_share = random_share_a.decrypt(sk);
    EncryptedTable random_share_b(table_size);
    auto server_share = random_share_b.restore_random_share("enc_table/random_share.txt");
    std::cout << "Restore & Decrypt " << timer.ElapsedTimeAsDouble() << "\n";
    return { .server_share = server_share, .client_share = client_share };
}

void compare(RandomShares rnd_shares, long threshold) {
    long m = 16384;
    long p = 8191, r = 2;
    long L = 6;
    long how_many_bits = 16;

    CryptoPP::Timer timer;
    timer.StartTimer();
    FHEcontext context(m, p, r);
    buildModChain(context, L);

    FHESecKey sk(context);
    sk.GenSecKey(64);
    const FHEPubKey &pk = sk;
    auto ea = context.ea;
    printf("slots %ld kappa %f\n", ea->size(), context.securityLevel());
    std::cout << "Setup key " << timer.ElapsedTimeAsDouble() << "\n";
    {
        std::ofstream fout("enc_table/suppression_key.key");
        fout << pk;
        fout.close();
    }
    {
        timer.StartTimer();
        size_t table_size = rnd_shares.client_share.size();
        EncryptedTableSuppression client_share(table_size);
        client_share.encrypt(rnd_shares.client_share, how_many_bits, pk);
        client_share.dump("enc_table/client_share.ctx");
        std::cout << "Encrypt & Dump " << timer.ElapsedTimeAsDouble() << "\n";
    }

    timer.StartTimer();
    size_t table_size = rnd_shares.server_share.size();
    EncryptedTableSuppression client_share(table_size);
    client_share.restore("enc_table/client_share.ctx", pk);
    std::cout << "Load client share " << timer.ElapsedTimeAsDouble() << "\n";

    timer.StartTimer();
    for (auto &ss : rnd_shares.server_share)
        ss += threshold;
    client_share.compare(rnd_shares.server_share);
    client_share.dump("enc_table/suppression.ctx");
    std::cout << "Suppression " << timer.ElapsedTimeAsDouble() << "\n";

    timer.StartTimer();
    EncryptedTableSuppression suppressed(table_size);
    suppressed.restore("enc_table/suppression.ctx", pk);
    auto cs = suppressed.open(table_size, sk);
    std::cout << "Load result & Decrypt " << timer.ElapsedTimeAsDouble() << "\n";
    for (size_t i = 0; i < table_size; ++i) {
        if (cs[i] != 0) {
            cs[i] = rnd_shares.client_share[i] - cs[i]  + threshold;
        }
    }
    // std::cout << cs << "\n";
}

int main(int argc, char *argv[]) {
    long m, p, r = 1, L;
    long table_size = 10;
    long parties_nr = 5;
    ArgMapping mapping;
    mapping.arg("m", m, "m");
    mapping.arg("p", p, "p");
    mapping.arg("r", r, "r");
    mapping.arg("L", L, "L");
    mapping.arg("P", parties_nr, "P");
    mapping.arg("S", table_size, "Table Size");
    mapping.parse(argc, argv);

    long threshold = 33;
    auto random_shares = poly_packing(table_size, parties_nr, threshold);
    compare(random_shares, threshold);
    return 0;
}


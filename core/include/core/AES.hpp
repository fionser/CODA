//
// Created by riku on 2016/07/27.
//

#ifndef CODA_CLION_AES_HPP
#define CODA_CLION_AES_HPP
#include <openssl/aes.h>
#include <NTL/ZZ.h>
#include <vector>
#include <memory>
namespace core {
class AES128 {
public:
    class Ctxt : public std::vector<uint8_t> {
    public:
        friend class AES128;
        Ctxt(size_t length)
        : std::vector<uint8_t>(length),
          _length(0) { }
        Ctxt() { Ctxt::Ctxt(0); }
        ~Ctxt() {}
        size_t size() const { return _length;}
        void size(const size_t size) {_length = size;}
    private:
        size_t _length;
    } ;

    AES128(const NTL::ZZ& init_key);

    ~AES128() {}

    Ctxt encrypt(const std::string &message);

    std::string decrypt(const Ctxt &ctxt);

private:
    AES_KEY _aes;
    unsigned char _key[AES_BLOCK_SIZE];
};

}
#endif //CODA_CLION_AES_HPP

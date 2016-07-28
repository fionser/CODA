//
// Created by riku on 2016/07/27.
//

#ifndef CODA_CLION_AES_HPP
#define CODA_CLION_AES_HPP
#include <openssl/aes.h>
#include <NTL/ZZ.h>
#include <vector>
namespace core {
class AES128 {
public:
    typedef std::vector<uint8_t> Ctxt;
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

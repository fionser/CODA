//
// Created by riku on 2016/07/27.
//

#include "core/AES.hpp"
#include <openssl/evp.h>
#include <openssl/err.h>
#include <cstring>
#include <cassert>
#define _ALG EVP_aes_128_cbc
namespace core {
AES128::AES128(const NTL::ZZ &init_key)
{
    size_t nr_bytes = NTL::NumBytes(init_key);
    std::vector<uint8_t> bytes(nr_bytes);
    NTL::BytesFromZZ(bytes.data(), init_key, nr_bytes);

    std::memset(_key, 0, sizeof(_key));
    for (size_t i = 0; i < nr_bytes; i++)
        _key[i] = static_cast<unsigned char>(bytes.at(i));
}

AES128::Ctxt AES128::encrypt(const std::string &message) {
    unsigned char _iv[AES_BLOCK_SIZE];
    NTL::ZZ random;
    NTL::RandomBits(random, 128);
    NTL::BytesFromZZ(_iv, random, AES_BLOCK_SIZE);
    size_t length = message.size() + (AES_BLOCK_SIZE << 1); // store the iv in the prefix
    AES128::Ctxt ctxt(length);
    for (size_t i = 0; i < AES_BLOCK_SIZE; i++)
        ctxt.at(i) = _iv[i];

    EVP_CIPHER_CTX ctx;
    EVP_CIPHER_CTX_init(&ctx);
    if (1 != EVP_EncryptInit_ex(&ctx, _ALG(), NULL, _key, _iv))
        printf("some wrong in EVP_EncryptInit_ex\n");

    int len = 0;
    int ctxt_len = AES_BLOCK_SIZE;
    if (1 != EVP_EncryptUpdate(&ctx, ctxt.data() + ctxt_len, &len, (unsigned char *)message.data(), message.size()))
        printf("some wrong in EVP_EncryptUpdate\n");
    ctxt_len += len;

    len = 0;
    if (1 != EVP_EncryptFinal_ex(&ctx, ctxt.data() + ctxt_len, &len))
        printf("some wrong in EVP_EncryptFinal_ex\n");
    ctxt_len += len;

    EVP_CIPHER_CTX_cleanup(&ctx);

    ctxt.size(ctxt_len);
    return ctxt;
}

std::string AES128::decrypt(const AES128::Ctxt &ctxt) {
    unsigned char _iv[AES_BLOCK_SIZE];
    for (size_t i = 0; i < AES_BLOCK_SIZE; i++)
        _iv[i] = ctxt.at(i);

    EVP_CIPHER_CTX ctx;
    EVP_CIPHER_CTX_init(&ctx);
    if (1 != EVP_DecryptInit_ex(&ctx, _ALG(), NULL, _key, _iv))
        printf("some wrong in EVP_DecryptInit_ex\n");

    int len = 0;
    int plain_len = 0;
    std::vector<unsigned char> plain(ctxt.size());

    if (1 != EVP_DecryptUpdate(&ctx, plain.data(), &len,
                               ctxt.data() + AES_BLOCK_SIZE,
                               ctxt.size() - AES_BLOCK_SIZE))
        printf("some wrong in EVP_DecryptUpdate\n");
    plain_len = len;

    len = 0;
    if (1 != EVP_DecryptFinal_ex(&ctx, plain.data() + plain_len, &len)) {
        ;
        printf("some wrong in EVP_DecryptFinal_ex\n");
//        ERR_print_errors_fp(stdout);
    }
    plain_len += len;
    EVP_CIPHER_CTX_cleanup(&ctx);

    const char *p = (const char *)plain.data();
    std::string str(p, plain_len);
    return str;
}
}

#include <string>
#include <vector>
#include <cstdint>
#include <openssl/evp.h>

#include "crypt.hpp"

std::vector<uint8_t> hashlib(const char *alg, const std::vector<uint8_t> &message) 
{
    EVP_MD_CTX *mdctx;
    const EVP_MD *md;
    std::vector<uint8_t> md_value(EVP_MAX_MD_SIZE, 0);
    unsigned int md_len, i;
    md = EVP_get_digestbyname(alg);
    mdctx = EVP_MD_CTX_new();
    if (!EVP_DigestInit_ex2(mdctx, md, NULL)) {
        printf("Message digest initialization failed.\n");
        EVP_MD_CTX_free(mdctx);
        return {};
    }
    if (!EVP_DigestUpdate(mdctx, message.data(), message.size())) {
        printf("Message digest update failed.\n");
        EVP_MD_CTX_free(mdctx);
        return {};
    }
    if (!EVP_DigestFinal_ex(mdctx, md_value.data(), &md_len)) {
        printf("Message digest finalization failed.\n");
        EVP_MD_CTX_free(mdctx);
        return {};
    }
    EVP_MD_CTX_free(mdctx);
    md_value.resize(md_len);
    return md_value;
}

std::vector<uint8_t> do_crypt(int do_encrypt, const char *alg, bool padding, const std::vector<uint8_t> &key, const std::vector<uint8_t> &iv, const std::vector<uint8_t> &data, int repeat = 1)
{
    std::vector<uint8_t> outbuf(data.size() * repeat + EVP_MAX_BLOCK_LENGTH, 0);
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    EVP_CIPHER *cipher = EVP_CIPHER_fetch(NULL, alg, NULL);
    if (!EVP_CipherInit_ex2(ctx, cipher, NULL, NULL,
                            do_encrypt, NULL)) {
        /* Error */
        EVP_CIPHER_free(cipher);
        EVP_CIPHER_CTX_free(ctx);
        return {};
    }
    if(!padding) {
        EVP_CIPHER_CTX_set_padding(ctx, 0);
    }
    if (!EVP_CipherInit_ex2(ctx, NULL, key.data(), iv.data(), do_encrypt, NULL)) {
        /* Error */
        EVP_CIPHER_free(cipher);
        EVP_CIPHER_CTX_free(ctx);
        return {};
    }
    int output_len = 0;
    int outlen;
    for(int i = 0; i < repeat; i++) {
        if (!EVP_CipherUpdate(ctx, outbuf.data() + output_len, &outlen, data.data(), data.size())) {
            /* Error */
            EVP_CIPHER_free(cipher);
            EVP_CIPHER_CTX_free(ctx);
            return {};
        }
        output_len += outlen;
    }
    if (!EVP_CipherFinal_ex(ctx, outbuf.data() + output_len, &outlen)) {
        /* Error */
        EVP_CIPHER_free(cipher);
        EVP_CIPHER_CTX_free(ctx);
        return {};
    }
    output_len += outlen;
    EVP_CIPHER_free(cipher);
    EVP_CIPHER_CTX_free(ctx);
    outbuf.resize(output_len);
    return outbuf;
}

std::vector<uint8_t> calculate_hash(int R, const std::vector<uint8_t> &password, const std::vector<uint8_t> &salt, const std::vector<uint8_t> &udata)
{
    std::vector<uint8_t> tmp;
    std::copy(password.begin(), password.end(), std::back_inserter(tmp));
    std::copy(salt.begin(), salt.end(), std::back_inserter(tmp));
    std::copy(udata.begin(), udata.end(), std::back_inserter(tmp));
    auto k = hashlib("SHA256", tmp);
    if(R < 6) return k;
    int count = 0;
    while(true) {
        count++;
        std::vector<uint8_t> k1;
        std::copy(password.begin(), password.end(), std::back_inserter(k1));
        std::copy(k.begin(), k.end(), std::back_inserter(k1));
        std::copy(udata.begin(), udata.end(), std::back_inserter(k1));
        std::vector<uint8_t> k_0_16;
        std::copy(k.begin(), k.begin()+16, std::back_inserter(k_0_16));
        std::vector<uint8_t> k_16_32;
        std::copy(k.begin()+16, k.begin()+32, std::back_inserter(k_16_32));
        auto e = do_crypt(1, "aes-128-cbc", false, k_0_16, k_16_32, k1, 64);
        int sum = 0;
        for(int i = 0; i < 16; i++) {
            sum += e[i];
        }
        sum = sum % 3;
        if(sum == 0) {
            k = hashlib("SHA256", e);
        }
        else if(sum == 1) {
            k = hashlib("SHA384", e);
        }
        else {
            k = hashlib("SHA512", e);
        }
        int last_e = e.back();
        if((count >= 64) && (last_e <= count - 32)) {
            break;
        }
    }
    k.resize(32);
    return k;
}

std::vector<uint8_t> verify_owner_password(int R, const std::vector<uint8_t> &password, const std::vector<uint8_t> &o_value, const std::vector<uint8_t> &oe_value, const std::vector<uint8_t> &u_value)
{
    auto password_trim = password;
    if(password.size() > 127) {
        password_trim.resize(127);
    }
    std::vector<uint8_t> o_value_0_32;
    std::copy(o_value.begin(), o_value.begin()+32, std::back_inserter(o_value_0_32));
    std::vector<uint8_t> o_value_32_40;
    std::copy(o_value.begin()+32, o_value.begin()+40, std::back_inserter(o_value_32_40));
    std::vector<uint8_t> o_value_40_48;
    std::copy(o_value.begin()+40, o_value.begin()+48, std::back_inserter(o_value_40_48));
    std::vector<uint8_t> u_value_0_48;
    std::copy(u_value.begin(), u_value.begin()+48, std::back_inserter(u_value_0_48));
    if(calculate_hash(R, password_trim, o_value_32_40, u_value_0_48) != o_value_0_32) {
        return {};
    }
    std::vector<uint8_t> iv(16);
    auto tmp_key = calculate_hash(R, password_trim, o_value_40_48, u_value_0_48);
    auto key = do_crypt(0, "aes-256-cbc", false, tmp_key, iv, oe_value);
    return key;
}

std::vector<uint8_t> verify_user_password(int R, const std::vector<uint8_t> &password, const std::vector<uint8_t> &u_value, const std::vector<uint8_t> &ue_value)
{
    auto password_trim = password;
    if(password.size() > 127) {
        password_trim.resize(127);
    }
    std::vector<uint8_t> u_value_0_32;
    std::copy(u_value.begin(), u_value.begin()+32, std::back_inserter(u_value_0_32));
    std::vector<uint8_t> u_value_32_40;
    std::copy(u_value.begin()+32, u_value.begin()+40, std::back_inserter(u_value_32_40));
    std::vector<uint8_t> u_value_40_48;
    std::copy(u_value.begin()+40, u_value.begin()+48, std::back_inserter(u_value_40_48));
    if(calculate_hash(R, password_trim, u_value_32_40, {}) != u_value_0_32) {
        return {};
    }
    std::vector<uint8_t> iv(16);
    auto tmp_key = calculate_hash(R, password_trim, u_value_40_48, {});
    auto key = do_crypt(0, "aes-256-cbc", false, tmp_key, iv, ue_value);
    return key;
}

bool verify_perms(const std::vector<uint8_t> &key, const std::vector<uint8_t> &perms, int p, bool metadata_encrypted)
{
    auto p2 = do_crypt(0, "aes-256-ecb", false, key, {}, perms);
    uint32_t Pvalue = *(uint32_t *)&p;
    std::vector<uint8_t> p1;
    std::copy((unsigned char *)&Pvalue, (unsigned char *)&Pvalue + 4, std::back_insert_iterator(p1));
    p1.push_back(0xff);
    p1.push_back(0xff);
    p1.push_back(0xff);
    p1.push_back(0xff);
    if(metadata_encrypted) {
        p1.push_back('T');
    }
    else {
        p1.push_back('F');
    }
    p1.push_back('a');
    p1.push_back('d');
    p1.push_back('b');
    p2.resize(12);
    return p1== p2;
}

std::vector<uint8_t> verify_v5(const std::vector<uint8_t> &password, int R, const std::vector<uint8_t> &O, const std::vector<uint8_t> &U, const std::vector<uint8_t> &OE, const std::vector<uint8_t> &UE)
{
    auto key = verify_owner_password(R, password, O, OE, U);
    if(key.empty()) {
        key = verify_user_password(R, password, U, UE);
    }
    return key;
}

void crypt_object::copy_value(std::vector<uint8_t> &target, const std::string &key)
{
    auto obj = operator[]<string_object>(key);
    if(obj) {
        auto str = obj->get_value();
        std::copy(str.begin(), str.end(), std::back_inserter(target));
    }
}

int crypt_object::read_integer(const std::string &key)
{
    auto obj = operator[]<integer_number>(key);
    if(obj) {
        return obj->get_value();
    }
    return 0;
}

crypt_object::crypt_object(const dictionary_object& base)
    : dictionary_object(base)
{
    copy_value(O, "O");
    copy_value(U, "U");
    copy_value(OE, "OE");
    copy_value(UE, "UE");
    copy_value(Perms, "Perms");
    P = read_integer("P");
    R = read_integer("R");
    V = read_integer("V");
    Length = read_integer("Length");

    if(V == 5) {
        file_encryption_key = verify_v5({}, R, O, U, OE, UE);
        std::cout << "verify: " << verify_perms(file_encryption_key, Perms, P, true) << std::endl;
    }
}

std::string crypt_object::decode_string(const std::string &enc_string)
{
    if(file_encryption_key.empty()) return enc_string;

    if(V == 5) {
        if(enc_string.size() < 16) return enc_string;
        std::vector<uint8_t> iv;
        std::vector<u_int8_t> content;
        std::copy(enc_string.begin(), enc_string.begin()+16, std::back_inserter(iv));
        std::copy(enc_string.begin()+16, enc_string.end(), std::back_inserter(content));
        auto dec_value = do_crypt(0, "aes-256-cbc", true, file_encryption_key, iv, content);
        std::string dec_string;
        std::copy(dec_value.begin(), dec_value.end(), std::back_inserter(dec_string));
        return dec_string;
    }
    return enc_string;
}

std::vector<uint8_t> crypt_object::decode_stream(const std::vector<uint8_t> &enc_stream)
{
    if(file_encryption_key.empty()) return enc_stream;

    if(V == 5) {
        if(enc_stream.size() < 16) return enc_stream;
        std::vector<uint8_t> iv;
        std::vector<u_int8_t> content;
        std::copy(enc_stream.begin(), enc_stream.begin()+16, std::back_inserter(iv));
        std::copy(enc_stream.begin()+16, enc_stream.end(), std::back_inserter(content));
        auto dec_stream = do_crypt(0, "aes-256-cbc", true, file_encryption_key, iv, content);
        return dec_stream;
    }
    return enc_stream;
}

#pragma once
#include <vector>
#include <cstdint>
#include "pdf.hpp"

class crypt_object: public dictionary_object {
    private:
        std::vector<uint8_t> O;
        std::vector<uint8_t> U;
        std::vector<uint8_t> OE;
        std::vector<uint8_t> UE;
        std::vector<uint8_t> Perms;
        int P;
        int R;
        int V;
        int Length;

        std::vector<uint8_t> file_encryption_key;

        void copy_value(std::vector<uint8_t> &target, const std::string &key);
        int read_integer(const std::string &key);
    public:
        crypt_object(const dictionary_object& base);

        std::string decode_string(const std::string &enc_string);
        std::vector<uint8_t> decode_stream(const std::vector<uint8_t> &enc_stream);
};

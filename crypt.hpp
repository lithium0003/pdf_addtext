#pragma once
#include <vector>
#include <cstdint>
#include "pdf.hpp"

class crypt_object: public dictionary_object {
    private:
        std::vector<uint8_t> ID1;
        std::shared_ptr<dictionary_object> CF;
        std::string StmF;
        std::string StrF;

        std::vector<uint8_t> O;
        std::vector<uint8_t> U;
        std::vector<uint8_t> OE;
        std::vector<uint8_t> UE;
        std::vector<uint8_t> Perms;
        int P;
        int R;
        int V;
        int Length;
        bool EncryptMetadata = true;

        std::vector<uint8_t> file_encryption_key;

        void copy_value(std::vector<uint8_t> &target, const std::string &key);
        int read_integer(const std::string &key);
        std::string read_name(const std::string &key);
        bool read_bool(bool &target, const std::string &key);
    public:
        crypt_object(const dictionary_object& base, const std::vector<uint8_t> &id1, const std::vector<uint8_t> &password = {});

        std::string decode_string(const std::string &enc_string, int objnum = 0, int gennum = 0);
        std::vector<uint8_t> decode_stream(const std::vector<uint8_t> &enc_stream, int objnum = 0, int gennum = 0);
};

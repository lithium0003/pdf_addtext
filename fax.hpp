#pragma once
#include <vector>
#include <cstdint>

struct CCITTFaxDecode_Parms {
    int K = 0;
    bool EndOfLine = false;
    bool EncodedByteAlign = false;
    int Columns = 1728;
    int Rows = 0;
    bool EndOfBlock = true;
    bool BlackIs1 = false;
};

class bit_stream {
    private:
        std::vector<uint8_t> _data;
        int _bit = 7;

    public:
        bit_stream() {}
        bit_stream(const std::vector<uint8_t> &data): _data(data) {}

        struct iterator {
            using iterator_category = std::forward_iterator_tag;
            using difference_type   = std::ptrdiff_t;

            private:
                std::vector<uint8_t>::iterator _it;
                int _bit = 0;
                uint8_t work;
            
            public:
                iterator(std::vector<uint8_t>::iterator it, int bit): _it(it), _bit(bit) {}

                int operator*() const { return (((*_it) & (1 << _bit)) > 0 )? 1: 0; }

                // Prefix increment
                iterator& operator++() {
                    if(_bit == 0) {
                        _bit = 7;
                        ++_it;
                    }
                    else {
                        _bit--;
                    }
                    return *this;
                }  
    
                // Postfix increment
                iterator operator++(int) { iterator tmp = *this; ++(*this); return tmp; }

                friend bool operator== (const iterator& a, const iterator& b) { return a._it == b._it && a._bit == b._bit; };
                friend bool operator!= (const iterator& a, const iterator& b) { return a._it != b._it || a._bit != b._bit; };
        };

        iterator begin() { return iterator(_data.begin(), 7); }
        iterator end() { return _bit < 7 ? iterator(_data.end()-1, _bit): iterator(_data.end(), 7); }

        void push_back(int value) {
            if(_bit == 7) {
                _data.push_back(0);
            }
            if(value > 0) {
                _data.back() |= 1 << _bit;
            }
            if(_bit == 0) {
                _bit = 7;
            }
            else {
                _bit--;
            }
        }

        operator std::vector<uint8_t>() const {
            return _data;
        } 
};

class CCITTFaxDecoder {
    private:
        bit_stream bstream;
        std::vector<uint8_t> outbufer;
        std::vector<uint32_t> codingLine;
        std::vector<uint32_t> refLine;
        CCITTFaxDecode_Parms option;
        int codingPos;
        int columns;
        int row = 0;
        bool nextLine2D;
        int inputBits = 0;
        int inputBuf = 0;
        int outputBits = 0;
        bool rowsDone = false;
        bool eof = false;

        void _addPixels(int a1, int blackPixels);
        void _addPixelsNeg(int a1, int blackPixels);

    public:
        CCITTFaxDecoder(const std::vector<uint8_t> &data, const CCITTFaxDecode_Parms &option);

        void output(const std::string &target_path, int page_no, const std::string &key, int width, int height, int rotate = 0);
};

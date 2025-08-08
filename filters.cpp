//
//  filters.cpp
//  bunkoOCR
//
//  Created by rei9 on 2025/08/04.
//

#include "filters.hpp"

#include <zlib.h>

#include    "JBIG2/Jb2Common.h"
#include    "JBIG2/Jb2_T4T6Lapper.h"
#include    "JBIG2/ImageUtil.h"
#include    "JBIG2/Jb2_Debug.h"

std::vector<uint8_t> ASCIIHexDecode(const std::vector<uint8_t> &stream)
{
    std::vector<uint8_t> result;
    int i = 0;
    uint8_t value = 0;
    for(auto v: stream) {
        if(v >= '0' && v <= '9') {
            value |= v - '0';
        }
        else if(v >= 'A' && v <= 'F') {
            value |= v - 'A' + 10;
        }
        else if(v >= 'a' && v <= 'f') {
            value |= v - 'a' + 10;
        }
        else if(v == 0x3e) {
            if(i == 1) {
                result.push_back(value);
            }
            break;
        }
        else {
            continue;
        }
        i++;
        if(i == 1) {
            value <<= 4;
        }
        else if(i == 2) {
            result.push_back(value);
            value = 0;
            i = 0;
        }
    }
    return result;
}

std::vector<uint8_t> ASCII85Decode(const std::vector<uint8_t> &stream)
{
    std::vector<uint8_t> result;
    int i = 0;
    uint32_t value = 0;
    for(auto v: stream) {
        if(v >= '!' && v <= 'u') {
            int b = v - '!';
            if(i == 0) {
                value += b * 85 * 85 * 85 * 85;
            }
            else if(i == 1) {
                value += b * 85 * 85 * 85;
            }
            else if(i == 2) {
                value += b * 85 * 85;
            }
            else if(i == 3) {
                value += b * 85;
            }
            else if(i == 4) {
                value += b;
            }
        }
        else if(i == 0 && v == 'z') {
            result.push_back(0);
            result.push_back(0);
            result.push_back(0);
            result.push_back(0);
            value = 0;
            i = 0;
            continue;
        }
        else if(v == 0x7e) {
            if(i == 0) {
            }
            else if(i == 2) {
                result.push_back((value & 0xff000000) >> 24);
            }
            else if(i == 3) {
                result.push_back((value & 0xff000000) >> 24);
                result.push_back((value & 0x00ff0000) >> 16);
            }
            else if(i == 4) {
                result.push_back((value & 0xff000000) >> 24);
                result.push_back((value & 0x00ff0000) >> 16);
                result.push_back((value & 0x0000ff00) >> 8);
            }
            break;
        }
        else {
            continue;
        }
        i++;
        if(i == 5) {
            result.push_back((value & 0xff000000) >> 24);
            result.push_back((value & 0x00ff0000) >> 16);
            result.push_back((value & 0x0000ff00) >> 8);
            result.push_back(value & 0x000000ff);
            value = 0;
            i = 0;
        }
    }
    return result;
}

int PaethPredictor(int a, int b, int c)
{
    // +-+-+
    // |c|b|
    // +-+-+
    // |a|?|
    // +-+-+
    int p = a + b - c;

    // pa = |b - c|     横向きの値の変わり具合
    // pb = |a - c|     縦向きの値の変わり具合
    // pc = |b-c + a-c| ふたつの合計
    int pa = abs(p - a);
    int pb = abs(p - b);
    int pc = abs(p - c);

    // 横向きのほうがなだらかな値の変化 → 左
    if (pa <= pb && pa <= pc)
        return a;

    // 縦向きのほうがなだらかな値の変化 → 上
    if (pb <= pc)
        return b;
        
    // 縦横それぞれ正反対に値が変化するため中間色を選択 → 左上
    return c;
}

std::vector<uint8_t> FlateDecode(const std::vector<uint8_t> &stream, std::shared_ptr<dictionary_object> params)
{
    std::vector<uint8_t> result;
    std::vector<uint8_t> outBuf(0x4000);
    z_stream zStream{ 0 };
    inflateInit(&zStream);

    zStream.avail_in = (uInt)stream.size();
    zStream.next_in = const_cast<uint8_t*>(stream.data());
    do {
        zStream.next_out = outBuf.data();
        zStream.avail_out = (uInt)outBuf.size();
        inflate(&zStream, Z_NO_FLUSH);
        auto outSize = outBuf.size() - zStream.avail_out;
        std::copy(outBuf.begin(), outBuf.begin() + outSize, std::back_inserter(result));
    } while (zStream.avail_out == 0);

    inflateEnd(&zStream);

    if(params) {
        int predictor = 1;
        auto predictor_obj = params->operator[]<integer_number>("Predictor");
        if(predictor_obj) {
            predictor = predictor_obj->get_value();
        }
        int columns = 1;
        auto columns_obj = params->operator[]<integer_number>("Columns");
        if(columns_obj) {
            columns = columns_obj->get_value();
        }
        if(predictor == 10) {
            std::vector<uint8_t> fixresult;
            auto it = result.begin();
            while(it != result.end()) {
                ++it;
                for(int x = 0; x < columns; x++) {
                    fixresult.push_back(*it++);
                }
            }
            return fixresult;
        }
        if(predictor == 11) {
            std::vector<uint8_t> fixresult;
            auto it = result.begin();
            while(it != result.end()) {
                ++it;
                uint8_t prev = 0;
                for(int x = 0; x < columns; x++) {
                    uint8_t v = *it++;
                    fixresult.push_back((v + prev) % 256);
                    prev = v;
                }
            }
            return fixresult;
        }
        if(predictor == 12) {
            std::vector<uint8_t> fixresult;
            auto it = result.begin();
            while(it != result.end()) {
                ++it;
                for(int x = 0; x < columns; x++) {
                    uint8_t prev = fixresult.size() < columns ? 0 : *(fixresult.end() - columns);
                    fixresult.push_back((*it++ + prev) % 256);
                }
            }
            return fixresult;
        }
        if(predictor == 13) {
            std::vector<uint8_t> fixresult;
            auto it = result.begin();
            while(it != result.end()) {
                ++it;
                uint8_t left = 0;
                for(int x = 0; x < columns; x++) {
                    uint8_t up = fixresult.size() < columns ? 0 : *(fixresult.end() - columns);
                    fixresult.push_back((*it++ + (left + up)/2) % 256);
                }
            }
            return fixresult;
        }
        if(predictor == 14) {
            std::vector<uint8_t> fixresult;
            auto it = result.begin();
            while(it != result.end()) {
                ++it;
                uint8_t left = 0;
                for(int x = 0; x < columns; x++) {
                    uint8_t up = fixresult.size() < columns ? 0 : *(fixresult.end() - columns);
                    uint8_t upleft = fixresult.size() < columns+1 ? 0 : *(fixresult.end() - columns-1);
                    fixresult.push_back((*it++ + PaethPredictor(left, up, upleft)) % 256);
                }
            }
            return fixresult;
        }
    }

    return result;
}

std::vector<uint8_t> JBIG2Decode(const std::vector<uint8_t> &stream)
{
    struct    StreamChain_s *str = StreamChainMake(NULL, (byte4)stream.size(), NoDiscard);
    struct    ImageChain_s *rImagePage = NULL;
    memcpy(str->buf, stream.data(), stream.size());
    ubyte4    NumberOfSegments = 1;
    uchar     ColorExtFlag = 0;

    auto Saddr = str->cur_p;
    NumberOfSegments = SegmentNumCount(str);

    auto Huff = CreateHuffmanTable( DEC );
    str->cur_p = Saddr;
    auto ImagePage = SegmentDecode( str, Huff, NumberOfSegments, &ColorExtFlag );//ImagePage->Image

    ImagePage = ImageChainSearch( ImagePage, 0);
    do{
        rImagePage = ImageChainCreate(rImagePage);
        if(!ColorExtFlag)
            rImagePage->Image = (struct Image_s *)ImageCharToBit1(ImagePage->Image);
        else
            rImagePage->Image = ImagePage->Image;
        ImagePage = ImagePage->child;
    } while(ImagePage!=NULL);

    std::vector<uint8_t> dec_stream(rImagePage->Image->width * rImagePage->Image->height);
    if(rImagePage->Image->type == BIT1) {
        auto D_TS = (uchar *)rImagePage->Image->data;
        auto dstp = dec_stream.data();
        for(int y = 0; y < rImagePage->Image->height; y++, D_TS += rImagePage->Image->col1step) {
            for(int x = 0; x < rImagePage->Image->width; x++, dstp++) {
                auto v = D_TS[x / 8] & (1 << (7 - x % 8));
                if(rImagePage->Image->MaxValue < 0){
                    *dstp = (v > 0) ? 0 : 0xff;
                }
                else {
                    *dstp = (v == 0) ? 0 : 0xff;
                }
            }
        }
        return dec_stream;
    }
    else {
        auto D_TS = (uchar *)rImagePage->Image->data;
        auto dstp = dec_stream.data();
        for(int y = 0; y < rImagePage->Image->height; y++, D_TS += rImagePage->Image->col1step) {
            for(int x = 0; x < rImagePage->Image->width; x++, dstp++) {
                auto v = D_TS[x];
                if(rImagePage->Image->MaxValue < 0){
                    *dstp = 0xff - v;
                }
                else {
                    *dstp = v;
                }
            }
        }
        return dec_stream;
    }
}

// https://rosettacode.org/wiki/LZW_compression#C++
template <typename Iterator>
std::string decompress(Iterator begin, Iterator end) {
    // Build the dictionary.
    int dictSize = 256;
    std::map<int,std::string> dictionary;
    for (int i = 0; i < 256; i++)
        dictionary[i] = std::string(1, i);
    
    std::string w(1, *begin++);
    std::string result = w;
    std::string entry;
    for ( ; begin != end; begin++) {
        int k = *begin;
        if (dictionary.count(k))
            entry = dictionary[k];
        else if (k == dictSize)
            entry = w + w[0];
        else
            throw "Bad compressed k";
        
        result += entry;
        
        // Add w+entry[0] to the dictionary.
        dictionary[dictSize++] = w + entry[0];
        
        w = entry;
    }
    return result;
}

std::vector<uint8_t> LZWDecode(const std::vector<uint8_t> &stream)
{
    auto value = decompress(stream.begin(), stream.end());
    std::vector<uint8_t> result;
    std::copy(value.begin(), value.end(), std::back_inserter(result));
    return result;
}


void decode_stream(dictionary_object &dict, std::vector<uint8_t> &stream)
{
    auto filter = dict.operator[]<name_object>("Filter");
    auto decodeParms = dict.operator[]<dictionary_object>("DecodeParms");
    if(filter) {
        if(filter->get_value() == "ASCIIHexDecode") {
            stream = ASCIIHexDecode(stream);
            dict.remove("Filter");
        }
        else if(filter->get_value() == "ASCII85Decode") {
            stream = ASCII85Decode(stream);
            dict.remove("Filter");
        }
        else if(filter->get_value() == "LZWDecode") {
            stream = LZWDecode(stream);
            dict.remove("Filter");
        }
        else if(filter->get_value() == "FlateDecode") {
            stream = FlateDecode(stream, decodeParms);
            dict.remove("Filter");
            dict.remove("DecodeParms");
        }
        return;
    }

    auto filters = dict.operator[]<array_object>("Filter");
    auto decodeParmss = dict.operator[]<array_object>("DecodeParms");
    if(filters) {
        int i = 0;
        while(true) {
            filter = filters->operator[]<name_object>(i);
            if(!filter) {
                break;
            }

            if(filter->get_value() == "ASCIIHexDecode") {
                stream = ASCIIHexDecode(stream);
                filters->remove(i);
            }
            else if(filter->get_value() == "ASCII85Decode") {
                stream = ASCII85Decode(stream);
                filters->remove(i);
            }
            else if(filter->get_value() == "LZWDecode") {
                stream = LZWDecode(stream);
                filters->remove(i);
            }
            else if(filter->get_value() == "FlateDecode") {
                if(decodeParmss) {
                    decodeParms = decodeParmss->operator[]<dictionary_object>(i);
                    if(decodeParms) {
                        decodeParmss->remove(i);
                    }
                }
                stream = FlateDecode(stream, decodeParms);
                filters->remove(i);
            }
            else {
                i++;
            }
        }
        if(filters->size() == 0) {
            dict.remove("Filter");
            dict.remove("DecodeParms");
        }
    }
}

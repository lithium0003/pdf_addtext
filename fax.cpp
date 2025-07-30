#include <iostream>
#include <map>
#include <string>
#include <filesystem>
#include <sstream>

#include <png.h>

#include "fax.hpp"

std::map<uint32_t, int> whiteTable = {
    {0b100110101, 0 },
    {0b1000111, 1 },
    {0b10111, 2 },
    {0b11000, 3 },
    {0b11011, 4 },
    {0b11100, 5 },
    {0b11110, 6 },
    {0b11111, 7 },
    {0b110011, 8 },
    {0b110100, 9 },
    {0b100111, 10 },
    {0b101000, 11 },
    {0b1001000, 12 },
    {0b1000011, 13 },
    {0b1110100, 14 },
    {0b1110101, 15 },
    {0b1101010, 16 },
    {0b1101011, 17 },
    {0b10100111, 18 },
    {0b10001100, 19 },
    {0b10001000, 20 },
    {0b10010111, 21 },
    {0b10000011, 22 },
    {0b10000100, 23 },
    {0b10101000, 24 },
    {0b10101011, 25 },
    {0b10010011, 26 },
    {0b10100100, 27 },
    {0b10011000, 28 },
    {0b100000010, 29 },
    {0b100000011, 30 },
    {0b100011010, 31 },
    {0b100011011, 32 },
    {0b100010010, 33 },
    {0b100010011, 34 },
    {0b100010100, 35 },
    {0b100010101, 36 },
    {0b100010110, 37 },
    {0b100010111, 38 },
    {0b100101000, 39 },
    {0b100101001, 40 },
    {0b100101010, 41 },
    {0b100101011, 42 },
    {0b100101100, 43 },
    {0b100101101, 44 },
    {0b100000100, 45 },
    {0b100000101, 46 },
    {0b100001010, 47 },
    {0b100001011, 48 },
    {0b101010010, 49 },
    {0b101010011, 50 },
    {0b101010100, 51 },
    {0b101010101, 52 },
    {0b100100100, 53 },
    {0b100100101, 54 },
    {0b101011000, 55 },
    {0b101011001, 56 },
    {0b101011010, 57 },
    {0b101011011, 58 },
    {0b101001010, 59 },
    {0b101001011, 60 },
    {0b100110010, 61 },
    {0b100110011, 62 },
    {0b100110100, 63 },
    {0b111011, 64 },
    {0b110010, 128 },
    {0b1010111, 192 },
    {0b10110111, 256 },
    {0b100110110, 320 },
    {0b100110111, 384 },
    {0b101100100, 448 },
    {0b101100101, 512 },
    {0b101101000, 576 },
    {0b101100111, 640 },
    {0b1011001100, 704 },
    {0b1011001101, 768 },
    {0b1011010010, 832 },
    {0b1011010011, 896 },
    {0b1011010100, 960 },
    {0b1011010101, 1024 },
    {0b1011010110, 1088 },
    {0b1011010111, 1152 },
    {0b1011011000, 1216 },
    {0b1011011001, 1280 },
    {0b1011011010, 1344 },
    {0b1011011011, 1408 },
    {0b1010011000, 1472 },
    {0b1010011001, 1536 },
    {0b1010011010, 1600 },
    {0b1011000, 1664 },
    {0b1010011011, 1728 },
    {0b100000001000, 1792 },
    {0b100000001100, 1856 },
    {0b100000001101, 1920 },
    {0b1000000010010, 1984 },
    {0b1000000010011, 2048 },
    {0b1000000010100, 2112 },
    {0b1000000010101, 2176 },
    {0b1000000010110, 2240 },
    {0b1000000010111, 2304 },
    {0b1000000011100, 2368 },
    {0b1000000011101, 2432 },
    {0b1000000011110, 2496 },
    {0b1000000011111, 2560 },
};

std::map<uint32_t, int> blackTable = {
    {0b10000110111, 0 },
    {0b1010, 1 },
    {0b111, 2 },
    {0b110, 3 },
    {0b1011, 4 },
    {0b10011, 5 },
    {0b10010, 6 },
    {0b100011, 7 },
    {0b1000101, 8 },
    {0b1000100, 9 },
    {0b10000100, 10 },
    {0b10000101, 11 },
    {0b10000111, 12 },
    {0b100000100, 13 },
    {0b100000111, 14 },
    {0b1000011000, 15 },
    {0b10000010111, 16 },
    {0b10000011000, 17 },
    {0b10000001000, 18 },
    {0b100001100111, 19 },
    {0b100001101000, 20 },
    {0b100001101100, 21 },
    {0b100000110111, 22 },
    {0b100000101000, 23 },
    {0b100000010111, 24 },
    {0b100000011000, 25 },
    {0b1000011001010, 26 },
    {0b1000011001011, 27 },
    {0b1000011001100, 28 },
    {0b1000011001101, 29 },
    {0b1000001101000, 30 },
    {0b1000001101001, 31 },
    {0b1000001101010, 32 },
    {0b1000001101011, 33 },
    {0b1000011010010, 34 },
    {0b1000011010011, 35 },
    {0b1000011010100, 36 },
    {0b1000011010101, 37 },
    {0b1000011010110, 38 },
    {0b1000011010111, 39 },
    {0b1000001101100, 40 },
    {0b1000001101101, 41 },
    {0b1000011011010, 42 },
    {0b1000011011011, 43 },
    {0b1000001010100, 44 },
    {0b1000001010101, 45 },
    {0b1000001010110, 46 },
    {0b1000001010111, 47 },
    {0b1000001100100, 48 },
    {0b1000001100101, 49 },
    {0b1000001010010, 50 },
    {0b1000001010011, 51 },
    {0b1000000100100, 52 },
    {0b1000000110111, 53 },
    {0b1000000111000, 54 },
    {0b1000000100111, 55 },
    {0b1000000101000, 56 },
    {0b1000001011000, 57 },
    {0b1000001011001, 58 },
    {0b1000000101011, 59 },
    {0b1000000101100, 60 },
    {0b1000001011010, 61 },
    {0b1000001100110, 62 },
    {0b1000001100111, 63 },
    {0b10000001111, 64 },
    {0b1000011001000, 128 },
    {0b1000011001001, 192 },
    {0b1000001011011, 256 },
    {0b1000000110011, 320 },
    {0b1000000110100, 384 },
    {0b1000000110101, 448 },
    {0b10000001101100, 512 },
    {0b10000001101101, 576 },
    {0b10000001001010, 640 },
    {0b10000001001011, 704 },
    {0b10000001001100, 768 },
    {0b10000001001101, 832 },
    {0b10000001110010, 896 },
    {0b10000001110011, 960 },
    {0b10000001110100, 1024 },
    {0b10000001110101, 1088 },
    {0b10000001110110, 1152 },
    {0b10000001110111, 1216 },
    {0b10000001010010, 1280 },
    {0b10000001010011, 1344 },
    {0b10000001010100, 1408 },
    {0b10000001010101, 1472 },
    {0b10000001011010, 1536 },
    {0b10000001011011, 1600 },
    {0b10000001100100, 1664 },
    {0b10000001100101, 1728 },
    {0b100000001000, 1792 },
    {0b100000001100, 1856 },
    {0b100000001101, 1920 },
    {0b1000000010010, 1984 },
    {0b1000000010011, 2048 },
    {0b1000000010100, 2112 },
    {0b1000000010101, 2176 },
    {0b1000000010110, 2240 },
    {0b1000000010111, 2304 },
    {0b1000000011100, 2368 },
    {0b1000000011101, 2432 },
    {0b1000000011110, 2496 },
    {0b1000000011111, 2560 },
};

std::map<uint32_t, std::string> twoDimTable = {
    {0b10001, "P"},
    {0b1001, "H"},
    {0b11, "V0"},
    {0b1011, "VR1"},
    {0b1000011, "VR2"},
    {0b10000011, "VR3"},
    {0b1010, "VL1"},
    {0b1000010, "VL2"},
    {0b10000010, "VL3"},
    {0b1000000000001, "EOL"},
};

int get_ValueCode(const std::map<uint32_t, int> &table, bit_stream::iterator &it, const bit_stream::iterator &end)
{
    uint32_t work = 1 << 1;
    while(it != end) {
        work += *it++;
        if(table.count(work) > 0) {
            auto code = table.at(work);
            return code;
        }
        work <<= 1;
    }
    return -1;
}

std::string get_TwoDimCode(bit_stream::iterator &it, const bit_stream::iterator &end)
{
    uint32_t work = 1 << 1;
    while(it != end) {
        work += *it++;
        if(twoDimTable.count(work) > 0) {
            auto state = twoDimTable[work];
            return state;
        }
        work <<= 1;
    }
    return "Error";
}

void CCITTFaxDecoder::_addPixels(int a1, int blackPixels) 
{
    if(a1 > codingLine[codingPos]) {
        if (a1 > columns) {
            std::cerr << "row is wrong length" << std::endl;
            a1 = columns;
        }
        if ((codingPos & 1) ^ blackPixels) {
            ++codingPos;
        }

        codingLine[codingPos] = a1;
    }
}

void CCITTFaxDecoder::_addPixelsNeg(int a1, int blackPixels) {
    if(a1 > codingLine[codingPos]) {
        if (a1 > columns) {
            std::cerr << "row is wrong length" << std::endl;
            a1 = columns;
        }
        if ((codingPos & 1) ^ blackPixels) {
            ++codingPos;
        }

        codingLine[codingPos] = a1;
    }
    else if (a1 < codingLine[codingPos]) {
        if (a1 < 0) {
            std::cerr << "invalid code" << std::endl;
            a1 = 0;
        }
        while (codingPos > 0 && a1 < codingLine[codingPos - 1]) {
            --codingPos;
        }
        codingLine[codingPos] = a1;
    }
}

CCITTFaxDecoder::CCITTFaxDecoder(const std::vector<uint8_t> &data, const CCITTFaxDecode_Parms &option)
    : bstream(data), codingLine(option.Columns + 1), refLine(option.Columns + 2),
    columns(option.Columns), nextLine2D(option.K < 0) {

    codingLine[0] = columns;
    codingPos = 0;

    auto it = bstream.begin();
    if(option.K >= 0) {
        int count = 1;
        while(it != bstream.end()) {
            if(*it++ == 0) {
                count++;
            }
            else if(count == 12) {
                break;
            }
            else {
                count = 0;
            }
        }
        nextLine2D = *it == 0;
        if(option.K > 0) {
            ++it;
        }
    }

    while(it != bstream.end()) {
        if (nextLine2D) {
            int i;
            for (i = 0; codingLine[i] < columns; ++i) {
                refLine[i] = codingLine[i];
            }
            refLine[i++] = columns;
            refLine[i] = columns;
            codingLine[0] = 0;
            codingPos = 0;
            int refPos = 0;
            int blackPixels = 0;

            while (codingLine[codingPos] < columns) {
                auto code1 = get_TwoDimCode(it, bstream.end());
                if(code1 == "P") {
                    _addPixels(refLine[refPos + 1], blackPixels);
                    if (refLine[refPos + 1] < columns) {
                        refPos += 2;
                    }
                }
                else if(code1 == "H") {
                    int code1 = 0;
                    int code2 = 0;
                    int code3 = 0;
                    if (blackPixels) {
                        do {
                            code1 += code3 = get_ValueCode(blackTable, it, bstream.end());
                        } while (code3 >= 64);
                        do {
                            code2 += code3 = get_ValueCode(whiteTable, it, bstream.end());
                        } while (code3 >= 64);
                    }
                    else {
                        do {
                            code1 += code3 = get_ValueCode(whiteTable, it, bstream.end());
                        } while (code3 >= 64);
                        do {
                            code2 += code3 = get_ValueCode(blackTable, it, bstream.end());
                        } while (code3 >= 64);
                    }
                    _addPixels(codingLine[codingPos] + code1, blackPixels);
                    if (codingLine[codingPos] < columns) {
                        _addPixels(codingLine[codingPos] + code2, blackPixels ^ 1);
                    }
                    while (refLine[refPos] <= codingLine[codingPos] && refLine[refPos] < columns) {
                        refPos += 2;
                    }
                }
                else if(code1 == "VR3") {
                    _addPixels(refLine[refPos] + 3, blackPixels);
                    blackPixels ^= 1;
                    if (codingLine[codingPos] < columns) {
                        ++refPos;
                        while (refLine[refPos] <= codingLine[codingPos] && refLine[refPos] < columns) {
                            refPos += 2;
                        }
                    }
                }
                else if(code1 == "VR2") {
                    _addPixels(refLine[refPos] + 2, blackPixels);
                    blackPixels ^= 1;
                    if (codingLine[codingPos] < columns) {
                        ++refPos;
                        while (refLine[refPos] <= codingLine[codingPos] && refLine[refPos] < columns) {
                            refPos += 2;
                        }
                    }
                }
                else if(code1 == "VR1") {
                    _addPixels(refLine[refPos] + 1, blackPixels);
                    blackPixels ^= 1;
                    if (codingLine[codingPos] < columns) {
                        ++refPos;
                        while (refLine[refPos] <= codingLine[codingPos] && refLine[refPos] < columns) {
                            refPos += 2;
                        }
                    }
                }
                else if(code1 == "V0") {
                    _addPixels(refLine[refPos], blackPixels);
                    blackPixels ^= 1;
                    if (codingLine[codingPos] < columns) {
                        ++refPos;
                        while (refLine[refPos] <= codingLine[codingPos] && refLine[refPos] < columns) {
                            refPos += 2;
                        }
                    }
                }
                else if(code1 == "VL3") {
                    _addPixelsNeg(refLine[refPos] - 3, blackPixels);
                    blackPixels ^= 1;
                    if (codingLine[codingPos] < columns) {
                        if (refPos > 0) {
                            --refPos;
                        } else {
                            ++refPos;
                        }
                        while (refLine[refPos] <= codingLine[codingPos] && refLine[refPos] < columns) {
                            refPos += 2;
                        }
                    }
                }
                else if(code1 == "VL2") {
                    _addPixelsNeg(refLine[refPos] - 2, blackPixels);
                    blackPixels ^= 1;
                    if (codingLine[codingPos] < columns) {
                        if (refPos > 0) {
                            --refPos;
                        } else {
                            ++refPos;
                        }
                        while (refLine[refPos] <= codingLine[codingPos] && refLine[refPos] < columns) {
                            refPos += 2;
                        }
                    }
                }
                else if(code1 == "VL1") {
                    _addPixelsNeg(refLine[refPos] - 1, blackPixels);
                    blackPixels ^= 1;
                    if (codingLine[codingPos] < columns) {
                        if (refPos > 0) {
                            --refPos;
                        } else {
                            ++refPos;
                        }
                        while (refLine[refPos] <= codingLine[codingPos] && refLine[refPos] < columns) {
                            refPos += 2;
                        }
                    }
                }
                else if(code1 == "EOL") {
                    _addPixels(columns, 0);
                    eof = true;
                }
                else {
                    std::cerr << "bad 2d code" << std::endl;
                    _addPixels(columns, 0);
                }
            }
        }
        else {
            codingLine[0] = 0;
            codingPos = 0;
            int blackPixels = 0;
            while (codingLine[codingPos] < columns) {
                int code1 = 0;
                int code2 = 0;
                int code3 = 0;
                if (blackPixels) {
                    do {
                        code1 += code3 = get_ValueCode(blackTable, it, bstream.end());
                    } while (code3 >= 64);
                } else {
                    do {
                        code1 += code3 = get_ValueCode(whiteTable, it, bstream.end());
                    } while (code3 >= 64);
                }
                _addPixels(codingLine[codingPos] + code1, blackPixels);
                blackPixels ^= 1;
            }
        }

        bool gotEOL = false;
        if(!option.EndOfBlock && row == option.Rows - 1) {
            rowsDone = true;
        }
        else {
            if (option.EndOfLine) {
                int count = 1;
                while(it != bstream.end()) {
                    if(*it++ == 0) {
                        count++;
                    }
                    else if(count == 12) {
                        break;
                    }
                    else {
                        count = 0;
                    }
                }
                if(it == bstream.end()) {
                    eof = true;
                }
                else {
                    gotEOL = true;
                }
            } 
            else {
                auto it2 = it;
                int count = 1;
                while(it2 != bstream.end()) {
                    if(*it2++ == 0) {
                        count++;
                    }
                    else if(count == 12) {
                        gotEOL = true;
                        break;
                    }
                    else {
                        break;
                    }
                }
                if(gotEOL) {
                    it = it2;
                }
                else if(it == bstream.end()) {
                    eof = true;
                }
            }
        }

        if (!eof && option.K > 0 && !rowsDone) {
            nextLine2D = *it++ == 0;
        }

        if(option.EndOfBlock && gotEOL && option.EncodedByteAlign) {
            int count = 1;
            while(it != bstream.end()) {
                if(*it == 0) {
                    count++;
                    it++;
                }
                else {
                    break;
                }
            }
            if(count == 12) {
                ++it;
                if(option.K > 0) {
                    ++it;
                }

                if(option.K >= 0) {
                    // RTC
                    for(int i = 0; i < 4; i++) {
                        while(it != bstream.end()) {
                            if(*it++ == 0) {
                                count++;
                            }
                            else if(count == 12) {
                                break;
                            }
                            else {
                                std::cerr << "bad rtc code" << std::endl;
                                break;
                            }
                        }
                        if(option.K > 0) {
                            ++it;
                        }
                    }
                }

                eof = true;
            }
        }

        if(eof) break;

        int x = 0;
        for (int i = (codingLine[0] > 0 ? 0: 1); x < columns; ++i) {
            int c = i & 1 ? 0 : 0xff;
            if(option.BlackIs1) {
                c = c == 0 ? 0xff : 0;
            }
            while(x <= codingLine[i] && x < columns) {
                outbufer.push_back(c);
                x++;
            }
        }
        row++;
    }
}

void CCITTFaxDecoder::output(const std::string &target_path, int page_no, const std::string &key, int width, int height, int rotate)
{
    if(rotate == 90 || rotate == 270) {
        std::swap(width, height);
    }

    std::filesystem::path path(target_path);
    std::stringstream ss;
    ss.fill('0');
    ss << "page" << std::setw(4) << page_no;
    path /= ss.str() + "_" + key + ".png";

    FILE *fp = fopen(path.c_str(), "wb");
    auto png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    auto info = png_create_info_struct(png);
    png_init_io(png, fp);

    png_set_IHDR(png, info, width, height, 8,
      PNG_COLOR_TYPE_GRAY, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
      PNG_FILTER_TYPE_DEFAULT);
    png_bytepp rows = (png_bytepp)png_malloc(png, sizeof(png_bytep) * height);
    png_set_rows(png, info, rows);
    memset(rows, 0, sizeof(png_bytep) * height);
    for (int y = 0; y < height; y++) {
        rows[y] = (png_bytep)png_malloc(png, sizeof(png_byte) * width);
    }
    if(rotate == 0) {
        const uint8_t *datap = outbufer.data();
        for (int y = 0; y < height; y++) {
            png_bytep row = rows[y];
            for (int x = 0; x < width; x++) {
                *row++ = *datap++;
            }
        }
    }
    else if(rotate == 90) {
        for (int y = 0; y < height; y++) {
            png_bytep row = rows[y];
            for (int x = 0; x < width; x++) {
                *row++ = outbufer[(width - 1 - x)*height + y];
            }
        }
    }
    else if(rotate == 180) {
        for (int y = 0; y < height; y++) {
            png_bytep row = rows[y];
            for (int x = 0; x < width; x++) {
                *row++ = outbufer[(height - 1 - y)*width + x];
            }
        }
    }
    else if(rotate == 270) {
        for (int y = 0; y < height; y++) {
            png_bytep row = rows[y];
            for (int x = 0; x < width; x++) {
                *row++ = outbufer[x * height + (height - 1 - y)];
            }
        }
    }
    png_write_png(png, info, PNG_TRANSFORM_IDENTITY, NULL);
    if (rows != NULL) {
        for (int y = 0; y < height; y++) {
            png_free(png, rows[y]);
        }
        png_free(png, rows);
    }
    png_destroy_write_struct(&png, &info);
    fclose(fp); 
}

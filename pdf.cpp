#include "pdf.hpp"

#include <jpeglib.h>
#include <zlib.h>

std::ostream& operator<<(std::ostream& os, const object* obj)
{
    os << obj->output();
    return os;
}

bool operator<(const name_object& a, const name_object& b) noexcept {
    return a.get_value() < b.get_value();
}

std::string stream_object::output() const {
    if(zlib_deflate){
        z_stream zStream{ 0 };
        deflateInit(&zStream, Z_DEFAULT_COMPRESSION);

        zStream.avail_in = _stream.size();
        zStream.next_in = const_cast<uint8_t*>(_stream.data());
        std::vector<uint8_t> outBuf(0x4000);
        std::vector<uint8_t> outStream;
        do {
            zStream.next_out = outBuf.data();
            zStream.avail_out = outBuf.size();
            deflate(&zStream, Z_FINISH);
            auto outSize = outBuf.size() - zStream.avail_out;
            std::copy(outBuf.begin(), outBuf.begin() + outSize, std::back_inserter(outStream));
        } while (zStream.avail_out == 0);

        deflateEnd(&zStream);
        auto dict = _dict;

        if(dict.operator[]<array_object>("Filter") == nullptr) {
            dict.add("Filter", std::shared_ptr<object>(new array_object({ std::shared_ptr<object>(new name_object("FlateDecode")) })));
        }
        else {
            dict.operator[]<array_object>("Filter")->insert(0, std::shared_ptr<object>(new name_object("FlateDecode")));
        }
        if(dict.operator[]<integer_number>("Length") == nullptr) {
            auto length = outStream.size();
            dict.add("Length", std::shared_ptr<object>(new integer_number(length)));
        }
        else {
            auto length = outStream.size();
            dict.operator[]<integer_number>("Length")->set_value(length);
        }

        std::stringstream ss;
        ss << dict.output() << "\r\n";
        ss << "stream\r\n";
        ss.write((const char *)outStream.data(), outStream.size());
        ss << "\r\nendstream";
        return ss.str();
    }
    else {
        std::stringstream ss;
        ss << _dict.output() << "\r\n";
        ss << "stream\r\n";
        ss.write((const char *)_stream.data(), _stream.size());
        ss << "\r\nendstream";
        return ss.str();
    }
}

JpegImageObject::JpegImageObject(const std::string &filename)
{
    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;    
    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_decompress(&cinfo);

    FILE *fp = fopen(filename.c_str(), "rb");
    fseek(fp, 0, SEEK_END);
    size_t filesize = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    jpeg_stdio_src(&cinfo, fp);

    jpeg_read_header(&cinfo, false);
    width  = cinfo.image_width;
    height = cinfo.image_height;
    bool rgb = cinfo.out_color_space == JCS_RGB;
    
    fseek(fp, 0, SEEK_SET);
    _stream.resize(filesize);
    fread(_stream.data(), filesize, 1, fp);

    fclose(fp);

    _dict.add("Type", std::shared_ptr<object>(new name_object("XObject")));
    _dict.add("Subtype", std::shared_ptr<object>(new name_object("Image")));
    _dict.add("Width", std::shared_ptr<object>(new integer_number(width)));
    _dict.add("Height", std::shared_ptr<object>(new integer_number(height)));
    if(rgb) {
        _dict.add("ColorSpace", std::shared_ptr<object>(new name_object("DeviceRGB")));
    }
    else {
        _dict.add("ColorSpace", std::shared_ptr<object>(new name_object("DeviceGray")));
    }
    _dict.add("BitsPerComponent", std::shared_ptr<object>(new integer_number(8)));
    _dict.add("Filter", std::shared_ptr<object>(new array_object({ std::shared_ptr<object>(new name_object("DCTDecode")) })));

    _dict.add("Length", std::shared_ptr<object>(new integer_number(filesize)));
}

pdf_file::pdf_file()
{
    root = add_object(std::shared_ptr<object>(new RootObject()));
    pages = add_object(std::shared_ptr<object>(new PagesObject()));
    root->cast<indirect_object>()->follow<RootObject>()->add_pages(pages);
    trailer.add("Root", root);

    prepare_font();
}

extern std::vector<uint8_t> dummyFont;

void pdf_file::prepare_font()
{
    auto dummyFontObject = add_object(std::shared_ptr<object>(new stream_object(dummyFont)));
    dummyFontObject->cast<indirect_object>()->follow<stream_object>()->get_dict().add("Subtype", std::shared_ptr<object>(new name_object("CIDFontType0C")));

    auto fontDescriptorObject = add_object(std::shared_ptr<object>(new dictionary_object()));
    fontDescriptorObject->cast<indirect_object>()->follow<dictionary_object>()->add("Type", std::shared_ptr<object>(new name_object("FontDescriptor")));
    fontDescriptorObject->cast<indirect_object>()->follow<dictionary_object>()->add("FontName", std::shared_ptr<object>(new name_object("Dummy")));
    fontDescriptorObject->cast<indirect_object>()->follow<dictionary_object>()->add("Flags", std::shared_ptr<object>(new integer_number(32)));
    fontDescriptorObject->cast<indirect_object>()->follow<dictionary_object>()->add("FontBBox", std::shared_ptr<object>(new Rectangle(0,0,1000,1000)));
    fontDescriptorObject->cast<indirect_object>()->follow<dictionary_object>()->add("ItalicAngle", std::shared_ptr<object>(new integer_number(0)));
    fontDescriptorObject->cast<indirect_object>()->follow<dictionary_object>()->add("Ascent", std::shared_ptr<object>(new integer_number(72)));
    fontDescriptorObject->cast<indirect_object>()->follow<dictionary_object>()->add("Descent", std::shared_ptr<object>(new integer_number(0)));
    fontDescriptorObject->cast<indirect_object>()->follow<dictionary_object>()->add("CapHeight", std::shared_ptr<object>(new integer_number(72)));
    fontDescriptorObject->cast<indirect_object>()->follow<dictionary_object>()->add("StemV", std::shared_ptr<object>(new integer_number(0)));
    fontDescriptorObject->cast<indirect_object>()->follow<dictionary_object>()->add("FontFile3", dummyFontObject);

    cidFontType0Object = add_object(std::shared_ptr<object>(new dictionary_object()));
    cidFontType0Object->cast<indirect_object>()->follow<dictionary_object>()->add("Type", std::shared_ptr<object>(new name_object("Font")));
    cidFontType0Object->cast<indirect_object>()->follow<dictionary_object>()->add("Subtype", std::shared_ptr<object>(new name_object("CIDFontType0")));
    cidFontType0Object->cast<indirect_object>()->follow<dictionary_object>()->add("BaseFont", std::shared_ptr<object>(new name_object("Dummy")));
    cidFontType0Object->cast<indirect_object>()->follow<dictionary_object>()->add("CIDSystemInfo", std::shared_ptr<object>(new dictionary_object({
        {name_object("Registry"), std::shared_ptr<object>(new literal_string("Adobe"))},
        {name_object("Ordering"), std::shared_ptr<object>(new literal_string("Identity"))},
        {name_object("Supplement"), std::shared_ptr<object>(new integer_number(0))},
    })));
    cidFontType0Object->cast<indirect_object>()->follow<dictionary_object>()->add("FontDescriptor", fontDescriptorObject);
}

std::shared_ptr<PageObject> pdf_file::new_page() 
{
    auto newpage = std::shared_ptr<PageObject>(new PageObject());
    auto page = add_object(newpage);
    pages->cast<indirect_object>()->follow<PagesObject>()->add_page(page);
    newpage->cast<PageObject>()->set_parent(pages);
    return newpage;
}

std::shared_ptr<object> pdf_file::add_object(std::shared_ptr<object> obj) {
    body.emplace_back(new indirect_object(body.size()+1, 0, obj));
    trailer.add("Size", std::shared_ptr<object>(new integer_number(body.size()+1)));
    return body.back();
}

std::string pdf_file::dump() const
{
    std::string content;
    std::vector<uint64_t> offsets;
    content += header + "\r\n";
    content += "%\xe2\xe3\xcf\xd3\r\n";
    
    for(const auto &item: body) {
        offsets.push_back(content.size());
        content += item->dump();
    }

    uint64_t cross_reference_offset = content.size();
    content += "xref\r\n";
    content += "0 " + std::to_string(body.size()+1) + "\r\n";
    content += "0000000000 65535 f\r\n";
    for(const auto off: offsets) {
        std::stringstream ss;
        ss.fill('0');
        ss << std::setw(10) << off;
        content += ss.str() + " 00000 n\r\n";
    }

    content += "trailer\r\n";
    content += trailer.dump();
    content += "\r\nstartxref\r\n";
    content += std::to_string(cross_reference_offset) + "\r\n";
    content += "%%EOF\r\n";
    
    return content;
}

std::vector<uint32_t> utf8_to_UCS(std::string s)
{
    std::vector<uint32_t> result;
    uint32_t value;
    int offset = 0;
    int count = 0;
    for(const uint8_t c: s) {
        if(count == 0) {
            if((c & 0x80) == 0) {
                value = c;
                result.push_back(value);
                count = 0;
                offset = 0;
            }
            else if((c & 0xe0) == 0xc0) {
                count = 2;
                offset = 1;
                value = (c & 0x1f) << 6;
            }
            else if((c & 0xf0) == 0xe0) {
                count = 3;
                offset = 1;
                value = (c & 0x0f) << 12;
            }
            else if((c & 0xf8) == 0xf0) {
                count = 4;
                offset = 1;
                value = (c & 0x07) << 18;
            }
            else {
                return result;
            }
        }
        else if(count == 2) {
            if(offset == 1) {
                if((c & 0xc0) == 0x80) {
                    value |= (c & 0x3f);
                    result.push_back(value);
                    count = 0;
                    offset = 0;
                }
                else {
                    return result;
                }
            }
            else {
                return result;
            }
        }
        else if(count == 3) {
            if(offset == 1) {
                if((c & 0xc0) == 0x80) {
                    value |= (c & 0x3f) << 6;
                    offset = 2;
                }
                else {
                    return result;
                }
            }
            else if(offset == 2) {
                if((c & 0xc0) == 0x80) {
                    value |= (c & 0x3f);
                    result.push_back(value);
                    count = 0;
                    offset = 0;
                }
                else {
                    return result;
                }
            }
            else {
                return result;
            }
        }
        else if(count == 4) {
            if(offset == 1) {
                if((c & 0xc0) == 0x80) {
                    value |= (c & 0x3f) << 12;
                    offset = 2;
                }
                else {
                    return result;
                }
            }
            else if(offset == 2) {
                if((c & 0xc0) == 0x80) {
                    value |= (c & 0x3f) << 6;
                    offset = 3;
                }
                else {
                    return result;
                }
            }
            else if(offset == 3) {
                if((c & 0xc0) == 0x80) {
                    value |= (c & 0x3f);
                    result.push_back(value);
                    count = 0;
                    offset = 0;
                }
                else {
                    return result;
                }
            }
            else {
                return result;
            }
        }
    }
    return result;
}

std::vector<uint16_t> UCS_to_utf16(std::vector<uint32_t> s)
{
    std::vector<uint16_t> result;
    for(const auto c: s) {
        if(c <= 0xFFFF) {
            result.push_back(c);
        }
        else if(c >= 0x10000 && c <= 0x10ffff) {
            uint32_t v = c - 0x10000;
            uint16_t w1 = 0xd8 | ((v & 0xffc00) >> 10);
            uint16_t w2 = 0xdc | (v & 0x003ff);
            result.push_back(w1);
            result.push_back(w2);
        }
        else {
            return result;
        }
    }
    return result;
}

std::shared_ptr<PageObject> pdf_file::add_image(const std::string &jpgname, const std::vector<charbox> &box)
{
    std::map<std::string, int> charmap;
    std::map<int, std::string> reversemap;
    int count = 1;
    std::string space_str(" ");
    reversemap[count] = space_str;
    charmap[space_str] = count++;
    for(const auto b: box) {
        if(charmap.count(b.text) > 0) {
            continue;
        }
        reversemap[count] = b.text;
        charmap[b.text] = count++;
    }
    std::vector<std::vector<charbox>> lines;
    int blockidx = -1;
    int lineidx = -1;
    for(const auto b: box) {
        if(b.blockidx != blockidx || b.lineidx != lineidx) {
            blockidx = b.blockidx;
            lineidx = b.lineidx;
            lines.emplace_back();
        }
        lines.back().push_back(b);
    }

    auto newpage = new_page();

    auto jpeg = std::shared_ptr<JpegImageObject>(new JpegImageObject(jpgname));
    auto imageXObject = add_object(jpeg);
    auto dictXObject = std::shared_ptr<object>(new dictionary_object());
    dictXObject->cast<dictionary_object>()->add("Im1", imageXObject);

    std::stringstream ss;
    ss << "q" << std::endl;
    ss << jpeg->width << " 0 0 " << jpeg->height << " 0 0 cm" << std::endl;
    ss << "/Im1 Do" << std::endl;
    ss << "Q" << std::endl;
    ss << "BT" << std::endl;
    ss << "/F1 1 Tf" << std::endl;
    ss << "3 Tr" << std::endl;
    for(const auto &line: lines) {
        ss << std::dec << std::fixed;
        float w = 0;
        float h = 0;
        int boxcount = 0;
        for(const auto &b: line) {
            w = std::max(w, b.w);
            h = std::max(h, b.h);
            boxcount++;
            if(b.subidx > 0 && b.space > 0) {
                boxcount++;
            }
        }
        auto b1 = line.front();
        auto b2 = line.back();
        float s = 0;
        float th = 0;
        float p1x = -1;
        float p1y = -1;
        float p2x = -1;
        float p2y = -1;
        for(const auto &b: line) {
            if(std::max(b.w, b.h) > std::max(w, h) * 0.5) {
                if(p1x < 0) {
                    p1x = b.cx;
                    p1y = b.cy;
                }
                p2x = b.cx;
                p2y = b.cy;
            }
        }
        if(b1.vertical) {
            s = b2.cy + b2.h / 2 - (b1.cy - b1.h / 2);
            if(p1x < 0) {
                th = -90.0 / 180.0 * 3.14159;
            }
            else {
                th = atan2(p2x - p1x, p2y - p1y) - 90.0 / 180.0 * 3.14159;
            }
        }
        else {
            s = b2.cx + b2.w / 2 - (b1.cx - b1.w / 2);
            if(p1x < 0) {
                th = 0;
            }
            else {
                th = -atan2(p2y - p1y, p2x - p1x);
            }
        }
        s /= boxcount;
        if(b1.vertical) {
            s /= h;
        }
        else {
            s /= w;
        }
        int tz = s * 100;
        float tx = b1.cx - b1.w / 2;
        if(b1.vertical) {
            float ty = jpeg->height - (b1.cy - b1.h / 2);
            ss << h * cosf(th);
            ss << " ";
            ss << h * sinf(th);
            ss << " ";
            ss << -h * sinf(th);
            ss << " ";
            ss << h * cosf(th);
            ss << " ";
            ss << tx;
            ss << " ";
            ss << ty;
            ss << " ";
            ss << "Tm" << std::endl;
        }
        else {
            float ty = jpeg->height - (b1.cy + b1.h / 2);
            ss << w * cosf(th);
            ss << " ";
            ss << w * sinf(th);
            ss << " ";
            ss << -w * sinf(th);
            ss << " ";
            ss << w * cosf(th);
            ss << " ";
            ss << tx;
            ss << " ";
            ss << ty;
            ss << " ";
            ss << "Tm" << std::endl;
        }
        ss << tz << " " << "Tz" << std::endl;
        ss << "<";
        auto prevfill = ss.fill();
        ss.fill('0');
        for(const auto &b: line) {
            if(b.subidx > 0 && b.space > 0) {
                ss << std::setw(4) << std::hex << charmap[space_str];
            }
            ss << std::setw(4) << std::hex << charmap[b.text];
        }
        ss.fill(prevfill);
        ss << "> Tj" << std::endl;
    }
    ss << "ET" << std::endl;
    std::string str = ss.str();
    std::vector<uint8_t> stream(str.begin(), str.end());
    auto content = add_object(std::shared_ptr<object>(new stream_object(stream)));
    newpage->set_contents(content);
    newpage->set_mediabox(0,0,jpeg->cast<JpegImageObject>()->width,jpeg->cast<JpegImageObject>()->height);
    newpage->operator[]<dictionary_object>("Resources")->add("XObject", dictXObject);

    auto type0FontObject = add_object(std::shared_ptr<object>(new dictionary_object()));
    type0FontObject->cast<indirect_object>()->follow<dictionary_object>()->add("Type", std::shared_ptr<object>(new name_object("Font")));
    type0FontObject->cast<indirect_object>()->follow<dictionary_object>()->add("Subtype", std::shared_ptr<object>(new name_object("Type0")));
    type0FontObject->cast<indirect_object>()->follow<dictionary_object>()->add("BaseFont", std::shared_ptr<object>(new name_object("Dummy")));
    type0FontObject->cast<indirect_object>()->follow<dictionary_object>()->add("Encoding", std::shared_ptr<object>(new name_object("Identity-H")));
    type0FontObject->cast<indirect_object>()->follow<dictionary_object>()->add("DescendantFonts", std::shared_ptr<object>(new array_object({cidFontType0Object})));
    {
        std::stringstream ss;
        ss << "/CIDInit /ProcSet findresource begin" << std::endl;
        ss << "12 dict begin" << std::endl;
        ss << "begincmap" << std::endl;
        ss << "/CIDSystemInfo <<" << std::endl;
        ss << " /Registry (Adobe) def" << std::endl;
        ss << " /Ordering (Identity) def" << std::endl;
        ss << " /Supplement 0 def" << std::endl;
        ss << ">> def" << std::endl;
        ss << "/CMapName /Identity-H def" << std::endl;
        ss << "/CMapType 1 def" << std::endl;
        ss << "1 begincodespacerange" << std::endl;
        ss << " <0000> <FFFF>" << std::endl;
        ss << "endcodespacerange" << std::endl;
        ss << std::endl;

        for(int i = 1; i < count; i+=100) {
            int j = (count - i > 100) ? 100 : count - i;
            ss << std::dec << j << " beginbfchar" << std::endl;
            for(int k = i; k < i+j; k++) {
                auto prevfill = ss.fill();
                ss.fill('0');
                ss << " <";
                ss << std::setw(4) << std::hex << k;
                ss << "> <";
                auto utf16str = UCS_to_utf16(utf8_to_UCS(reversemap[k]));
                for(const auto c: utf16str) {
                    ss << std::setw(4) << std::hex << c;
                }
                ss << ">" << std::endl;
                ss.fill(prevfill);
            }
            ss << "endbfchar" << std::endl;
            ss << std::endl;
        }

        ss << "endcmap" << std::endl;
        ss << "CMapName currentdict /CMap defineresource pop" << std::endl;
        ss << "end" << std::endl;
        ss << "end" << std::endl;

        std::string str = ss.str();
        std::vector<uint8_t> stream(str.begin(), str.end());
        auto toUnicodeObject = add_object(std::shared_ptr<object>(new stream_object(stream)));        
        type0FontObject->cast<indirect_object>()->follow<dictionary_object>()->add("ToUnicode", toUnicodeObject);
    }
    auto fontObject = std::shared_ptr<object>(new dictionary_object());
    fontObject->cast<dictionary_object>()->add("F1", type0FontObject);
    newpage->operator[]<dictionary_object>("Resources")->add("Font", fontObject);

    return newpage;
}

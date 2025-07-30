#include <numeric>
#include <filesystem>

#include "pdf.hpp"
#include "crypt.hpp"
#include "fax.hpp"

#include "jpeglib.h"
#include <zlib.h>
#include <png.h>

extern "C" {
#include "transupp.h"
}

std::ostream& operator<<(std::ostream& os, const object* obj)
{
    os << obj->output();
    return os;
}

std::ostream& operator<<(std::ostream& os, const object &obj)
{
    return operator<<(os, &obj);
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
        auto dict = _dict;
        if(dict.operator[]<integer_number>("Length") == nullptr) {
            auto length = _stream.size();
            dict.add("Length", std::shared_ptr<object>(new integer_number(length)));
        }
        else {
            auto length = _stream.size();
            dict.operator[]<integer_number>("Length")->set_value(length);
        }

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
    root->follow<RootObject>()->add_pages(pages);
    trailer.add("Root", root);

    prepare_font();
}

extern std::vector<uint8_t> dummyFont;

void pdf_file::prepare_font()
{
    auto dummyFontObject = add_object(std::shared_ptr<object>(new stream_object(dummyFont)));
    dummyFontObject->follow<stream_object>()->get_dict().add("Subtype", std::shared_ptr<object>(new name_object("CIDFontType0C")));

    auto fontDescriptorObject = add_object(std::shared_ptr<object>(new dictionary_object()));
    fontDescriptorObject->follow<dictionary_object>()->add("Type", std::shared_ptr<object>(new name_object("FontDescriptor")));
    fontDescriptorObject->follow<dictionary_object>()->add("FontName", std::shared_ptr<object>(new name_object("Dummy")));
    fontDescriptorObject->follow<dictionary_object>()->add("Flags", std::shared_ptr<object>(new integer_number(32)));
    fontDescriptorObject->follow<dictionary_object>()->add("FontBBox", std::shared_ptr<object>(new Rectangle(0,0,1000,1000)));
    fontDescriptorObject->follow<dictionary_object>()->add("ItalicAngle", std::shared_ptr<object>(new integer_number(0)));
    fontDescriptorObject->follow<dictionary_object>()->add("Ascent", std::shared_ptr<object>(new integer_number(72)));
    fontDescriptorObject->follow<dictionary_object>()->add("Descent", std::shared_ptr<object>(new integer_number(0)));
    fontDescriptorObject->follow<dictionary_object>()->add("CapHeight", std::shared_ptr<object>(new integer_number(72)));
    fontDescriptorObject->follow<dictionary_object>()->add("StemV", std::shared_ptr<object>(new integer_number(0)));
    fontDescriptorObject->follow<dictionary_object>()->add("FontFile3", dummyFontObject);

    cidFontType0Object = add_object(std::shared_ptr<object>(new dictionary_object()));
    cidFontType0Object->follow<dictionary_object>()->add("Type", std::shared_ptr<object>(new name_object("Font")));
    cidFontType0Object->follow<dictionary_object>()->add("Subtype", std::shared_ptr<object>(new name_object("CIDFontType0")));
    cidFontType0Object->follow<dictionary_object>()->add("BaseFont", std::shared_ptr<object>(new name_object("Dummy")));
    cidFontType0Object->follow<dictionary_object>()->add("CIDSystemInfo", std::shared_ptr<object>(new dictionary_object({
        {name_object("Registry"), std::shared_ptr<object>(new literal_string("Adobe"))},
        {name_object("Ordering"), std::shared_ptr<object>(new literal_string("Identity"))},
        {name_object("Supplement"), std::shared_ptr<object>(new integer_number(0))},
    })));
    cidFontType0Object->follow<dictionary_object>()->add("FontDescriptor", fontDescriptorObject);
}

std::shared_ptr<PageObject> pdf_file::new_page() 
{
    auto newpage = std::shared_ptr<PageObject>(new PageObject());
    auto page = add_object(newpage);
    pages->follow<PagesObject>()->add_page(page);
    newpage->set_parent(pages);
    return newpage;
}

std::shared_ptr<indirect_object> pdf_file::add_object(std::shared_ptr<object> obj) {
    int newidx = 1;
    for(const auto i: bodyidx) {
        newidx = std::max(newidx, i+1);
    }
    body.emplace_back(new indirect_object(newidx, 0, obj));
    bodyidx.push_back(newidx);
    trailer.add("Size", std::shared_ptr<object>(new integer_number(newidx)));
    return body.back();
}

std::string pdf_file::dump() const
{
    std::string content;
    std::vector<uint64_t> offsets;
    int maxidx = 1;
    for(const auto i: bodyidx) {
        maxidx = std::max(maxidx, i+1);
    }
    offsets.resize(maxidx);

    content += header + "\r\n";
    content += "%\xe2\xe3\xcf\xd3\r\n";
    
    for(int i = 0; i < bodyidx.size(); i++) {
        offsets[bodyidx[i]] = content.size();
        content += body[i]->dump();
    }

    size_t cross_reference_offset = content.size();
    content += "xref\r\n";
    content += "0 " + std::to_string(maxidx) + "\r\n";
    content += "0000000000 65535 f\r\n";
    for(int i = 1; i < offsets.size(); i++) {
        std::stringstream ss;
        ss.fill('0');
        ss << std::setw(10) << offsets[i];
        if(offsets[i] > 0) {
            content += ss.str() + " 00000 n\r\n";
        }
        else {
            content += ss.str() + " 00000 f\r\n";
        }
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
        if(b.ruby) continue;
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
    auto dictXObject = std::shared_ptr<dictionary_object>(new dictionary_object());
    dictXObject->add("Im1", imageXObject);

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
            if(b.text == ".") continue;
            if(b.text == ",") continue;
            if(b.text == "'") continue;
            if(b.text == "\"") continue;
            if(b.text == "、") continue;
            if(b.text == "。") continue;
            if(b.text == "“") continue;
            if(b.text == "”") continue;
            if(p1x < 0) {
                p1x = b.cx;
                p1y = b.cy;
            }
            p2x = b.cx;
            p2y = b.cy;
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
        if(b1.vertical) {
            float tx = p1x - w / 2;
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
            float tx = b1.cx - b1.w / 2;
            float ty = jpeg->height - (p1y + h / 4);
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
    newpage->set_mediabox(0,0,jpeg->width,jpeg->height);
    newpage->operator[]<dictionary_object>("Resources")->add("XObject", dictXObject);

    auto type0FontObject = add_object(std::shared_ptr<object>(new dictionary_object()));
    type0FontObject->follow<dictionary_object>()->add("Type", std::shared_ptr<object>(new name_object("Font")));
    type0FontObject->follow<dictionary_object>()->add("Subtype", std::shared_ptr<object>(new name_object("Type0")));
    type0FontObject->follow<dictionary_object>()->add("BaseFont", std::shared_ptr<object>(new name_object("Dummy")));
    type0FontObject->follow<dictionary_object>()->add("Encoding", std::shared_ptr<object>(new name_object("Identity-H")));
    type0FontObject->follow<dictionary_object>()->add("DescendantFonts", std::shared_ptr<object>(new array_object({cidFontType0Object})));
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
        type0FontObject->follow<dictionary_object>()->add("ToUnicode", toUnicodeObject);
    }
    auto fontObject = std::shared_ptr<dictionary_object>(new dictionary_object());
    fontObject->add("F1", type0FontObject);
    newpage->operator[]<dictionary_object>("Resources")->add("Font", fontObject);

    return newpage;
}

pdf_file::pdf_file(const std::string &filename)
{
    std::ifstream ifs(filename, std::ios::binary);
    std::string line;
    std::getline(ifs, line);
    header = line.substr(0, 8);

    std::streamoff offset = -1;
    ifs.seekg(offset, std::ios_base::end);
    while(ifs.peek() != '%') {
        ifs.seekg(--offset, std::ios_base::end);
    }
    offset -= 2;
    ifs.seekg(offset, std::ios_base::end);
    while(std::isspace(ifs.peek())) {
        ifs.seekg(--offset, std::ios_base::end);
    }
    offset -= 1;
    while(!std::isspace(ifs.peek())) {
        ifs.seekg(--offset, std::ios_base::end);
    }
    size_t cross_reference_offset;
    ifs >> cross_reference_offset;

    std::vector<int64_t> body_offsets;
    while(true) {
        ifs.seekg(cross_reference_offset, std::ios_base::beg);
        size_t trailer_offset = ifs.tellg();
        while(std::getline(ifs, line)) {
            if(line.substr(0,7) == "trailer") {
                ifs.seekg(trailer_offset, std::ios_base::beg);
                ifs.ignore(7);
                break;
            }
            if(line.substr(0,4) == "xref") {
                if(!std::getline(ifs, line)) break;
            }
            size_t start, length;
            std::stringstream ss(line);
            ss >> start >> length;
            body_offsets.resize(std::max(start + length + 1, body_offsets.size()));
            for(int i = start; i < start + length; i++) {
                int64_t off;
                int gen;
                char state;
                if(!std::getline(ifs, line)) break;
                std::stringstream ss1(line);
                ss1 >> off >> gen >> state;
                if(state == 'n' && body_offsets[i] == 0) {
                    body_offsets[i] = off;
                }
            }
            trailer_offset = ifs.tellg();
        }

        auto tobject = parse_object(ifs);
        if(tobject) {
            auto trailer_object = std::dynamic_pointer_cast<dictionary_object>(tobject);
            if(trailer_object) {
                auto keys = trailer_object->keys();
                for(const auto &key: keys) {
                    if(trailer.operator[]<object>(key) == nullptr) {
                        trailer.add(key, trailer_object->value(key));
                    }
                }
                auto prev = trailer_object->operator[]<integer_number>("Prev");
                if(prev) {
                    cross_reference_offset = prev->get_value();
                    continue;
                }
            }
        }
        break;
    }

    auto encrypt_obj = trailer.operator[]<indirect_object>("Encrypt");
    if(encrypt_obj) {
        ifs.seekg(body_offsets[encrypt_obj->number()], std::ios_base::beg);
        auto obj = parse_object(ifs);
        if(obj) {
            auto dict = std::dynamic_pointer_cast<indirect_object>(obj)->follow<dictionary_object>();
            if(dict) {
                encrypt = std::shared_ptr<object>(new crypt_object(*dict));
                trailer.remove("Encrypt");
            }
        }
    }

    std::vector<int> body_offsets_idx(body_offsets.size());
    std::iota(body_offsets_idx.begin(), body_offsets_idx.end(), 0);
    std::sort(body_offsets_idx.begin(), body_offsets_idx.end(), [&](auto a, auto b){
        return body_offsets[a] < body_offsets[b];
    });

    for(auto it = body_offsets_idx.begin(); it != body_offsets_idx.end(); ++it) {
        if(body_offsets[*it] > 0) {
            // std::cout << "read obj " << *it << " offset " << body_offsets[*it] << std::endl;
            ifs.seekg(body_offsets[*it], std::ios_base::beg);
            auto obj = parse_object(ifs);
        }
    }

    std::sort(body.begin(), body.end(), [](auto a, auto b){
        return a->number() < b->number();
    });
    std::sort(bodyidx.begin(), bodyidx.end());

    auto root_obj = trailer.operator[]<indirect_object>("Root");
    if(root_obj) {
        root = root_obj;
        auto dict = root_obj->follow<dictionary_object>();
        root_obj->set(std::shared_ptr<RootObject>(new RootObject(*dict)));
    }

    auto pages_obj = root_obj->follow<RootObject>()->operator[]<indirect_object>("Pages");
    if(pages_obj) {
        pages = pages_obj;
        auto dict = pages_obj->follow<dictionary_object>();
        pages_obj->set(std::shared_ptr<PagesObject>(new PagesObject(*dict)));
    }
}

bool is_whitespace(char c)
{
    return c == 0x00 || c == 0x09 || c == 0x0a || c == 0x0c || c == 0x0d || c == 0x20;
}

bool is_delimiter(char c)
{
    return c == '(' || c == ')' || c == '<' || c == '>' || c == '[' || c == ']' || c == '%';

}

std::shared_ptr<hexadecimal_string> pdf_file::parse_hexstring(std::istream &ss)
{
    std::string result = "";
    int c = 0;
    int v1 = -1;
    while(ss) {
        c = ss.get();
        if(!ss) break;
        if(c == '<') continue;
        if(c == '>') break;
        if(is_whitespace(c)) continue;
        int v;
        if(c >= '0' && c <= '9') {
            v = c - '0';
        }
        else if(c >= 'A' && c <= 'F') {
            v = c - 'A' + 10;
        }
        else if(c >= 'a' && c <= 'f') {
            v = c - 'a' + 10;
        }
        else {
            throw std::runtime_error("invalid hexstring");
        }
        if (v1 < 0) {
            v1 = v;
        }
        else {
            result += (v1 << 4) | v;
            v1 = -1;
        }
    }
    if(v1 >= 0) {
        result += (v1 << 4);
    }

    if(encrypt) {
        result = std::dynamic_pointer_cast<crypt_object>(encrypt)->decode_string(result);
    }

    return std::shared_ptr<hexadecimal_string>(new hexadecimal_string(result));
}

std::shared_ptr<literal_string> pdf_file::parse_literal(std::istream &ss)
{
    std::string result = "";
    bool isescape = false;
    bool isescapeCR = false;
    std::string octalstr = "";
    while(ss) {
        char c = ss.get();
        if(!ss) break;
        if(isescapeCR && c == 0x0a) {
            isescapeCR = false;
            continue;
        }
        if(!octalstr.empty() && c >= '0' && c <= '7') {
            octalstr += c;
            continue;
        }
        if(!octalstr.empty()) {
            int v = 0;
            for(auto o: octalstr) {
                int v2 = o - '0';
                v = (v << 3) | v2;
            }
            result += v;
            octalstr.clear();
        }
        if(isescape) {
            isescape = false;
            if(c == 'n') {
                c = 0x0a;
            }
            else if(c == 'r') {
                c = 0x0d;
            }
            else if(c == 't') {
                c = 0x09;
            }
            else if(c == 'b') {
                c = 0x08;
            }
            else if(c == 'f') {
                c = 0x0c;
            }
            else if(c == '(') {
                c = 0x28;
            }
            else if(c == ')') {
                c = 0x29;
            }
            else if(c == '\\') {
                c = 0x5c;
            }
            else if(c >= '0' && c <= '7') {
                octalstr += c;
            }
            else if(c == 0x0d) {
                c = 0x0a;
                isescapeCR = true;
            }
            else if(c == 0x0a) {
                c = 0x0a;
            }
        }
        else if(c == '\\') {
            isescape = true;
            continue;
        }
        result += c;
    }

    if(encrypt) {
        result = std::dynamic_pointer_cast<crypt_object>(encrypt)->decode_string(result);
    }

    return std::shared_ptr<literal_string>(new literal_string(result));
}

std::shared_ptr<name_object> pdf_file::parse_name(std::istream &ss)
{
    char c = ss.get();
    while(c != '/') {
        c = ss.get();
    }
    std::string result = "";
    while(ss) {
        c = ss.get();
        if(!ss) break;
        if(is_whitespace(c)) break;
        if(is_delimiter(c) || c == '/') {
            ss.unget();
            break;
        }
        if(c == '#') {
            c = ss.get();
            int v1;
            if(c >= '0' && c <= '9') {
                v1 = c - '0';
            }
            else if(c >= 'A' && c <= 'F') {
                v1 = c - 'A' + 10;
            }
            else if(c >= 'a' && c <= 'f') {
                v1 = c - 'a' + 10;
            }
            c = ss.get();
            int v2;
            if(c >= '0' && c <= '9') {
                v2 = c - '0';
            }
            else if(c >= 'A' && c <= 'F') {
                v2 = c - 'A' + 10;
            }
            else if(c >= 'a' && c <= 'f') {
                v2 = c - 'a' + 10;
            }
            result += (v1 << 4) | v2;
        }
        else {
            result += c;
        }
    }
    return std::shared_ptr<name_object>(new name_object(result));
}

std::shared_ptr<object> pdf_file::parse_numeric(std::istream &is, std::istream &ss)
{
    std::string numstr;
    int count = 0;
    while(is) {
        int c = is.get();
        if(is) {
            if((c >= '0' && c <= '9') || c == '.' || c == '+' || c == '-') {
                numstr += char(c);
            }
            else if (numstr.empty() && is_whitespace(c)) {
                continue;
            }
            else {
                count++;
                break;
            }
        }
    }
    while(is) {
        int c = is.get();
        if(is) {
            count++;
        }
    }
    ss.seekg(-count, std::ios::cur);
    if(numstr.find('.') == std::string::npos) {
        int value;
        std::stringstream(numstr) >> value;
        return std::shared_ptr<object>(new integer_number(value));
    }
    else {
        double value;
        std::stringstream(numstr) >> value;
        return std::shared_ptr<object>(new real_number(value));
    }
}

std::shared_ptr<array_object> pdf_file::parse_array(std::stringstream &ss)
{
    ss.seekg(-1, std::ios::end);
    ss << ' ';
    ss.seekg(1, std::ios::beg);
    std::vector<std::shared_ptr<object>> arrayobj;
    while(ss) {
        if(is_whitespace(ss.peek())) {
            ss.get();
        }
        auto obj = parse_object(ss);
        if(obj) {
            arrayobj.push_back(obj);
        }
    }
    return std::shared_ptr<array_object>(new array_object(arrayobj));
}

std::shared_ptr<dictionary_object> pdf_file::parse_dictionary(std::stringstream &ss)
{
    ss.seekg(0, std::ios::beg);
    int count = 2;
    while(ss.get() != '<' || --count > 0);
    auto dictobj = std::shared_ptr<dictionary_object>(new dictionary_object());
    while(ss) {
        std::shared_ptr<object> key = parse_object(ss);
        std::shared_ptr<object> value = parse_object(ss);
        if(key && value) {
            auto name_key = std::dynamic_pointer_cast<name_object>(key);
            if(name_key != nullptr) {
                dictobj->add(name_key->get_value(), value);
            }
        }
    }
    return dictobj;
}

std::shared_ptr<object> pdf_file::parse_object(std::istream &ss)
{
    enum state {
        array,
        hexstring,
        dictionary,
        literalstring,
        name,
        escape,
        comment,
        maybeobj1,
        maybeobj2,
        maybeobj3,
        numeric
    };

    std::stringstream bs;
    std::vector<state> current_state;
    std::string keyword_buffer;

    while(ss) {
        char c = ss.get();
        if(!ss) break;
        bs << c;

        // std::string outstr;
        // for(auto cstate: current_state) {
        //     outstr += std::to_string(cstate) + " ";
        // }
        // if(outstr.empty()) {
        //     outstr = "(empty)";
        // }
        // if(c >= 0x20 && c < 0x7f) {
        //     std::cout << c << " " << outstr << std::endl;
        // }
        // else {
        //     int ci = *(unsigned char *)&c;
        //     std::cout << "b" << ci << " " << outstr << std::endl;
        // }

    retry: ;
        if (!current_state.empty() && current_state.back() == escape) {
            current_state.pop_back();
            continue;
        }
        else if (c == '\\') {
            keyword_buffer.clear();
            current_state.push_back(escape);            
        }
        else if (!current_state.empty() && current_state.back() == comment) {
            if(c == '\r') {
                if(ss.peek() == '\n') {
                    c = ss.get();
                }
                current_state.pop_back();
                bs = std::stringstream();
                continue;
            }
            else if (c == '\n') {
                current_state.pop_back();
                bs = std::stringstream();
                continue;
            }
        }
        else if (!current_state.empty() && current_state.back() == literalstring && c == ')') {
            current_state.pop_back();
            if(current_state.empty()) {
                std::string buf = bs.str();
                buf = buf.substr(1, buf.size()-2);
                std::stringstream bs2(buf);
                return parse_literal(bs2);
            }                        
        }
        else if (!current_state.empty() && current_state.back() == literalstring) {
            // ignore spetials
        }
        else if (c == '%') {
            keyword_buffer.clear();
            current_state.push_back(comment);
        }
        else if (!current_state.empty() && current_state.back() == name && (is_whitespace(c) || is_delimiter(c) || c == '/')) {
            current_state.pop_back();
            if(current_state.empty()) {
                if(is_delimiter(c) || c == '/') {
                    ss.unget();
                    std::string buf = bs.str();
                    buf = buf.substr(0, buf.size()-1);
                    std::stringstream bs2(buf);
                    return parse_name(bs2);
                }
                return parse_name(bs);
            }
            goto retry;
        }
        else if (!current_state.empty() && current_state.back() == numeric && std::string("0123456789+-.").find_first_of(c) == std::string::npos) {
            current_state.pop_back();
            if(current_state.empty()) {
                return parse_numeric(bs, ss);
            }
            goto retry;
        }
        else if (!current_state.empty() && current_state.back() == array && c == ']') {
            current_state.pop_back();
            if(current_state.empty()) {
                return parse_array(bs);
            }
        }
        else if (!current_state.empty() && current_state.back() == hexstring && c == '>') {
            current_state.pop_back();
            if(current_state.empty()) {
                return parse_hexstring(bs);
            }            
        }
        else if (!current_state.empty() && current_state.back() == dictionary && c == '>') {
            c = ss.get();
            bs << c;

            if(c == '>') {
                current_state.pop_back();
                if(current_state.empty()) {
                    return parse_dictionary(bs);
                }
            }
        }
        else if (!current_state.empty() && current_state.back() == maybeobj1 && is_whitespace(c)) {
            current_state.back() = maybeobj2;
        }
        else if (!current_state.empty() && current_state.back() == maybeobj1 && c == '.') {
            current_state.back() = numeric;
        }
        else if (!current_state.empty() && current_state.back() == maybeobj1 && std::string("0123456789").find_first_of(c) == std::string::npos) {
            current_state.pop_back();
            if(current_state.empty()) {
                return parse_numeric(bs, ss);
            }
        }
        else if (!current_state.empty() && current_state.back() == maybeobj2 && is_whitespace(c)) {
            current_state.back() = maybeobj3;
        }
        else if (!current_state.empty() && current_state.back() == maybeobj2 && std::string("0123456789").find_first_of(c) == std::string::npos) {
            current_state.pop_back();
            if(current_state.empty()) {
                return parse_numeric(bs, ss);
            }
        }
        else if (!current_state.empty() && current_state.back() == maybeobj3 && c == 'R') {
            current_state.pop_back();
            int obj_num, gen_num;
            std::stringstream(bs.str()) >> obj_num >> gen_num;
            return ref_object(obj_num);
        }
        else if (!current_state.empty() && current_state.back() == maybeobj3 && c == 'o') {
            current_state.pop_back();
            int obj_num, gen_num;
            std::string str3;
            std::stringstream(bs.str()) >> obj_num >> gen_num;
            ss.ignore(2);
            return register_object(ss, obj_num);
        }
        else if (!current_state.empty() && current_state.back() == maybeobj3) {
            current_state.pop_back();
            if(current_state.empty()) {
                return parse_numeric(bs, ss);
            }
        }
        else if (c == '[') {
            keyword_buffer.clear();
            current_state.push_back(array);
        }
        else if (!current_state.empty() && current_state.back() == hexstring && c == '<') {
            current_state.back() = dictionary;
        }
        else if (c == '<') {
            keyword_buffer.clear();
            current_state.push_back(hexstring);            
        }
        else if (c == '(') {
            keyword_buffer.clear();
            current_state.push_back(literalstring);            
        }
        else if (c == '/') {
            keyword_buffer.clear();
            current_state.push_back(name);
        }
        else if (current_state.empty()){
            if(!is_whitespace(c)) {
                keyword_buffer += c;
            }
            else {
                continue;
            }

            if(keyword_buffer == "null") {
                return std::shared_ptr<object>(new null_object());
            }
            if(keyword_buffer == "true") {
                return std::shared_ptr<object>(new boolean_value(true));
            }
            if(keyword_buffer == "false") {
                return std::shared_ptr<object>(new boolean_value(false));
            }
            
            if(keyword_buffer == "xref") {
                return nullptr;
            }
            if(keyword_buffer == "startxref") {
                return nullptr;
            }

            if(c == '+' || c == '-') {
                current_state.push_back(numeric);
            }
            else if(std::string("0123456789").find_first_of(c) != std::string::npos) {
                current_state.push_back(maybeobj1);
            }
        }
    }
    return nullptr;
}

std::shared_ptr<indirect_object> pdf_file::ref_object(int objnum)
{
    if(objnum == 0) return nullptr;

    auto it = std::find(bodyidx.begin(), bodyidx.end(), objnum);
    if(it == bodyidx.end()) {
        bodyidx.push_back(objnum);
        body.emplace_back(new indirect_object(objnum));
        int maxidx = 1;
        for(auto i: bodyidx) {
            maxidx = std::max(i+1, maxidx);
        }
        trailer.add("Size", std::shared_ptr<object>(new integer_number(maxidx)));
        it = std::find(bodyidx.begin(), bodyidx.end(), objnum);
    }
    auto idx = std::distance(bodyidx.begin(), it);
    return body[idx];
}

std::shared_ptr<indirect_object> pdf_file::register_object(std::istream &ss, int obj_num)
{
    if(ref_object(obj_num)->isLoaded()) {
        return ref_object(obj_num);
    }

    auto obj = parse_object(ss);
    if(!obj) throw std::runtime_error("parse error");

    // std::cout << "register_object " << obj_num << std::endl;
    // obj->print();
    // std::cout << std::endl;

    std::string keyword;
    ss >> keyword;
    if(keyword.substr(0, 6) == "endobj") {
        auto indirectobj = ref_object(obj_num);
        indirectobj->set(obj);
        return indirectobj;
    }
    if(keyword.substr(0, 6) == "stream") {
        auto dict = std::dynamic_pointer_cast<dictionary_object>(obj);
        if(dict) {
            auto length = dict->operator[]<integer_number>("Length");
            if(!length) {
                length = ref_object(dict->operator[]<indirect_object>("Length")->number())->follow<integer_number>();
            }
            if(length) {
                if(ss.peek() == 0x0d) {
                    ss.ignore(2);
                }
                else if (ss.peek() == 0x0a) {
                    ss.ignore();
                }
                std::vector<uint8_t> stream(length->get_value());
                ss.read((char *)stream.data(), stream.size());
                if(ss.peek() == 0x0d) {
                    ss.ignore(2);
                }
                else if (ss.peek() == 0x0a) {
                    ss.ignore();
                }
                ss >> keyword;
                if(keyword.substr(0, 9) == "endstream") {
                    ss >> keyword;
                    if(keyword.substr(0, 6) == "endobj") {
                        if(encrypt) {
                            stream = std::dynamic_pointer_cast<crypt_object>(encrypt)->decode_stream(stream);
                        }

                        auto streamobj = std::shared_ptr<object>(new stream_object(*dict, stream));
                        auto indirectobj = ref_object(obj_num);
                        indirectobj->set(streamobj);
                        return indirectobj;
                    }
                }
            }
        }
    }
    return nullptr;
}

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

std::vector<uint8_t> FlateDecode(const std::vector<uint8_t> &stream)
{
    std::vector<uint8_t> result;
    std::vector<uint8_t> outBuf(0x4000);
    z_stream zStream{ 0 };
    inflateInit(&zStream);

    zStream.avail_in = stream.size();
    zStream.next_in = const_cast<uint8_t*>(stream.data());
    do {
        zStream.next_out = outBuf.data();
        zStream.avail_out = outBuf.size();
        inflate(&zStream, Z_NO_FLUSH);
        auto outSize = outBuf.size() - zStream.avail_out;
        std::copy(outBuf.begin(), outBuf.begin() + outSize, std::back_inserter(result));
    } while (zStream.avail_out == 0);

    inflateEnd(&zStream);

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
        else if(filter->get_value() == "FlateDecode") {
            stream = FlateDecode(stream);
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
                return;
            }

            if(filter->get_value() == "ASCIIHexDecode") {
                stream = ASCIIHexDecode(stream);
                filters->remove(i);
            }
            else if(filter->get_value() == "ASCII85Decode") {
                stream = ASCII85Decode(stream);
                filters->remove(i);
            }
            else if(filter->get_value() == "FlateDecode") {
                stream = FlateDecode(stream);
                if(decodeParmss) {
                    decodeParms = decodeParmss->operator[]<dictionary_object>(i);
                    if(decodeParms) {
                        decodeParmss->remove(i);
                    }
                }
                filters->remove(i);
            }
            else {
                i++;
            }
        }
    }
}

void output_jpeg(const std::string &target_path, int page_no, const std::string &key, const std::vector<uint8_t> &data, int rotate = 0)
{
    std::filesystem::path path(target_path);
    std::stringstream ss;
    ss.fill('0');
    ss << "page" << std::setw(4) << page_no;
    path /= ss.str() + "_" + key + ".jpg";

    {
        std::ofstream ofs(path, std::ios::binary);
        ofs.write((const char *)data.data(), data.size());
    }

    if(rotate == 0) return;

    struct jpeg_decompress_struct srcObj;
    struct jpeg_error_mgr srcErrMgr;
    
    struct jpeg_compress_struct destObj;
    struct jpeg_error_mgr destErrMgr;

    jpeg_transform_info transformoption;
    if(rotate == 90) {
        transformoption.transform = JXFORM_ROT_90;
    }
    else if(rotate == 180) {
        transformoption.transform = JXFORM_ROT_180;
    }
    else if(rotate == 270) {
        transformoption.transform = JXFORM_ROT_270;
    }
    transformoption.perfect = FALSE;
    transformoption.trim = FALSE;
    transformoption.force_grayscale = FALSE;
    transformoption.crop = FALSE;
    transformoption.slow_hflip = FALSE;

    srcObj.err = jpeg_std_error(&srcErrMgr);
    jpeg_create_decompress(&srcObj);

    destObj.err = jpeg_std_error(&destErrMgr);
    jpeg_create_compress(&destObj);

    FILE *inputFile = fopen(path.c_str(), "rb");
    jpeg_stdio_src(&srcObj, inputFile);
    JCOPY_OPTION copyOpt = JCOPYOPT_ALL;

    jcopy_markers_setup(&srcObj, copyOpt);

    (void)jpeg_read_header(&srcObj, TRUE);

    jtransform_request_workspace(&srcObj, &transformoption);

    auto srcCoefArr = jpeg_read_coefficients(&srcObj);
    jpeg_copy_critical_parameters(&srcObj, &destObj);

    auto destCoefArr = jtransform_adjust_parameters(&srcObj, &destObj, srcCoefArr, &transformoption);

    fclose(inputFile);

    FILE *outputFile = fopen(path.c_str(), "wb");
    jpeg_stdio_dest(&destObj, outputFile);
    
    jpeg_write_coefficients(&destObj, destCoefArr);

    jcopy_markers_execute(&srcObj, &destObj, copyOpt);

    jtransform_execute_transformation(&srcObj, &destObj, srcCoefArr, &transformoption);

    jpeg_finish_compress(&destObj);
    jpeg_destroy_compress(&destObj);

    (void)jpeg_finish_decompress(&srcObj);
    jpeg_destroy_decompress(&srcObj);

    fclose(outputFile);
}

void output_gray8(const std::string &target_path, int page_no, const std::string &key, const std::vector<uint8_t> &data, int width, int height, int rotate = 0)
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
        const uint8_t *datap = data.data();
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
            *row++ = data[(width - 1 - x)*height + y];
            }
        }
    }
    else if(rotate == 180) {
        for (int y = 0; y < height; y++) {
            png_bytep row = rows[y];
            for (int x = 0; x < width; x++) {
            *row++ = data[(height - 1 - y)*width + x];
            }
        }
    }
    else if(rotate == 270) {
        for (int y = 0; y < height; y++) {
            png_bytep row = rows[y];
            for (int x = 0; x < width; x++) {
            *row++ = data[x * height + (height - 1 - y)];
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

int pdf_file::extract_images(const std::string &target_path)
{
    auto tree = pages->follow<PagesObject>();
    int page_no = 1;
    int count = 0;
    for(auto it = tree->begin(); it != tree->end(); ++it, ++page_no) {
        auto res = (*it)->operator[]<dictionary_object>("Resources");
        if(!res) continue;
        auto rotateobj = (*it)->operator[]<integer_number>("Rotate");
        int rotate = 0;
        if(rotateobj) {
            rotate = rotateobj->get_value();
        }
        auto xobject = res->operator[]<dictionary_object>("XObject");
        if(!xobject) continue;

        for(auto key: xobject->keys()) {
            auto item = xobject->operator[]<indirect_object>(key)->follow<stream_object>();
            auto subtype = item->get_dict().operator[]<name_object>("Subtype");
            if(subtype && subtype->get_value() == "Image") {
                std::cout << page_no << ":" << key << std::endl;

                auto filter = item->get_dict().operator[]<object>("Filter");
                if(filter) {
                    auto afilter = std::dynamic_pointer_cast<name_object>(filter);
                    if(afilter && afilter->get_value() == "DCTDecode") {
                        output_jpeg(target_path, page_no, key, item->get_stream(), rotate);
                        count++;
                    }
                    if(afilter && afilter->get_value() == "CCITTFaxDecode") {
                        int width = item->get_dict().operator[]<integer_number>("Width")->get_value();
                        int height = item->get_dict().operator[]<integer_number>("Height")->get_value();

                        CCITTFaxDecode_Parms params;
                        auto decodeParam = item->get_dict().operator[]<dictionary_object>("DecodeParms");
                        if(decodeParam) {
                            auto k = decodeParam->operator[]<integer_number>("K");
                            if(k) {
                                params.K = k->get_value();
                            }

                            auto b = decodeParam->operator[]<boolean_value>("EndOfLine");
                            if(b) {
                                params.EndOfLine = b->get_value();
                            }

                            b = decodeParam->operator[]<boolean_value>("EncodedByteAlign");
                            if(b) {
                                params.EncodedByteAlign = b->get_value();
                            }

                            k = decodeParam->operator[]<integer_number>("Columns");
                            if(k) {
                                params.Columns = k->get_value();
                            }

                            k = decodeParam->operator[]<integer_number>("Rows");
                            if(k) {
                                params.Rows = k->get_value();
                            }

                            b = decodeParam->operator[]<boolean_value>("EndOfBlock");
                            if(b) {
                                params.EndOfBlock = b->get_value();
                            }

                            b = decodeParam->operator[]<boolean_value>("BlackIs1");
                            if(b) {
                                params.BlackIs1 = b->get_value();
                            }
                        }
                        const auto &values = item->get_stream();
                        CCITTFaxDecoder decoder(values, params);
                        decoder.output(target_path, page_no, key, width, height, rotate);
                        count++;
                    }
                    auto filters = std::dynamic_pointer_cast<array_object>(filter);
                    if(filters && filters->operator[]<name_object>(0)->get_value() == "DCTDecode") {
                        output_jpeg(target_path, page_no, key, item->get_stream(), rotate);
                        count++;
                    }
                    if(filters && filters->operator[]<name_object>(0)->get_value() == "CCITTFaxDecode") {
                        int width = item->get_dict().operator[]<integer_number>("Width")->get_value();
                        int height = item->get_dict().operator[]<integer_number>("Height")->get_value();

                        CCITTFaxDecode_Parms params;
                        auto decodeParam = item->get_dict().operator[]<array_object>("DecodeParms");
                        if(decodeParam) {
                            auto k = decodeParam->operator[]<dictionary_object>(0)->operator[]<integer_number>("K");
                            if(k) {
                                params.K = k->get_value();
                            }

                            auto b = decodeParam->operator[]<dictionary_object>(0)->operator[]<boolean_value>("EndOfLine");
                            if(b) {
                                params.EndOfLine = b->get_value();
                            }

                            b = decodeParam->operator[]<dictionary_object>(0)->operator[]<boolean_value>("EncodedByteAlign");
                            if(b) {
                                params.EncodedByteAlign = b->get_value();
                            }

                            k = decodeParam->operator[]<dictionary_object>(0)->operator[]<integer_number>("Columns");
                            if(k) {
                                params.Columns = k->get_value();
                            }

                            k = decodeParam->operator[]<dictionary_object>(0)->operator[]<integer_number>("Rows");
                            if(k) {
                                params.Rows = k->get_value();
                            }

                            b = decodeParam->operator[]<dictionary_object>(0)->operator[]<boolean_value>("EndOfBlock");
                            if(b) {
                                params.EndOfBlock = b->get_value();
                            }

                            b = decodeParam->operator[]<dictionary_object>(0)->operator[]<boolean_value>("BlackIs1");
                            if(b) {
                                params.BlackIs1 = b->get_value();
                            }
                        }
                        const auto &values = item->get_stream();
                        CCITTFaxDecoder decoder(values, params);
                        decoder.output(target_path, page_no, key, width, height, rotate);
                        count++;
                    }
                }
                else {
                    // raw image
                    int width = item->get_dict().operator[]<integer_number>("Width")->get_value();
                    int height = item->get_dict().operator[]<integer_number>("Height")->get_value();
                    int bits = item->get_dict().operator[]<integer_number>("BitsPerComponent")->get_value();
                    auto color = item->get_dict().operator[]<name_object>("ColorSpace")->get_value();
                    const auto &values = item->get_stream();
                    if(std::all_of(values.begin(), values.end(), [&](auto i) { return i == values[0]; })) continue;
                    if(bits == 8) {
                        output_gray8(target_path, page_no, key, values, width, height, rotate);
                    }
                    else if(bits == 1) {
                        std::vector<uint8_t> graybuf;
                        bit_stream bstream(values);
                        int width_b = (width + 7) / 8;
                        width_b *= 8;
                        auto it = bstream.begin();
                        for(int y = 0; y < height; y++) {
                            for(int x = 0; it != bstream.end() && x < width_b; x++) {
                                uint8_t v = *it++ == 0 ? 0 : 0xff;
                                if(x < width) {
                                    graybuf.push_back(v);
                                }
                            }
                        }
                        output_gray8(target_path, page_no, key, graybuf, width, height, rotate);
                    }
                    count++;
                }
            }
        }
    }
    return count;
}

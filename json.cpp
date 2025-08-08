#include "json.hpp"

#include <fstream>
#include <sstream>
#include <map>
#include <memory>
#include <cctype>

class json_object {
    public:
        json_object() {}
        virtual ~json_object(){}

        virtual std::string output() const = 0;
};

class json_number: public json_object {
    public:
        double value;

        std::string output() const override {
            std::stringstream ss;
            ss << value;
            return ss.str();
        }
};

class json_string: public json_object {
    public:
        std::string value;

        json_string(const std::string &init_value): value(init_value) {}

        std::string output() const override {
            std::stringstream ss(value);
            std::string ret;
            char c;
            while(ss.get(c)) {
                if(c == '"') {
                    ret += "\\\"";
                }
                else if(c == '\\') {
                    ret += "\\\\";
                }
                else if(c >= 0 && c < 0x20) {
                    if(c == '\b') {
                        ret += "\\b";
                    }
                    else if(c == '\f') {
                        ret += "\\f";
                    }
                    else if(c == '\n') {
                        ret += "\\n";
                    }
                    else if(c == '\r') {
                        ret += "\\r";
                    }
                    else if(c == '\t') {
                        ret += "\\t";
                    }
                    else {
                        int v = c;
                        char table[] = "0123456789abcdef";
                        ret += "\\u00";
                        ret += table[(v & 0xf0) >> 4];
                        ret += table[v & 0x0f];
                    }
                }
                else {
                    ret += c;
                }
            }
            return "\"" + ret + "\"";
        }
};

class json_boolean: public json_object {
    public:
        bool value;

        std::string output() const override {
            if(value) {
                return "true";
            }
            else {
                return "false";
            }
        }
};

class json_null: public json_object {
    public:
        std::string output() const override {
            return "null";
        }
};

class json_array: public json_object {
    protected:
        std::vector<std::shared_ptr<json_object>> value;
    public:
        json_array(std::vector<std::shared_ptr<json_object>> &init_value): value(init_value) {}

        std::shared_ptr<json_object> operator[](size_t idx) const {
            if(idx < value.size()) {
                return value[idx];
            }
            return nullptr;
        }

        size_t size() const {
            return value.size();
        }

        std::string output() const override {
            std::stringstream ss;
            int count = 0;
            for(const auto &v: value) {
                ss << v->output();
                if(count++ < value.size()-1) {
                    ss << "," << std::endl;
                }
            }
            ss.seekg(0, std::ios_base::beg);

            std::string ret;
            ret += "[\n";
            std::string line;
            while(std::getline(ss, line)) {
                ret += "  " + line + "\n";
            }
            ret += "]";
            return ret;
        }
};

class json_dict: public json_object {
    protected:
        std::map<std::string, std::shared_ptr<json_object>> value;
    public:
        json_dict(std::map<std::string, std::shared_ptr<json_object>> &init_value): value(init_value) {}

        std::shared_ptr<json_object> operator[](const std::string &key) const {
            if(value.count(key) > 0) {
                return value.at(key);
            }
            return nullptr;
        }

        std::vector<std::string> keys() const {
            std::vector<std::string> ret;
            for(const auto &v: value) {
                ret.push_back(v.first);
            }
            return ret;
        }

        std::string output() const override {
            std::stringstream ss;
            int count = 0;
            for(const auto &v: value) {
                ss << json_string(v.first).output() << ": " << v.second->output();
                if(count++ < value.size()-1) {
                    ss << "," << std::endl;
                }
            }
            ss.seekg(0, std::ios_base::beg);

            std::string ret;
            ret += "{\n";
            std::string line;
            while(std::getline(ss, line)) {
                ret += "  " + line + "\n";
            }
            ret += "}";
            return ret;
        }
};

std::shared_ptr<json_object> read_json(std::istream &stream);
std::shared_ptr<json_string> read_json_string(std::istream &stream);

std::shared_ptr<json_dict> read_json_dict(std::istream &stream)
{
    std::map<std::string, std::shared_ptr<json_object>> value;

    char c;
    std::string key;
    while(stream.get(c)) {
        if(std::isspace(c)) {
            continue;
        }
        if(c == '}') {
            break;
        }
        if(c == ',') {
            continue;
        }
        if(key == "") {
            if(c == '"') {
                auto k = read_json_string(stream);
                if(k) {
                    key = k->value;
                    continue;
                }
            }
        }
        else {
            if(c == ':') {
                continue;
            }
            stream.unget();
            auto v = read_json(stream);
            if(v) {
                value[key] = v;
                key = "";
                continue;
            }
        }
        return nullptr;
    }

    return std::make_shared<json_dict>(value);
}

std::shared_ptr<json_array> read_json_array(std::istream &stream)
{
    std::vector<std::shared_ptr<json_object>> value;

    char c;
    while(stream.get(c)) {
        if(std::isspace(c)) {
            continue;
        }
        if(c == ']') {
            break;
        }
        if(c == ',') {
            continue;
        }
        stream.unget();
        auto v = read_json(stream);
        if(v) {
            value.push_back(v);
        }
        else {
            return nullptr;
        }
    }
    return std::make_shared<json_array>(value);
}

std::shared_ptr<json_string> read_json_string(std::istream &stream)
{
    char c;
    bool escape = false;
    std::string str;
    while(stream.get(c)) {
        if(escape) {
            escape = false;
            if(c == '"') {
                str += c;
            }
            else if(c == '\\') {
                str += c;
            }
            else if(c == '/') {
                str += c;
            }
            else if(c == 'b') {
                str += '\b';
            }
            else if(c == 'f') {
                str += '\f';
            }
            else if(c == 'n') {
                str += '\n';
            }
            else if(c == 'r') {
                str += '\r';
            }
            else if(c == 't') {
                str += '\t';
            }
            else if(c == 'u') {
                uint16_t v = 0;
                int count = 0;
                while(count++ < 4 && stream.get(c)) {
                    v <<= 4;
                    if(c >= '0' && c <= '9') {
                        v |= c - '0';
                    }
                    else if(c >= 'a' && c <= 'f') {
                        v |= c - 'a' + 10;
                    }
                    else if(c >= 'A' && c <= 'F') {
                        v |= c - 'A' + 10;
                    }
                }
                str += char((v & 0xff00) >> 8);
                str += char(v & 0xff);
            }
            else {
                return nullptr;
            }
            continue;
        }
        if(c == '"') {
            auto ret = std::make_shared<json_string>(str);
            return ret;
        }
        if(c == '\\') {
            escape = true;
            continue;
        }
        if(c >= 0 && c < 0x20) {
            return nullptr;
        }
        str += c;
    }
    return nullptr;
}

std::shared_ptr<json_object> read_json(std::istream &stream)
{
    char c;
    while(stream.get(c)) {
        if(std::isspace(c)) {
            continue;
        }
        if(c == '{') {
            return read_json_dict(stream);
        }
        if(c == '[') {
            return read_json_array(stream);
        }
        if(c == '"') {
            return read_json_string(stream);
        }
        if(c == '-' || (c >= '0' && c <= '9')) {
            std::string number;
            number += c;
            while(stream.get(c)) {
                if((c >= '0' && c <= '9') || c == '+' || c == '-' || c == '.' || c == 'e' || c == 'E') {
                    number += c;
                }
                else {
                    stream.unget();
                    break;
                }
            }
            auto ret = std::make_shared<json_number>();
            std::stringstream(number) >> ret->value;
            return ret;
        }
        if(c == 't' || c == 'f' || c == 'n') {
            std::string keyword;
            keyword += c;
            while(stream.get(c)) {
                if(c >= 'a' && c <= 'z') {
                    keyword += c;
                }
                else {
                    stream.unget();
                    break;
                }
            }
            if(keyword == "true") {
                auto ret = std::make_shared<json_boolean>();
                ret->value = true;
                return ret;
            }
            if(keyword == "false") {
                auto ret = std::make_shared<json_boolean>();
                ret->value = false;
                return ret;
            }
            if(keyword == "null") {
                auto ret = std::make_shared<json_null>();
                return ret;
            }
        }
        return nullptr;
    }
    return nullptr;
}

std::vector<charbox> load_json_boxes(std::shared_ptr<json_object> json)
{
    std::vector<charbox> ret;
    if(json) {
        auto dict = std::dynamic_pointer_cast<json_dict>(json);
        if(!dict) return {};
        auto protate = std::dynamic_pointer_cast<json_number>((*dict)["rotate"]);
        int rotate = 0;
        if(protate) {
            rotate = protate->value;
        }
        auto array = std::dynamic_pointer_cast<json_array>((*dict)["box"]);
        if(!array) return {};
        auto count = array->size();
        for(int i = 0; i < count; i++) {
            auto box = std::dynamic_pointer_cast<json_dict>((*array)[i]);
            if(!box) return {};
            
            auto pcx = std::dynamic_pointer_cast<json_number>((*box)["cx"]);
            float cx = (pcx) ? pcx->value : 0;
            auto pcy = std::dynamic_pointer_cast<json_number>((*box)["cy"]);
            float cy = (pcy) ? pcy->value : 0;
            auto pw = std::dynamic_pointer_cast<json_number>((*box)["w"]);
            float w = (pw) ? pw->value : 0;
            auto ph = std::dynamic_pointer_cast<json_number>((*box)["h"]);
            float h = (ph) ? ph->value : 0;
            auto pruby = std::dynamic_pointer_cast<json_number>((*box)["ruby"]);
            bool ruby = (pruby) ? pruby->value != 0 : false;
            auto prubybase = std::dynamic_pointer_cast<json_number>((*box)["rubybase"]);
            bool rubybase = (prubybase) ? prubybase->value != 0 : false;
            auto pvertical = std::dynamic_pointer_cast<json_number>((*box)["vertical"]);
            bool vertical = (pvertical) ? pvertical->value != 0 : false;
            auto pspace = std::dynamic_pointer_cast<json_number>((*box)["space"]);
            bool space = (pspace) ? pspace->value != 0 : false;
            auto pblockidx = std::dynamic_pointer_cast<json_number>((*box)["blockidx"]);
            int blockidx = (pblockidx) ? pblockidx->value : 0;
            auto plineidx = std::dynamic_pointer_cast<json_number>((*box)["lineidx"]);
            int lineidx = (plineidx) ? plineidx->value : 0;
            auto psubidx = std::dynamic_pointer_cast<json_number>((*box)["subidx"]);
            int subidx = (psubidx) ? psubidx->value : 0;
            auto ptext = std::dynamic_pointer_cast<json_string>((*box)["text"]);
            std::string text = (ptext) ? ptext->value : "";
            
            ret.push_back({ cx, cy, w, h, ruby, rubybase, vertical, space, blockidx, lineidx, subidx, rotate, text });
        }

        std::sort(ret.begin(), ret.end(), [](auto a, auto b){
            if(a.blockidx != b.blockidx) return a.blockidx < b.blockidx;
            if(a.lineidx != b.lineidx) return a.lineidx < b.lineidx;
            return a.subidx < b.subidx;
        });
        return ret;
    }
    return {};
}

std::map<std::string, std::vector<charbox>> read_all_boxes(const std::string &filename)
{
    std::ifstream ifs(filename);
    return read_all_boxes(ifs);
}

std::map<std::string, std::vector<charbox>> read_all_boxes(std::istream &is)
{
    std::map<std::string, std::vector<charbox>> ret;
    auto json = read_json(is);
    if(json) {
        auto dict = std::dynamic_pointer_cast<json_dict>(json);
        if(dict) {
            auto pages = std::dynamic_pointer_cast<json_array>((*dict)["pages"]);
            for(int i = 0; i < pages->size(); i++) {
                auto pagedict = std::dynamic_pointer_cast<json_dict>((*pages)[i]);
                auto name = std::dynamic_pointer_cast<json_string>((*pagedict)["name"]);
                if(name) {
                    ret.emplace(name->value, load_json_boxes(pagedict));
                }
            }
        }
    }
    return ret;
}

std::vector<charbox> read_boxes(const std::string &filename)
{
    std::ifstream ifs(filename);
    return read_boxes(ifs);
}

std::vector<charbox> read_boxes(std::istream &is)
{
    auto json = read_json(is);
    if(json) {
        auto dict = std::dynamic_pointer_cast<json_dict>(json);
        if(dict) {
            auto pages = std::dynamic_pointer_cast<json_array>((*dict)["pages"]);
            if(pages) {
                return load_json_boxes((*pages)[0]);
            }
            return load_json_boxes(json);
        }
    }
    return {};
}

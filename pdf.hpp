#pragma once
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <map>
#include <memory>
#include <cstdint>
#include <algorithm>

#include "json.hpp"

class object {
    public:
        virtual ~object(){}
        virtual void print(int indent = 0) const {
            std::cout << debug();
        }
        virtual std::string output() const { return ""; }
        virtual std::string debug() const { return output(); }
        virtual std::string dump() const { return output(); }

        friend std::ostream& operator<<(std::ostream& os, const object* obj);
        friend std::ostream& operator<<(std::ostream& os, const object &obj);
};
// boolean values, integers, real numbers, strings, names, arrays, dictionaries, streams, and the null object.

class direct_object: public object {
};

class boolean_value: public direct_object {
    private:
        bool _value;

    public:
        boolean_value(bool value) : _value(value) {}

        inline bool get_value() const {
            return _value;
        }

        inline void set_value(bool new_value) {
            _value = new_value;
        }

        std::string output() const override { 
            if (_value) {
                return "true";
            }
            else {
                return "false";
            }
        }
};

class numeric_value: public direct_object {
};

class integer_number: public numeric_value {
    private:
        int _value;

    public:
        integer_number(int value) {
            _value = value;
        }

        inline int get_value() const {
            return _value;
        }

        inline void set_value(int new_value) {
            _value = new_value;
        }

        std::string output() const override { 
            std::stringstream ss;
            ss << _value;
            return ss.str();
        }
};

class real_number: public numeric_value {
    private:
        double _value;

    public:
        real_number(double value) {
            _value = value;
        }

        inline double get_value() const {
            return _value;
        }

        inline void set_value(double new_value) {
            _value = new_value;
        }

        std::string output() const override {
            std::stringstream ss;
            if(int(_value) == _value) {
                ss << int(_value);
            }
            else {
                ss << std::fixed << _value;
            }
            return ss.str();
        }
};


class string_object: public direct_object {
    protected:
        std::string _value;

    public:
        inline std::string get_value() const {
            return _value;
        }

        inline void set_value(std::string new_value) {
            _value = new_value;
        }

        std::string debug() const override {
            std::stringstream ss;
            for(const auto &c: _value) {
                if(c >= 0x20 && c < 0x7f) {
                    ss << c;
                }
                else {
                    int value = *(unsigned char *)&c;
                    char table[] = "0123456789ABCDEF";
                    ss << "\\u" << table[(value & 0xF0) >> 4] << table[value & 0xF];
                }
            }
            return ss.str();
        }
};


class literal_string: public string_object {
    public:
        literal_string(std::string value) {
            _value = value;
        }

        std::string output() const override { 
            std::string result;
            const char table[] = "01234567";
            int parenthesis_pair = 0;
            for(const auto &c: _value) {
                if(c == '(') {
                    parenthesis_pair++;
                }
                else if(c == ')') {
                    parenthesis_pair--;
                }
            }

            int parenthesis = 0;
            for(const auto &c: _value) {
                int ci = *(unsigned char *)&c;
                if(parenthesis_pair + parenthesis < 0) {
                    if(c == ')') {
                        parenthesis++;
                        result += "\\)";
                        continue;
                    }
                }
                if(parenthesis_pair + parenthesis > 0) {
                    if(c == '(') {
                        parenthesis--;
                        result += "\\(";
                        continue;
                    }
                }
                if(ci == 0x5C) {
                    result += "\\\\";
                }
                else if(ci >= 0x20 && ci <= 0x7E) {
                    result += c;
                }
                else if(ci == 0x0A) {
                    result += "\\n";
                }
                else if(ci == 0x0D) {
                    result += "\\r";
                }
                else if(ci == 0x09) {
                    result += "\\t";
                }
                else if(ci == 0x08) {
                    result += "\\b";
                }
                else if(ci == 0x0C) {
                    result += "\\f";
                }
                else {
                    int code = ci;
                    int c0 = code & 0x07;
                    int c1 = (code & 0x38) >> 3;
                    int c2 = (code & 0xc0) >> 6;
                    result += 0x5c;
                    result += table[c2];
                    result += table[c1];
                    result += table[c0];
                }
            }

            return "(" + result + ")";
        }
};

class hexadecimal_string: public string_object {
    public:
        hexadecimal_string(std::string value) {
            _value = value;
        }

        std::string output() const override { 
            std::string result;
            const char table[] = "0123456789ABCDEF";
            for(const auto &c: _value) {
                int ci = *(unsigned char *)&c;
                int code = ci;
                int c0 = code % 16;
                code /= 16;
                int c1 = code % 16;
                result += table[c1];
                result += table[c0];                
            }
            
            return "<" + result + ">";
        }
};


class name_object: public direct_object {
    private:
        std::string _value;

    public:
        name_object(std::string value) {
            _value = value;
        }

        inline std::string get_value() const {
            return _value;
        }

        std::string output() const override { 
            std::string result;
            const char table[] = "0123456789ABCDEF";
            for(const auto &c: _value) {
                int ci = *(unsigned char *)&c;
                if(c == '#') {
                    result += "#23";
                }
                else if(ci >= 0x21 && ci <= 0x7E) {
                    result += c;
                }
                else {
                    int code = ci;
                    int c0 = code % 16;
                    code /= 16;
                    int c1 = code % 16;
                    result += "#";
                    result += table[c1];
                    result += table[c0];
                }
            }
            
            return "/" + result;
        }

        std::string debug() const override {
            return "/" + _value;
        }
};

class indirect_object: public object {
    private:
        unsigned int _number;
        unsigned int _generation;
        std::shared_ptr<object> _target;
    
    public:
        indirect_object(unsigned int num, unsigned int gen, std::shared_ptr<object> target)
            : _number(num), _generation(gen), _target(target) {}

        indirect_object(unsigned int num)
            : _number(num), _generation(0), _target(nullptr) {}

        template<typename T>
        std::shared_ptr<T> follow() {
            return std::dynamic_pointer_cast<T>(_target);
        }

        bool isLoaded() {
            return _target.get() != nullptr;
        }

        void set(std::shared_ptr<object> target) {
            _target = target;
        }

        inline unsigned int number() const {
            return _number;
        }

        std::string dump() const override {
            std::stringstream ss;
            ss << _number << " " << _generation << " obj\r\n";
            if(_target) {
                ss << _target->dump();
            }
            else {
                ss << "null";
            }
            ss << "\r\nendobj\r\n";
            return ss.str();
        }

        std::string output() const override {
            std::stringstream ss;
            ss << _number << " " << _generation << " R";
            return ss.str();
        }

        std::string debug() const override {
            std::stringstream ss;
            ss << _number << " " << _generation << " R";
            return ss.str();
        }
};

class array_object: public direct_object {
    private:
        std::vector<std::shared_ptr<object>> _value;

    public:
        array_object() {}
        array_object(const std::vector<std::shared_ptr<object>> &value): _value(value) {}

        template<typename T>
        std::shared_ptr<T> operator[](std::size_t idx) const {
            if(idx < _value.size()) {
                if(!std::is_same_v<T, indirect_object>) {
                    auto obj = std::dynamic_pointer_cast<indirect_object>(_value[idx]);
                    if(obj) {
                        return obj->follow<T>();
                    }
                }
                return std::dynamic_pointer_cast<T>(_value[idx]);
            }
            else {
                return nullptr;
            }
        }

        size_t size() const {
            return _value.size();
        }

        void add(std::shared_ptr<object> value) {
            _value.push_back(value);
        }

        void insert(std::size_t idx, std::shared_ptr<object> value) {
            _value.insert(_value.begin()+idx, value);
        }

        void remove(std::size_t idx) {
            _value.erase(_value.begin() + idx);
        }

        std::string output() const override {
            std::string result;
            for(int i = 0; i < _value.size(); i++) {
                result += _value[i]->output();
                if(i < _value.size()-1) {
                    result += " ";
                }
            }
            return "[" + result + "]";
        }

        std::string debug() const override {
            std::string result;
            for(int i = 0; i < _value.size(); i++) {
                std::stringstream buf(_value[i]->debug() + "\n");
                std::string s;
                while(std::getline(buf, s)) {
                    result += " " + s + "\n";
                }
            }
            return "[\n" + result + "]";
        }
};

class dictionary_object: public direct_object {
    private:
        std::vector<name_object> _keys;
        std::vector<std::shared_ptr<object>> _value;

    public:
        dictionary_object() {}

        dictionary_object(const std::map<name_object, std::shared_ptr<object>> &value) {
            for(const auto &item: value) {
                _keys.push_back(item.first);
                _value.push_back(item.second);
            }
        }

        template<typename T>
        std::shared_ptr<T> operator[](const std::string key) const {
            for(int i = 0; i < _keys.size(); i++) {
                if(_keys[i].get_value() == key) {
                    if(!std::is_same_v<T, indirect_object>) {
                        auto obj = std::dynamic_pointer_cast<indirect_object>(_value[i]);
                        if(obj) {
                            return obj->follow<T>();
                        }
                    }
                    return std::dynamic_pointer_cast<T>(_value[i]);
                }
            }
            return nullptr;
        }

        std::vector<std::string> keys() const {
            std::vector<std::string> ret;
            for(const auto &k: _keys) {
                ret.push_back(k.get_value());
            }
            return ret;
        }

        std::shared_ptr<object> value(const std::string &key) const {
            for(int i = 0; i < _keys.size(); i++) {
                if(_keys[i].get_value() == key) {
                    return _value[i];
                }
            }
            return nullptr;
        }

        void add(const std::string &key, std::shared_ptr<object> value) {
            int i = -1;
            for(int j = 0; j < _keys.size(); j++) {
                if(_keys[j].get_value() == key) {
                    i = j;
                    break;
                }
            }
            if(i < 0) {
                _keys.emplace_back(key);
                _value.push_back(value);
            }
            else {
                _value[i] = value;
            }
        }

        void remove(const std::string &key) {
            int i = -1;
            for(int j = 0; j < _keys.size(); j++) {
                if(_keys[j].get_value() == key) {
                    i = j;
                    break;
                }
            }
            if(i < 0) return;
            _keys.erase(_keys.begin() + i);
            _value.erase(_value.begin() + i);
        }

        std::string output() const override {
            std::string result;
            for(int i = 0; i < _keys.size(); i++) {
                result += _keys[i].output();
                result += " ";
                result += _value[i]->output();
                if(i < _value.size()-1) {
                    result += " ";
                }
            }
            return "<<" + result + ">>";
        }

        std::string debug() const override {
            std::string result;
            for(int i = 0; i < _value.size(); i++) {
                auto keystr = " " + _keys[i].debug();
                result += keystr + " ";
                std::string pad;
                std::stringstream buf(_value[i]->debug() + "\n");
                std::string s;
                while(std::getline(buf, s)) {
                    result += pad + s + "\n";
                    if(pad == "") {
                        pad = std::string(keystr.size()+1, ' ');
                    }
                }
            }
            return "<<\n" + result + ">>";
        }
};

void decode_stream(dictionary_object &dict, std::vector<uint8_t> &stream);

class stream_object: public direct_object {
    protected:
        dictionary_object _dict;
        std::vector<uint8_t> _stream;

    public:
        bool zlib_deflate = true;

        stream_object() {}

        stream_object(const std::vector<uint8_t> &stream)
            : stream_object(dictionary_object(), stream) {}

        stream_object(const dictionary_object &dict, const std::vector<uint8_t> &stream)
            : _dict(dict), _stream(stream) {
            decode_stream(_dict, _stream);
            auto length = _stream.size();
            _dict.add("Length", std::shared_ptr<object>(new integer_number((int)length)));
        }

        dictionary_object& get_dict() {
            return _dict;
        }

        std::vector<uint8_t>& get_stream() {
            return _stream;
        }

        void set_stream(const std::vector<uint8_t>& stream) {
            _stream = stream;
            auto length = _stream.size();
            _dict.add("Length", std::shared_ptr<object>(new integer_number((int)length)));
        }

        std::string output() const override;

        std::string debug() const override {
            std::stringstream ss;
            ss << _dict.debug() << std::endl;
            ss << "stream " << _stream.size() << "bytes" << std::endl;
            ss << std::hex;
            char fillSaved = ss.fill('0');
            for(int i = 0; i < _stream.size(); i++) {
                if(i % 16 == 0) {
                    ss << std::setw(8) << i << ": "; 
                }
                ss << std::setw(2) << int(_stream[i]) << " ";
                if(i % 16 == 15) {
                    ss << std::endl; 
                }
            }
            ss << std::endl;
            ss.fill(fillSaved);
            ss << "end stream" << std::endl;
            return ss.str();
        }
};

class null_object: public direct_object {
    public:
        std::string output() const override {
            return "null";
        }
};

class Rectangle: public array_object {
    public:
        Rectangle(): array_object(std::vector<std::shared_ptr<object>>{
            std::shared_ptr<object>(new real_number(0)),
            std::shared_ptr<object>(new real_number(0)),
            std::shared_ptr<object>(new real_number(0)),
            std::shared_ptr<object>(new real_number(0)),
        }) {}

        Rectangle(double x1, double y1, double x2, double y2): array_object(std::vector<std::shared_ptr<object>>{
            std::shared_ptr<object>(new real_number(x1)),
            std::shared_ptr<object>(new real_number(y1)),
            std::shared_ptr<object>(new real_number(x2)),
            std::shared_ptr<object>(new real_number(y2)),
        }) {}

        void set_value(double x1, double y1, double x2, double y2) {
            operator[]<real_number>(0)->set_value(x1);
            operator[]<real_number>(1)->set_value(y1);
            operator[]<real_number>(2)->set_value(x2);
            operator[]<real_number>(3)->set_value(y2);
        }
};

class JpegImageObject: public stream_object {
    public:
        int width;
        int height;

        JpegImageObject(const std::string &filename);
};

class IndexedColorSpace: public array_object {
    private:
        int hi_value = 0;
        int m = 0;
        std::vector<uint8_t> lookup_table;

    public:
        bool isValid = false;
        
        IndexedColorSpace(const array_object &base);
        void lookup(uint8_t i, uint8_t &r, uint8_t &g, uint8_t &b);
};

class PageObject: public dictionary_object {
    public:
        PageObject() {
            add("Type", std::shared_ptr<object>(new name_object("Page")));
            add("Resources", std::shared_ptr<object>(new dictionary_object()));
            add("MediaBox", std::shared_ptr<object>(new Rectangle()));
        }

        PageObject(const dictionary_object &base)
            : dictionary_object(base) {}

        void set_parent(std::shared_ptr<object> obj) {
            add("Parent", obj);
        }

        void set_contents(std::shared_ptr<object> obj) {
            add("Contents", obj);
        }

        void set_mediabox(double x1, double y1, double x2, double y2) {
            operator[]<Rectangle>("MediaBox")->set_value(x1,y1,x2,y2);
        }
};

class PagesObject: public dictionary_object {
    private:
        std::vector<std::shared_ptr<PageObject>> page_array;
    public:
        PagesObject() {
            add("Type", std::shared_ptr<object>(new name_object("Pages")));
            add("Kids", std::shared_ptr<object>(new array_object()));
            add("Count", std::shared_ptr<object>(new integer_number(0)));
        }

        PagesObject(const dictionary_object &base)
            : dictionary_object(base) {
            auto kids = operator[]<array_object>("Kids");
            int i = 0;
            auto p = kids->operator[]<indirect_object>(i);
            while(p) {
                auto dict = p->follow<dictionary_object>();
                if(dict) {
                    if(dict->operator[]<name_object>("Type")->get_value() == "Pages") {
                        p->set(std::shared_ptr<object>(new PagesObject(*dict)));
                    }
                    else if(dict->operator[]<name_object>("Type")->get_value() == "Page") {
                        p->set(std::shared_ptr<object>(new PageObject(*dict)));
                    }
                }
                p = kids->operator[]<indirect_object>(++i);
            }
            i = 0;
            p = kids->operator[]<indirect_object>(i);
            while(p) {
                auto tree = p->follow<PagesObject>();
                if(tree) {
                    std::copy(tree->page_array.begin(), tree->page_array.end(), std::back_inserter(page_array));
                }
                auto leaf = p->follow<PageObject>();
                if(leaf) {
                    page_array.push_back(leaf);
                }
                p = kids->operator[]<indirect_object>(++i);
            }
        }

        void add_page(std::shared_ptr<object> obj) {
            operator[]<array_object>("Kids")->add(obj);
            auto count = operator[]<integer_number>("Count");
            count->set_value(count->get_value() + 1);
        }

        std::vector<std::shared_ptr<PageObject>>::iterator begin() {
            return page_array.begin();
        }

        std::vector<std::shared_ptr<PageObject>>::iterator end() {
            return page_array.end();
        }
};

class RootObject: public dictionary_object {
    public:
        RootObject() {
            add("Type", std::shared_ptr<object>(new name_object("Catalog")));
        }

        RootObject(const dictionary_object &base)
            : dictionary_object(base) {}

        void add_pages(std::shared_ptr<object> obj) {
            add("Pages", obj);
        }
};

class pdf_file {
    private:
        std::string header = "%PDF-1.3";
        std::vector<int> bodyidx;
        std::vector<std::shared_ptr<indirect_object>> body;
        dictionary_object trailer;
        std::shared_ptr<indirect_object> root;
        std::shared_ptr<indirect_object> pages;
        std::shared_ptr<indirect_object> cidFontType0Object;
        std::shared_ptr<object> encrypt;
        std::vector<int64_t> body_offsets;

        void prepare_font();
        std::shared_ptr<PageObject> new_page();
        std::shared_ptr<indirect_object> add_object(std::shared_ptr<object> obj);
        std::shared_ptr<indirect_object> ref_object(int objnum);
        std::shared_ptr<object> parse_object(std::istream &ss, int obj_num = 0, int gen_num = 0);
        std::shared_ptr<hexadecimal_string> parse_hexstring(std::istream &ss, int obj_num, int gen_num);
        std::shared_ptr<literal_string> parse_literal(std::istream &ss, int obj_num, int gen_num);
        std::shared_ptr<name_object> parse_name(std::istream &ss);
        std::shared_ptr<object> parse_numeric(std::istream &is, std::istream &ss);
        std::shared_ptr<array_object> parse_array(std::istream &ss);
        std::shared_ptr<dictionary_object> parse_dictionary(std::istream &ss);
        std::shared_ptr<indirect_object> register_object(std::istream &ss, int obj_num, int gen_num);
        void register_ObjectStream(std::shared_ptr<stream_object> objstream);

    public:
        pdf_file();
        pdf_file(const std::string &filename);
        std::string dump() const;
        std::shared_ptr<PageObject> add_image(const std::string &jpgname, const std::vector<charbox> &box);
        bool add_image(std::shared_ptr<PageObject> page, const std::string &key, int imwidth, int imheight, int rotate, const std::vector<charbox> &box);
        int extract_images(const std::string &target_path);
        int process_images(const std::string &target_path);
        int process_json(const std::string &target_path);
};


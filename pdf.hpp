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

        template<typename T>
        T* cast() {
            return dynamic_cast<T *>(this);
        }

        friend std::ostream& operator<<(std::ostream& os, const object* obj);
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
            ss << _value;
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
            for(const auto c: _value) {
                if(c == '(') {
                    parenthesis_pair++;
                }
                else if(c == ')') {
                    parenthesis_pair--;
                }
            }

            int parenthesis = 0;
            for(const auto c: _value) {
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
                    int c0 = code % 8;
                    code /= 8;
                    int c1 = code % 8;
                    code /= 8;
                    int c2 = code % 8;
                    result += '\\';
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
            for(const auto c: _value) {
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
            for(const auto c: _value) {
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
};

class array_object: public direct_object {
    private:
        std::vector<std::shared_ptr<object>> _value;

    public:
        array_object() {}
        array_object(const std::vector<std::shared_ptr<object>> &value): _value(value) {}

        template<typename T>
        T* operator[](std::size_t idx) const {
            return dynamic_cast<T*>(_value[idx].get());
        }

        void add(std::shared_ptr<object> value) {
            _value.push_back(value);
        }

        void insert(std::size_t idx, std::shared_ptr<object> value) {
            _value.insert(_value.begin()+idx, value);
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
                std::stringstream buf(_value[i]->output() + "\n");
                std::string s;
                while(std::getline(buf, s)) {
                    result += " " + s + "\n";
                }
            }
            return "[\n" + result + "]\n";
        }
};

class dictionary_object: public direct_object {
    private:
        std::vector<name_object> _keys;
        std::vector<std::shared_ptr<object>> _value;

    public:
        dictionary_object() {}

        dictionary_object(const std::map<name_object, std::shared_ptr<object>> &value) {
            for(const auto item: value) {
                _keys.push_back(item.first);
                _value.push_back(item.second);
            }
        }

        template<typename T>
        T* operator[](const std::string key) const {
            for(int i = 0; i < _keys.size(); i++) {
                if(_keys[i].get_value() == key) {
                    return dynamic_cast<T*>(_value[i].get());
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
                result += " " + _keys[i].debug() + " ";
                int count = result.size();
                std::string pad;
                std::stringstream buf(_value[i]->debug() + "\n");
                std::string s;
                while(std::getline(buf, s)) {
                    result += pad + s + "\n";
                    if(pad == "") {
                        pad = std::string(count+1, ' ');
                    }
                }
            }
            return "<<\n" + result + ">>\n";
        }
};

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
            if(_dict.operator[]<integer_number>("Length") == nullptr) {
                auto length = _stream.size();
                _dict.add("Length", std::shared_ptr<object>(new integer_number(length)));
            }
        }

        dictionary_object& get_dict() {
            return _dict;
        }

        std::vector<uint8_t>& get_stream() {
            return _stream;
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

class indirect_object: public object {
    private:
        unsigned int _number;
        unsigned int _generation;
        std::shared_ptr<object> _target;
    
    public:
        indirect_object(unsigned int num, unsigned int gen, std::shared_ptr<object> target)
            : _number(num), _generation(gen), _target(target) {}

        template<typename T>
        T* follow() {
            return dynamic_cast<T*>(_target.get());
        }

        std::string dump() const override {
            std::stringstream ss;
            ss << _number << " " << _generation << " obj\r\n";
            ss << _target->dump();
            ss << "\r\nendobj\r\n";
            return ss.str();
        }

        std::string output() const override {
            std::stringstream ss;
            ss << _number << " " << _generation << " R";
            return ss.str();
        }

        std::string debug() const override { 
            return dump();
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

class PageObject: public dictionary_object {
    public:
        PageObject() {
            add("Type", std::shared_ptr<object>(new name_object("Page")));
            add("Resources", std::shared_ptr<object>(new dictionary_object()));
            add("MediaBox", std::shared_ptr<object>(new Rectangle()));
        }

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
    public:
        PagesObject() {
            add("Type", std::shared_ptr<object>(new name_object("Pages")));
            add("Kids", std::shared_ptr<object>(new array_object()));
            add("Count", std::shared_ptr<object>(new integer_number(0)));
        }

        void add_page(std::shared_ptr<object> obj) {
            operator[]<array_object>("Kids")->add(obj);
            auto count = operator[]<integer_number>("Count");
            count->set_value(count->get_value() + 1);
        }
};

class RootObject: public dictionary_object {
    public:
        RootObject() {
            add("Type", std::shared_ptr<object>(new name_object("Catalog")));
        }

        void add_pages(std::shared_ptr<object> obj) {
            add("Pages", obj);
        }
};

class pdf_file {
    private:
        std::string header = "%PDF-1.3";
        std::vector<std::shared_ptr<object>> body;
        dictionary_object trailer;
        std::shared_ptr<object> root;
        std::shared_ptr<object> pages;
        std::shared_ptr<object> cidFontType0Object;

        void prepare_font();
        std::shared_ptr<PageObject> new_page();
        std::shared_ptr<object> add_object(std::shared_ptr<object> obj);
    public:
        pdf_file();
        std::string dump() const;
        std::shared_ptr<PageObject> add_image(const std::string &jpgname, const std::vector<charbox> &box);
};

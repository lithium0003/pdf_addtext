#pragma once
#include <string>
#include <vector>
#include <map>

struct charbox {
    float cx;
    float cy;
    float w;
    float h;
    bool ruby;
    bool rubybase;
    bool vertical;
    bool space;
    int blockidx;
    int lineidx;
    int subidx;
    int rotate;
    std::string text;
};

std::vector<charbox> read_boxes(const std::string &filename);
std::map<std::string, std::vector<charbox>> read_all_boxes(const std::string &filename);

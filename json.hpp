#pragma once
#include <string>
#include <vector>

struct charbox {
    float cx;
    float cy;
    float w;
    float h;
    bool vertical;
    bool space;
    int blockidx;
    int lineidx;
    int subidx;
    std::string text;
};

std::vector<charbox> read_boxes(const std::string &filename);

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <map>
#include <memory>
#include <cstdint>
#include <algorithm>

#include "pdf.hpp"

int main(int argc, char **argv)
{
    if(argc <= 2) {
        std::cout << "usage: " << argv[0] << " (output.pdf) (input1.jpg) [(input2.jpg) ...]" << std::endl;
        return 0;
    }
    std::string outputname(argv[1]);

    pdf_file pdf;
    for(int i = 2; i < argc; i++) {
        std::string inputname(argv[i]);
        auto pos = inputname.find_last_of('.');
        if(pos != inputname.npos) {
            std::string ext(inputname, pos);
            if(ext == ".jpg" || ext == ".jpeg" || ext == ".jpe") {
                std::string jsonname = inputname + ".json";

                auto box = read_boxes(jsonname);
                if(box.size() == 0) {
                    std::cerr << "OCR json load failed: " << jsonname << std::endl;
                }
                pdf.add_image(inputname, box);
                continue;
            }
        }
        std::cerr << "not jpeg file: " << inputname << std::endl;
    }

    std::ofstream output(outputname, std::ios::binary);
    output << pdf.dump();

    return 0;
}
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <map>
#include <memory>
#include <cstdint>
#include <algorithm>
#include <filesystem>

#include "pdf.hpp"

int create(int argc, char **argv)
{
    if (argc < 3) {
        std::cout << "usage: " << argv[0] << " create (output.pdf) (input1.jpg) [(input2.jpg) ...]" << std::endl;
        return 0;
    }

    std::string outputname(argv[2]);

    pdf_file pdf;
    for(int i = 3; i < argc; i++) {
        std::string inputname(argv[i]);
        std::cerr << "input file: " << inputname << std::endl;
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

int extract(int argc, char **argv)
{
    if (argc < 3) {
        std::cout << "usage: " << argv[0] << " extract (target.pdf) (output_dir)" << std::endl;
        return 0;
    }

    pdf_file pdf(argv[2]);
    std::string target_path;

    if(argc > 3) {
        target_path = argv[3];
        std::filesystem::create_directories(target_path);
    }
    else {
        target_path = ".";
    }

    int count = pdf.extract_images(target_path);
    std::cout << "extract images: " << count << std::endl;

    return 0;
}

void print_usage(char *argv0)
{
    std::cout << "usage: " << argv0 << " create (output.pdf) (input1.jpg) [(input2.jpg) ...]" << std::endl;
    std::cout << "usage: " << argv0 << " extract (target.pdf) (output_dir)" << std::endl;
}

int main(int argc, char **argv)
{
    if(argc <= 2) {
        print_usage(argv[0]);
        return 0;
    }
    std::string command(argv[1]);

    if(command == "create") {
        return create(argc, argv);
    }
    if(command == "extract") {
        return extract(argc, argv);
    }

    std::cout << "command not found: " << command << std::endl;
    print_usage(argv[0]);
    return 0;
}
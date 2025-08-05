//
//  filters.hpp
//  bunkoOCR
//
//  Created by rei9 on 2025/08/04.
//

#ifndef filters_hpp
#define filters_hpp

#include "pdf.hpp"

void decode_stream(dictionary_object &dict, std::vector<uint8_t> &stream);
std::vector<uint8_t> JBIG2Decode(const std::vector<uint8_t> &stream);

#endif /* filters_hpp */

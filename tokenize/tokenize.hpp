#ifndef LIPH_TOKENIZE_HPP
#define LIPH_TOKENIZE_HPP

#include <string>
#include <vector>


std::vector<std::string> tokenize(const std::string &input, const std::string &delimiter, bool allowEmpty = true) {
    std::string::size_type index = 0, last_index = 0;
    std::vector<std::string> tokens;
    
    if(delimiter.empty()) {
        if(allowEmpty || !input.empty())
            return {input};
        else
            return {};
    }
    
    while((index = input.find(delimiter, last_index)) != std::string::npos) {
        if(allowEmpty || index != last_index)
            tokens.push_back(input.substr(last_index, index - last_index));
        last_index = index + delimiter.size();
    }
    
    if(allowEmpty || last_index != input.size())
        tokens.push_back(input.substr(last_index));
    return tokens;
}


#endif

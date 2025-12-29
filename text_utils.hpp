#ifndef TEXT_UTILS_H
#define TEXT_UTILS_H

#include <cstdint>
#include <string>
#include <vector>

namespace TextUtils {

inline bool isContinuationByte(unsigned char b);

std::vector<uint32_t> stringToCodes(const std::string &s);

std::string codesToString(const std::vector<uint32_t> &codes);

inline uint32_t charToLower(uint32_t c);

inline bool isValidSymbol(uint32_t c);

std::string toLowerCase(const std::string &s);

std::vector<std::string> tokenize(const std::string &text);

}

#endif

#include "text_utils.hpp"

namespace TextUtils {

inline bool isContinuationByte(unsigned char b) { return (b & 0xC0) == 0x80; }

std::vector<uint32_t> stringToCodes(const std::string &s) {
  std::vector<uint32_t> result;
  size_t i = 0;
  const unsigned char *ptr = reinterpret_cast<const unsigned char *>(s.data());
  const size_t length = s.size();

  while (i < length) {
    unsigned char byte = ptr[i];
    uint32_t codepoint = 0;
    size_t numContinuationBytes = 0;

    if (byte < 0x80) {
      codepoint = byte;
      numContinuationBytes = 0;
    } else if ((byte & 0xE0) == 0xC0) {
      codepoint = byte & 0x1F;
      numContinuationBytes = 1;
    } else if ((byte & 0xF0) == 0xE0) {
      codepoint = byte & 0x0F;
      numContinuationBytes = 2;
    } else if ((byte & 0xF8) == 0xF0) {
      codepoint = byte & 0x07;
      numContinuationBytes = 3;
    } else {
      ++i;
      continue;
    }

    if (i + numContinuationBytes >= length) {
      break;
    }

    bool isValid = true;
    for (size_t j = 1; j <= numContinuationBytes; ++j) {
      if (!isContinuationByte(ptr[i + j])) {
        isValid = false;
        break;
      }
      codepoint = (codepoint << 6) | (ptr[i + j] & 0x3F);
    }

    if (!isValid) {
      ++i;
      continue;
    }

    result.push_back(codepoint);
    i += 1 + numContinuationBytes;
  }

  return result;
}

std::string codesToString(const std::vector<uint32_t> &codes) {
  std::string result;
  result.reserve(codes.size() * 2);

  for (uint32_t codepoint : codes) {
    if (codepoint <= 0x7F) {
      result.push_back(static_cast<char>(codepoint));
    } else if (codepoint <= 0x7FF) {
      result.push_back(static_cast<char>(0xC0 | ((codepoint >> 6) & 0x1F)));
      result.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
    } else if (codepoint <= 0xFFFF) {
      result.push_back(static_cast<char>(0xE0 | ((codepoint >> 12) & 0x0F)));
      result.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
      result.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
    } else if (codepoint <= 0x10FFFF) {
      result.push_back(static_cast<char>(0xF0 | ((codepoint >> 18) & 0x07)));
      result.push_back(static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F)));
      result.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
      result.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
    }
  }

  return result;
}

inline uint32_t charToLower(uint32_t codepoint) {
  if (codepoint >= 'A' && codepoint <= 'Z') {
    return codepoint + 32;
  }

  if (codepoint >= 0x0410 && codepoint <= 0x042F) {
    return codepoint + 0x20;
  }

  if (codepoint == 0x0401) {
    return 0x0451;
  }

  return codepoint;
}

inline bool isValidSymbol(uint32_t codepoint) {

  if ((codepoint >= 'A' && codepoint <= 'Z') ||
      (codepoint >= 'a' && codepoint <= 'z')) {
    return true;
  }

  if (codepoint >= '0' && codepoint <= '9') {
    return true;
  }

  if (codepoint >= 0x0400 && codepoint <= 0x04FF) {
    return true;
  }

  return false;
}

std::string toLowerCase(const std::string &s) {
  std::vector<uint32_t> codes = stringToCodes(s);

  for (auto &code : codes) {
    code = charToLower(code);
  }

  return codesToString(codes);
}

std::vector<std::string> tokenize(const std::string &text) {
  std::vector<std::string> tokens;
  std::vector<uint32_t> codes = stringToCodes(text);
  std::vector<uint32_t> currentToken;

  for (uint32_t codepoint : codes) {
    if (isValidSymbol(codepoint)) {
      currentToken.push_back(charToLower(codepoint));
    } else {
      if (!currentToken.empty()) {
        tokens.push_back(codesToString(currentToken));
        currentToken.clear();
      }
    }
  }

  if (!currentToken.empty()) {
    tokens.push_back(codesToString(currentToken));
  }

  return tokens;
}

} // namespace TextUtils

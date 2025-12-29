#include "compression_utils.hpp"
#include <algorithm>
#include <stdexcept>

namespace CompressionUtils {

void vbyteEncode(int value, std::vector<uint8_t> &output) {
  if (value < 0) {
    throw std::invalid_argument("VByte encoding requires non-negative values");
  }

  while (value >= 128) {
    output.push_back(static_cast<uint8_t>(value & 0x7F));
    value >>= 7;
  }

  output.push_back(static_cast<uint8_t>(value | 0x80));
}

int vbyteDecode(const std::vector<uint8_t> &data, size_t &offset) {
  if (offset >= data.size()) {
    throw std::out_of_range("VByte decode: offset out of range");
  }

  int result = 0;
  int shift = 0;

  while (offset < data.size()) {
    uint8_t byte = data[offset++];
    result |= (byte & 0x7F) << shift;

    if (byte & 0x80) {
      break;
    }

    shift += 7;

    if (shift > 28) {
      throw std::overflow_error("VByte decode: number too large");
    }
  }

  return result;
}

std::vector<uint8_t>
compressPostingList(const std::vector<std::pair<int, int>> &postings) {
  if (postings.empty()) {
    return std::vector<uint8_t>();
  }

  std::vector<uint8_t> compressed;
  compressed.reserve(postings.size() * 3);

  int lastDocId = 0;

  for (const auto &posting : postings) {
    int docId = posting.first;
    int frequency = posting.second;

    if (docId < lastDocId) {
      throw std::invalid_argument("Posting list must be sorted by docId");
    }
    if (frequency <= 0) {
      throw std::invalid_argument("Frequency must be positive");
    }

    int delta = docId - lastDocId;
    vbyteEncode(delta, compressed);
    vbyteEncode(frequency, compressed);

    lastDocId = docId;
  }

  return compressed;
}

std::vector<std::pair<int, int>>
decompressPostingList(const std::vector<uint8_t> &data) {
  if (data.empty()) {
    return std::vector<std::pair<int, int>>();
  }

  std::vector<std::pair<int, int>> postings;
  size_t offset = 0;
  int lastDocId = 0;

  try {
    while (offset < data.size()) {
      int delta = vbyteDecode(data, offset);
      int frequency = vbyteDecode(data, offset);
      lastDocId += delta;
      postings.emplace_back(lastDocId, frequency);
    }
  } catch (const std::exception &e) {
    throw std::runtime_error(std::string("Error decompressing posting list: ") +
                             e.what());
  }

  return postings;
}

size_t vbyteSize(int value) {
  if (value < 0) {
    return 0;
  }

  if (value == 0) {
    return 1;
  }

  size_t size = 0;
  while (value > 0) {
    size++;
    value >>= 7;
  }

  return size;
}

size_t
estimateCompressedSize(const std::vector<std::pair<int, int>> &postings) {
  if (postings.empty()) {
    return 0;
  }

  size_t totalSize = 0;
  int lastDocId = 0;

  for (const auto &posting : postings) {
    int delta = posting.first - lastDocId;
    int frequency = posting.second;

    totalSize += vbyteSize(delta);
    totalSize += vbyteSize(frequency);

    lastDocId = posting.first;
  }

  return totalSize;
}

bool validateCompressedData(const std::vector<uint8_t> &data) {
  if (data.empty()) {
    return true;
  }

  size_t offset = 0;
  int lastDocId = 0;

  try {
    while (offset < data.size()) {
      int delta = vbyteDecode(data, offset);
      if (delta < 0) {
        return false;
      }

      int frequency = vbyteDecode(data, offset);
      if (frequency <= 0) {
        return false;
      }

      lastDocId += delta;

      if (lastDocId < 0 || lastDocId > 1000000000) {
        return false;
      }
    }

    return true;

  } catch (...) {
    return false;
  }
}

} // namespace CompressionUtils

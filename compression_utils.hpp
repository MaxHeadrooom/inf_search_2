#ifndef COMPRESSION_UTILS_HPP
#define COMPRESSION_UTILS_HPP

#include <cstdint>
#include <utility>
#include <vector>

namespace CompressionUtils {

// ============================================================================
// VByte encoding/decoding
// ============================================================================

/**
 * @brief Кодирует целое число в VByte формат
 * @param value Число для кодирования
 * @param output Вектор для записи закодированных байтов
 */
void vbyteEncode(int value, std::vector<uint8_t> &output);

/**
 * @brief Декодирует число из VByte формата
 * @param data Вектор с закодированными данными
 * @param offset Позиция начала чтения (будет изменена после чтения)
 * @return Декодированное число
 */
int vbyteDecode(const std::vector<uint8_t> &data, size_t &offset);

// ============================================================================
// Posting list compression
// ============================================================================

/**
 * @brief Сжимает posting list (список документов и частот)
 * @param postings Вектор пар (docId, frequency)
 * @return Сжатое представление в виде байтового массива
 */
std::vector<uint8_t>
compressPostingList(const std::vector<std::pair<int, int>> &postings);

/**
 * @brief Распаковывает posting list
 * @param data Сжатые данные
 * @return Вектор пар (docId, frequency)
 */
std::vector<std::pair<int, int>>
decompressPostingList(const std::vector<uint8_t> &data);

// ============================================================================
// Utility functions
// ============================================================================

/**
 * @brief Вычисляет размер числа в VByte формате
 * @param value Число для оценки
 * @return Количество байтов, необходимых для кодирования
 */
size_t vbyteSize(int value);

/**
 * @brief Оценивает размер сжатого posting list
 * @param postings Список для оценки
 * @return Примерный размер в байтах
 */
size_t estimateCompressedSize(const std::vector<std::pair<int, int>> &postings);

/**
 * @brief Проверяет корректность сжатых данных
 * @param data Данные для проверки
 * @return true если данные валидны
 */
bool validateCompressedData(const std::vector<uint8_t> &data);

} // namespace CompressionUtils

#endif // COMPRESSION_UTILS_HPP

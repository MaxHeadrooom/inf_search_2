#ifndef SEARCH_ENGINE_HPP
#define SEARCH_ENGINE_HPP

#include <cmath>
#include <cstdint>
#include <map>
#include <set>
#include <string>
#include <vector>

// ============================================================================
// Hasher
// ============================================================================

struct Hasher {
  size_t operator()(const std::string &key) const;
  size_t operator()(int key) const;
};

// ============================================================================
// CustomHashMap
// ============================================================================

template <typename K, typename V> class CustomHashMap {
public:
  static constexpr size_t HASH_SIZE = 10000;

  void insert(const K &key, const V &value);
  V *find(const K &key);
  const V *find(const K &key) const;
  bool count(const K &key) const;
  V &operator[](const K &key);
  size_t size() const;

  class Iterator {
  public:
    Iterator(std::vector<std::pair<K, V>> *base, size_t idx);
    std::pair<K, V> &operator*();
    Iterator &operator++();
    bool operator!=(const Iterator &other) const;

  private:
    void skip();
    std::vector<std::pair<K, V>> *m_base;
    size_t m_bucketIdx;
    typename std::vector<std::pair<K, V>>::iterator m_it;
  };

  class ConstIterator {
  public:
    ConstIterator(const std::vector<std::pair<K, V>> *base, size_t idx);
    const std::pair<K, V> &operator*() const;
    ConstIterator &operator++();
    bool operator!=(const ConstIterator &other) const;

  private:
    void skip();
    const std::vector<std::pair<K, V>> *m_base;
    size_t m_bucketIdx;
    typename std::vector<std::pair<K, V>>::const_iterator m_it;
  };

  Iterator begin();
  Iterator end();
  ConstIterator begin() const;
  ConstIterator end() const;

private:
  std::vector<std::pair<K, V>> m_buckets[HASH_SIZE];
  Hasher m_hasher;
};

// ============================================================================
// CustomHashMap Implementation
// ============================================================================

template <typename K, typename V>
void CustomHashMap<K, V>::insert(const K &key, const V &value) {
  size_t index = m_hasher(key);
  for (auto &p : m_buckets[index]) {
    if (p.first == key) {
      p.second = value;
      return;
    }
  }
  m_buckets[index].push_back({key, value});
}

template <typename K, typename V> V *CustomHashMap<K, V>::find(const K &key) {
  size_t index = m_hasher(key);
  for (auto &p : m_buckets[index]) {
    if (p.first == key)
      return &p.second;
  }
  return nullptr;
}

template <typename K, typename V>
const V *CustomHashMap<K, V>::find(const K &key) const {
  size_t index = m_hasher(key);
  for (const auto &p : m_buckets[index]) {
    if (p.first == key)
      return &p.second;
  }
  return nullptr;
}

template <typename K, typename V>
bool CustomHashMap<K, V>::count(const K &key) const {
  return find(key) != nullptr;
}

template <typename K, typename V>
V &CustomHashMap<K, V>::operator[](const K &key) {
  V *ptr = find(key);
  if (!ptr) {
    insert(key, V{});
    return *find(key);
  }
  return *ptr;
}

template <typename K, typename V> size_t CustomHashMap<K, V>::size() const {
  size_t total = 0;
  for (size_t i = 0; i < HASH_SIZE; ++i)
    total += m_buckets[i].size();
  return total;
}

// Iterator (non-const)
template <typename K, typename V>
CustomHashMap<K, V>::Iterator::Iterator(std::vector<std::pair<K, V>> *base,
                                        size_t idx)
    : m_base(base), m_bucketIdx(idx) {
  if (m_bucketIdx < HASH_SIZE) {
    m_it = m_base[m_bucketIdx].begin();
    skip();
  }
}

template <typename K, typename V> void CustomHashMap<K, V>::Iterator::skip() {
  while (m_bucketIdx < HASH_SIZE && m_it == m_base[m_bucketIdx].end()) {
    m_bucketIdx++;
    if (m_bucketIdx < HASH_SIZE)
      m_it = m_base[m_bucketIdx].begin();
  }
}

template <typename K, typename V>
std::pair<K, V> &CustomHashMap<K, V>::Iterator::operator*() {
  return *m_it;
}

template <typename K, typename V>
typename CustomHashMap<K, V>::Iterator &
CustomHashMap<K, V>::Iterator::operator++() {
  ++m_it;
  skip();
  return *this;
}

template <typename K, typename V>
bool CustomHashMap<K, V>::Iterator::operator!=(const Iterator &other) const {
  return m_bucketIdx != other.m_bucketIdx ||
         (m_bucketIdx < HASH_SIZE && m_it != other.m_it);
}

// ConstIterator
template <typename K, typename V>
CustomHashMap<K, V>::ConstIterator::ConstIterator(
    const std::vector<std::pair<K, V>> *base, size_t idx)
    : m_base(base), m_bucketIdx(idx) {
  if (m_bucketIdx < HASH_SIZE) {
    m_it = m_base[m_bucketIdx].begin();
    skip();
  }
}

template <typename K, typename V>
void CustomHashMap<K, V>::ConstIterator::skip() {
  while (m_bucketIdx < HASH_SIZE && m_it == m_base[m_bucketIdx].end()) {
    m_bucketIdx++;
    if (m_bucketIdx < HASH_SIZE)
      m_it = m_base[m_bucketIdx].begin();
  }
}

template <typename K, typename V>
const std::pair<K, V> &CustomHashMap<K, V>::ConstIterator::operator*() const {
  return *m_it;
}

template <typename K, typename V>
typename CustomHashMap<K, V>::ConstIterator &
CustomHashMap<K, V>::ConstIterator::operator++() {
  ++m_it;
  skip();
  return *this;
}

template <typename K, typename V>
bool CustomHashMap<K, V>::ConstIterator::operator!=(
    const ConstIterator &other) const {
  return m_bucketIdx != other.m_bucketIdx ||
         (m_bucketIdx < HASH_SIZE && m_it != other.m_it);
}

// begin() / end()
template <typename K, typename V>
typename CustomHashMap<K, V>::Iterator CustomHashMap<K, V>::begin() {
  return Iterator(m_buckets, 0);
}

template <typename K, typename V>
typename CustomHashMap<K, V>::Iterator CustomHashMap<K, V>::end() {
  return Iterator(m_buckets, HASH_SIZE);
}

template <typename K, typename V>
typename CustomHashMap<K, V>::ConstIterator CustomHashMap<K, V>::begin() const {
  return ConstIterator(m_buckets, 0);
}

template <typename K, typename V>
typename CustomHashMap<K, V>::ConstIterator CustomHashMap<K, V>::end() const {
  return ConstIterator(m_buckets, HASH_SIZE);
}

// ============================================================================
// SearchEngine
// ============================================================================

class SearchEngine {
public:
  explicit SearchEngine(const std::string &configDir = ".");

  SearchEngine(const std::string &dataDir, const std::string &dictPath,
               const std::string &indexDir);

  bool initialize();
  void run();

  void indexDocuments();
  bool saveIndex();
  bool loadIndex();

  void performBooleanSearch();
  void performTfIdfSearch();

  void analyzeZipfLaw();

private:
  struct Config {
    std::string dataDir;
    std::string dictPath;
    std::string invIndexPath;
    std::string docNamesPath;
    std::string docLengthsPath;
    std::string docUrlsPath;

    double minTfIdfScore = 0.05;
    size_t topKResults = 10;
    size_t zipfTopTerms = 15;
  };

  Config m_config;

  CustomHashMap<std::string, std::string> m_lemmas;
  CustomHashMap<std::string, std::vector<uint8_t>> m_invertedIndex;
  CustomHashMap<int, std::string> m_docNames;
  CustomHashMap<int, int> m_docLengths;
  CustomHashMap<int, std::string> m_docUrls;
  long long m_totalDocsCount;

  struct BooleanQuery {
    std::vector<std::string> requiredTerms;
    std::vector<std::string> excludedTerms;
    std::vector<std::string> optionalTerms;

    bool hasRequiredTerms() const { return !requiredTerms.empty(); }
    bool hasOptionalTerms() const { return !optionalTerms.empty(); }
  };

  BooleanQuery parseBooleanQuery(const std::string &query) const;
  std::set<int> getDocumentsForTerm(const std::string &term) const;
  std::vector<int> executeBooleanQuery(const BooleanQuery &query) const;
  bool
  verifyRequiredTermsInDocument(int docId,
                                const std::vector<std::string> &terms) const;

  struct ScoredDocument {
    int docId;
    double score;

    bool operator>(const ScoredDocument &other) const {
      return score > other.score;
    }
  };

  std::map<int, double>
  calculateTfIdfScores(const std::vector<std::string> &queryTerms) const;

  double calculateTfIdf(int termFreq, int docLength, int docsWithTerm) const;

  std::vector<ScoredDocument>
  rankDocuments(const std::map<int, double> &scores) const;

  bool loadDictionary();
  bool loadDocUrls();
  bool loadIndexMetadata();
  bool saveIndexMetadata();

  struct DocumentStats {
    int docId;
    std::string filename;
    int wordCount;
    std::map<std::string, int> termFrequencies;
  };

  DocumentStats processDocument(const std::string &filePath, int docId);

  void buildInvertedIndex(const std::vector<DocumentStats> &docStats);

  std::string getDocumentUrl(int docId) const;
  std::string getDocumentPath(int docId) const;
  void displayMenu() const;
  void displaySearchResults(const std::vector<int> &docIds) const;
  void displayTfIdfResults(const std::vector<ScoredDocument> &results) const;

  struct TermStatistics {
    std::string term;
    int totalFrequency;
    int documentFrequency;
  };

  std::vector<TermStatistics> getTermStatistics() const;
  void displayZipfAnalysis(const std::vector<TermStatistics> &stats) const;
};

#endif // SEARCH_ENGINE_HPP

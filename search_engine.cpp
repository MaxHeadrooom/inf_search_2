#include "search_engine.hpp"
#include "compression_utils.hpp"
#include "file_utils.hpp"
#include "text_utils.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace fs = std::filesystem;

size_t Hasher::operator()(const std::string &key) const {
  size_t hash = 0;
  for (unsigned char c : key) {
    hash = (hash * 31 + c);
  }
  return hash % 10000;
}

size_t Hasher::operator()(int key) const {
  return static_cast<size_t>(std::abs(key)) % 10000;
}

SearchEngine::SearchEngine(const std::string &configDir) : m_totalDocsCount(0) {
  m_config.dataDir = configDir + "/dataset_txt";
  m_config.dictPath = configDir + "/resources/lemmas.txt";
  m_config.invIndexPath = configDir + "/inverted_index.bin";
  m_config.docNamesPath = configDir + "/doc_names.txt";
  m_config.docLengthsPath = configDir + "/doc_lengths.txt";
  m_config.docUrlsPath = configDir + "/urls.txt";
}

SearchEngine::SearchEngine(const std::string &dataDir,
                           const std::string &dictPath,
                           const std::string &indexDir)
    : m_totalDocsCount(0) {

  m_config.dataDir = dataDir;
  m_config.dictPath = dictPath;
  m_config.invIndexPath = indexDir + "/inverted_index.bin";
  m_config.docNamesPath = indexDir + "/doc_names.txt";
  m_config.docLengthsPath = indexDir + "/doc_lengths.txt";
  m_config.docUrlsPath = indexDir + "/urls.txt";
}

bool SearchEngine::initialize() {
  std::cout << "=== Initializing Search Engine ===\n";

  if (!loadDictionary()) {
    std::cerr << "Error: Failed to load dictionary from " << m_config.dictPath
              << std::endl;
    return false;
  }
  std::cout << "Dictionary loaded: " << m_lemmas.size() << " lemmas\n";

  if (!loadDocUrls()) {
    std::cerr << "Warning: Failed to load document URLs from "
              << m_config.docUrlsPath << std::endl;
  }

  return true;
}

void SearchEngine::run() {
  while (true) {
    displayMenu();

    int choice;
    std::cin >> choice;
    std::cin.ignore();

    if (choice == 4) {
      std::cout << "Exiting...\n";
      break;
    }

    switch (choice) {
    case 1:
      indexDocuments();
      saveIndex();
      analyzeZipfLaw();
      break;

    case 2:
      if (m_invertedIndex.size() == 0) {
        if (!loadIndex()) {
          std::cout << "No index found. Please rebuild (option 1).\n";
          continue;
        }
      }
      performBooleanSearch();
      break;

    case 3:
      if (m_invertedIndex.size() == 0) {
        if (!loadIndex()) {
          std::cout << "No index found. Please rebuild (option 1).\n";
          continue;
        }
      }
      performTfIdfSearch();
      break;

    default:
      std::cout << "Invalid choice. Please try again.\n";
    }
  }
}

void SearchEngine::indexDocuments() {
  std::cout << "\n=== Starting Indexing ===\n";
  std::cout << "Scanning directory: " << m_config.dataDir << "\n";

  if (!fs::exists(m_config.dataDir)) {
    std::cerr << "Error: Directory does not exist: " << m_config.dataDir
              << std::endl;
    return;
  }

  m_invertedIndex = CustomHashMap<std::string, std::vector<uint8_t>>();
  m_docNames = CustomHashMap<int, std::string>();
  m_docLengths = CustomHashMap<int, int>();

  std::map<std::string, std::vector<std::pair<int, int>>> tempPostings;

  int docId = 0;
  int filesProcessed = 0;

  try {
    for (const auto &entry : fs::directory_iterator(m_config.dataDir)) {
      if (entry.path().extension() != ".txt") {
        continue;
      }

      docId++;
      filesProcessed++;

      if (filesProcessed % 100 == 0) {
        std::cout << "Processed " << filesProcessed << " documents...\r"
                  << std::flush;
      }

      DocumentStats stats = processDocument(entry.path().string(), docId);

      m_docNames.insert(docId, stats.filename);
      m_docLengths.insert(docId, stats.wordCount);

      for (const auto &termFreq : stats.termFrequencies) {
        tempPostings[termFreq.first].emplace_back(docId, termFreq.second);
      }
    }
  } catch (const std::exception &e) {
    std::cerr << "\nError during indexing: " << e.what() << std::endl;
    return;
  }

  m_totalDocsCount = docId;
  std::cout << "\n\nDocuments processed: " << m_totalDocsCount << "\n";
  std::cout << "Building inverted index...\n";

  int termsProcessed = 0;
  for (auto &entry : tempPostings) {
    const std::string &term = entry.first;
    auto &postings = entry.second;

    std::sort(postings.begin(), postings.end());

    std::vector<uint8_t> compressed =
        CompressionUtils::compressPostingList(postings);

    m_invertedIndex.insert(term, std::move(compressed));

    termsProcessed++;
    if (termsProcessed % 1000 == 0) {
      std::cout << "Compressed " << termsProcessed << " terms...\r"
                << std::flush;
    }
  }

  std::cout << "\n\nIndexing completed!\n";
  std::cout << "Total documents: " << m_totalDocsCount << "\n";
  std::cout << "Total unique terms: " << m_invertedIndex.size() << "\n";
}

SearchEngine::DocumentStats
SearchEngine::processDocument(const std::string &filePath, int docId) {

  DocumentStats stats;
  stats.docId = docId;
  stats.filename = fs::path(filePath).filename().string();

  std::ifstream file(filePath);
  if (!file.is_open()) {
    std::cerr << "Warning: Cannot open file " << filePath << std::endl;
    return stats;
  }

  std::string content((std::istreambuf_iterator<char>(file)),
                      std::istreambuf_iterator<char>());
  file.close();

  std::vector<std::string> tokens = TextUtils::tokenize(content);
  stats.wordCount = tokens.size();

  for (const auto &token : tokens) {
    stats.termFrequencies[token]++;
  }

  return stats;
}

bool SearchEngine::saveIndex() {
  std::cout << "\n=== Saving Index ===\n";

  std::ofstream invFile(m_config.invIndexPath, std::ios::binary);
  if (!invFile.is_open()) {
    std::cerr << "Error: Cannot save inverted index to "
              << m_config.invIndexPath << std::endl;
    return false;
  }

  for (auto &entry : m_invertedIndex) {
    const std::string &term = entry.first;
    const std::vector<uint8_t> &data = entry.second;

    uint32_t termLen = term.size();
    invFile.write(reinterpret_cast<const char *>(&termLen), sizeof(termLen));
    invFile.write(term.c_str(), termLen);

    uint32_t dataSize = data.size();
    invFile.write(reinterpret_cast<const char *>(&dataSize), sizeof(dataSize));
    invFile.write(reinterpret_cast<const char *>(data.data()), dataSize);
  }
  invFile.close();
  std::cout << "Inverted index saved: " << m_config.invIndexPath << "\n";

  std::ofstream lenFile(m_config.docLengthsPath);
  if (!lenFile.is_open()) {
    std::cerr << "Warning: Cannot save document lengths\n";
  } else {
    for (auto &entry : m_docLengths) {
      lenFile << entry.first << " " << entry.second << "\n";
    }
    lenFile.close();
    std::cout << "Document lengths saved: " << m_config.docLengthsPath << "\n";
  }

  std::ofstream namesFile(m_config.docNamesPath);
  if (!namesFile.is_open()) {
    std::cerr << "Warning: Cannot save document names\n";
  } else {
    for (auto &entry : m_docNames) {
      namesFile << entry.first << " " << entry.second << "\n";
    }
    namesFile.close();
    std::cout << "Document names saved: " << m_config.docNamesPath << "\n";
  }

  std::cout << "Index saved successfully!\n";
  return true;
}

bool SearchEngine::loadIndex() {
  std::cout << "\n=== Loading Index ===\n";

  std::ifstream invFile(m_config.invIndexPath, std::ios::binary);
  if (!invFile.is_open()) {
    std::cerr << "Error: Cannot load inverted index from "
              << m_config.invIndexPath << std::endl;
    return false;
  }

  m_invertedIndex = CustomHashMap<std::string, std::vector<uint8_t>>();

  while (invFile.peek() != EOF) {

    uint32_t termLen;
    if (!invFile.read(reinterpret_cast<char *>(&termLen), sizeof(termLen))) {
      break;
    }

    std::string term(termLen, 0);
    invFile.read(&term[0], termLen);

    uint32_t dataSize;
    invFile.read(reinterpret_cast<char *>(&dataSize), sizeof(dataSize));

    std::vector<uint8_t> data(dataSize);
    invFile.read(reinterpret_cast<char *>(data.data()), dataSize);

    m_invertedIndex.insert(term, std::move(data));
  }
  invFile.close();
  std::cout << "Inverted index loaded: " << m_invertedIndex.size()
            << " terms\n";

  if (!loadIndexMetadata()) {
    return false;
  }

  m_totalDocsCount = m_docLengths.size();
  std::cout << "Total documents: " << m_totalDocsCount << "\n";
  std::cout << "Index loaded successfully!\n";

  return true;
}

bool SearchEngine::loadIndexMetadata() {

  std::ifstream lenFile(m_config.docLengthsPath);
  if (!lenFile.is_open()) {
    std::cerr << "Error: Cannot load document lengths from "
              << m_config.docLengthsPath << std::endl;
    return false;
  }

  m_docLengths = CustomHashMap<int, int>();
  int id, length;
  while (lenFile >> id >> length) {
    m_docLengths.insert(id, length);
  }
  lenFile.close();

  std::ifstream namesFile(m_config.docNamesPath);
  if (!namesFile.is_open()) {
    std::cerr << "Warning: Cannot load document names\n";
  } else {
    m_docNames = CustomHashMap<int, std::string>();
    std::string name;
    while (namesFile >> id >> std::ws && std::getline(namesFile, name)) {
      if (!name.empty()) {
        m_docNames.insert(id, name);
      }
    }
    namesFile.close();
  }

  return true;
}

bool SearchEngine::saveIndexMetadata() { return true; }

bool SearchEngine::loadDictionary() {
  std::ifstream file(m_config.dictPath);
  if (!file.is_open()) {
    return false;
  }

  m_lemmas = CustomHashMap<std::string, std::string>();
  std::string key, value;
  int count = 0;

  while (file >> key >> value) {
    m_lemmas.insert(TextUtils::toLowerCase(key), TextUtils::toLowerCase(value));
    count++;
  }

  file.close();
  return count > 0;
}

bool SearchEngine::loadDocUrls() {
  std::ifstream file(m_config.docUrlsPath);
  if (!file.is_open()) {
    return false;
  }

  m_docUrls = CustomHashMap<int, std::string>();
  std::string line;
  int loaded = 0;

  while (std::getline(file, line)) {
    if (line.empty()) {
      continue;
    }

    std::stringstream ss(line);
    int id;
    std::string url;

    if (ss >> id && std::getline(ss, url)) {

      size_t start = url.find_first_not_of(" \t");
      if (start != std::string::npos) {
        url = url.substr(start);
      }
      m_docUrls.insert(id, url);
      loaded++;
    }
  }

  file.close();
  return loaded > 0;
}

void SearchEngine::performBooleanSearch() {
  std::cout << "\n=== BOOLEAN SEARCH ===\n";
  std::cout << "Syntax: +required -excluded optional\n";
  std::cout << "Type 'exit' to return to main menu\n\n";

  std::string queryStr;

  while (true) {
    std::cout << "Bool Query: ";
    std::cout.flush();

    if (!std::getline(std::cin, queryStr)) {
      break;
    }

    if (queryStr == "exit") {
      break;
    }

    if (queryStr.empty()) {
      std::cout << "Results: No documents match.\n\n";
      continue;
    }

    BooleanQuery query = parseBooleanQuery(queryStr);

    std::vector<int> results = executeBooleanQuery(query);

    displaySearchResults(results);
    std::cout << "\n";
  }
}

SearchEngine::BooleanQuery
SearchEngine::parseBooleanQuery(const std::string &queryStr) const {

  BooleanQuery query;
  std::stringstream ss(queryStr);
  std::string token;

  while (ss >> token) {
    char prefix = ' ';
    std::string rawWord = token;

    if (token.size() > 1 && (token[0] == '+' || token[0] == '-')) {
      prefix = token[0];
      rawWord = token.substr(1);
    }

    std::vector<std::string> parsed = TextUtils::tokenize(rawWord);
    if (parsed.empty()) {
      continue;
    }

    std::string term = parsed[0];

    if (prefix == '+') {
      query.requiredTerms.push_back(term);
    } else if (prefix == '-') {
      query.excludedTerms.push_back(term);
    } else {
      query.optionalTerms.push_back(term);
    }
  }

  return query;
}

std::set<int> SearchEngine::getDocumentsForTerm(const std::string &term) const {
  std::set<int> docIds;

  const std::vector<uint8_t> *data = m_invertedIndex.find(term);
  if (!data) {
    return docIds;
  }

  auto postings = CompressionUtils::decompressPostingList(*data);
  for (const auto &posting : postings) {
    docIds.insert(posting.first);
  }

  return docIds;
}

std::vector<int>
SearchEngine::executeBooleanQuery(const BooleanQuery &query) const {

  std::set<int> candidateSet;
  bool hasCandidates = false;

  if (query.hasRequiredTerms()) {
    for (const auto &term : query.requiredTerms) {
      std::set<int> termDocs = getDocumentsForTerm(term);

      if (termDocs.empty()) {

        return std::vector<int>();
      }

      if (!hasCandidates) {
        candidateSet = std::move(termDocs);
        hasCandidates = true;
      } else {

        std::set<int> intersection;
        std::set_intersection(
            candidateSet.begin(), candidateSet.end(), termDocs.begin(),
            termDocs.end(), std::inserter(intersection, intersection.begin()));
        candidateSet = std::move(intersection);
      }

      if (candidateSet.empty()) {
        return std::vector<int>();
      }
    }
  }

  if (!query.hasRequiredTerms() && query.hasOptionalTerms()) {
    for (const auto &term : query.optionalTerms) {
      std::set<int> termDocs = getDocumentsForTerm(term);
      candidateSet.insert(termDocs.begin(), termDocs.end());
    }
    hasCandidates = true;
  }

  if (!hasCandidates) {
    return std::vector<int>();
  }

  if (!query.excludedTerms.empty()) {
    std::set<int> excludedDocs;
    for (const auto &term : query.excludedTerms) {
      std::set<int> termDocs = getDocumentsForTerm(term);
      excludedDocs.insert(termDocs.begin(), termDocs.end());
    }

    std::set<int> filteredSet;
    std::set_difference(candidateSet.begin(), candidateSet.end(),
                        excludedDocs.begin(), excludedDocs.end(),
                        std::inserter(filteredSet, filteredSet.begin()));
    candidateSet = std::move(filteredSet);
  }

  if (query.hasRequiredTerms()) {
    std::vector<int> verified;
    for (int docId : candidateSet) {
      if (verifyRequiredTermsInDocument(docId, query.requiredTerms)) {
        verified.push_back(docId);
      }
    }
    return verified;
  }

  return std::vector<int>(candidateSet.begin(), candidateSet.end());
}

bool SearchEngine::verifyRequiredTermsInDocument(
    int docId, const std::vector<std::string> &terms) const {

  std::string docPath = getDocumentPath(docId);
  std::string content = FileUtils::readFileContent(docPath);

  if (content.empty()) {
    return false;
  }

  std::string lowerContent = TextUtils::toLowerCase(content);

  for (const std::string &term : terms) {
    if (lowerContent.find(term) == std::string::npos) {
      return false;
    }
  }

  return true;
}

void SearchEngine::performTfIdfSearch() {
  std::cout << "\n=== TF-IDF SEARCH ===\n";
  std::cout << "Type 'exit' to return to main menu\n\n";

  std::string queryStr;

  while (true) {
    std::cout << "TF-IDF Query: ";
    std::cout.flush();

    if (!std::getline(std::cin, queryStr)) {
      break;
    }

    if (queryStr == "exit") {
      break;
    }

    if (queryStr.empty()) {
      std::cout << "No query terms.\n\n";
      continue;
    }

    std::vector<std::string> queryTerms = TextUtils::tokenize(queryStr);

    if (queryTerms.empty()) {
      std::cout << "No valid query terms.\n\n";
      continue;
    }

    std::map<int, double> scores = calculateTfIdfScores(queryTerms);

    if (scores.empty()) {
      std::cout << "No matching documents found.\n\n";
      continue;
    }

    std::vector<ScoredDocument> rankedResults = rankDocuments(scores);

    displayTfIdfResults(rankedResults);
    std::cout << "\n";
  }
}

std::map<int, double> SearchEngine::calculateTfIdfScores(
    const std::vector<std::string> &queryTerms) const {

  std::map<int, double> scores;

  for (const std::string &term : queryTerms) {
    const std::vector<uint8_t> *data = m_invertedIndex.find(term);
    if (!data) {
      continue;
    }

    auto postings = CompressionUtils::decompressPostingList(*data);

    double idf =
        std::log(static_cast<double>(m_totalDocsCount) / postings.size());

    for (const auto &posting : postings) {
      int docId = posting.first;
      int termFreq = posting.second;

      const int *docLenPtr = m_docLengths.find(docId);
      if (!docLenPtr || *docLenPtr == 0) {
        continue;
      }

      int docLength = *docLenPtr;

      double tf = static_cast<double>(termFreq) / docLength;

      scores[docId] += tf * idf;
    }
  }

  return scores;
}

double SearchEngine::calculateTfIdf(int termFreq, int docLength,
                                    int docsWithTerm) const {

  double tf = static_cast<double>(termFreq) / docLength;
  double idf = std::log(static_cast<double>(m_totalDocsCount) / docsWithTerm);
  return tf * idf;
}

std::vector<SearchEngine::ScoredDocument>
SearchEngine::rankDocuments(const std::map<int, double> &scores) const {

  std::vector<ScoredDocument> results;
  results.reserve(scores.size());

  for (const auto &entry : scores) {
    if (entry.second >= m_config.minTfIdfScore) {
      results.push_back({entry.first, entry.second});
    }
  }

  std::sort(results.begin(), results.end(),
            [](const ScoredDocument &a, const ScoredDocument &b) {
              return a.score > b.score;
            });

  return results;
}

void SearchEngine::analyzeZipfLaw() {
  std::cout << "\n=== ZIPF'S LAW ANALYSIS ===\n";

  auto stats = getTermStatistics();
  displayZipfAnalysis(stats);
}

std::vector<SearchEngine::TermStatistics>
SearchEngine::getTermStatistics() const {
  std::vector<TermStatistics> stats;

  for (auto &entry : m_invertedIndex) {
    const std::string &term = entry.first;
    const std::vector<uint8_t> &data = entry.second;

    auto postings = CompressionUtils::decompressPostingList(data);

    TermStatistics termStat;
    termStat.term = term;
    termStat.documentFrequency = postings.size();
    termStat.totalFrequency = 0;

    for (const auto &posting : postings) {
      termStat.totalFrequency += posting.second;
    }

    stats.push_back(termStat);
  }

  std::sort(stats.begin(), stats.end(),
            [](const TermStatistics &a, const TermStatistics &b) {
              return a.totalFrequency > b.totalFrequency;
            });

  return stats;
}

void SearchEngine::displayZipfAnalysis(
    const std::vector<TermStatistics> &stats) const {

  std::cout << std::left << std::setw(20) << "Term" << std::setw(15)
            << "Frequency" << std::setw(10) << "Rank" << "F × R\n";
  std::cout << std::string(55, '-') << "\n";

  size_t limit = std::min(stats.size(), m_config.zipfTopTerms);

  for (size_t i = 0; i < limit; ++i) {
    int rank = i + 1;
    long long constant = static_cast<long long>(stats[i].totalFrequency) * rank;

    std::cout << std::left << std::setw(20) << stats[i].term << std::setw(15)
              << stats[i].totalFrequency << std::setw(10) << rank << constant
              << "\n";
  }

  std::cout
      << "\nZipf's law suggests F × R should be approximately constant.\n";
}

void SearchEngine::displayMenu() const {
  std::cout << "\n=== SEARCH ENGINE ===\n";
  std::cout << "1. Rebuild index\n";
  std::cout << "2. Boolean search\n";
  std::cout << "3. TF-IDF search\n";
  std::cout << "4. Exit\n";
  std::cout << "Choice: ";
}

void SearchEngine::displaySearchResults(const std::vector<int> &docIds) const {
  std::cout << "Results: ";

  if (docIds.empty()) {
    std::cout << "No documents match.";
  } else {
    std::cout << docIds.size() << " document(s) found\n";
    for (int docId : docIds) {
      std::string url = getDocumentUrl(docId);
      std::cout << "  " << url << "\n";
    }
  }
}

void SearchEngine::displayTfIdfResults(
    const std::vector<ScoredDocument> &results) const {

  size_t limit = std::min(results.size(), m_config.topKResults);

  std::cout << "Top " << limit << " results:\n";

  for (size_t i = 0; i < limit; ++i) {
    int docId = results[i].docId;
    double score = results[i].score;
    std::string url = getDocumentUrl(docId);

    std::cout << (i + 1) << ". " << url << " | Score: " << std::fixed
              << std::setprecision(6) << score << "\n";
  }
}

std::string SearchEngine::getDocumentUrl(int docId) const {
  const std::string *urlPtr = m_docUrls.find(docId);
  if (urlPtr) {
    return *urlPtr;
  }

  const std::string *namePtr = m_docNames.find(docId);
  if (namePtr) {
    return *namePtr;
  }

  return "[doc_" + std::to_string(docId) + "]";
}

std::string SearchEngine::getDocumentPath(int docId) const {
  const std::string *namePtr = m_docNames.find(docId);
  if (!namePtr) {
    return m_config.dataDir + "/" + std::to_string(docId) + ".txt";
  }
  return m_config.dataDir + "/" + *namePtr;
}

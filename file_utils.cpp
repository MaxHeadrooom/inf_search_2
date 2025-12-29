#include "file_utils.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <filesystem>

namespace fs = std::filesystem;

namespace FileUtils {

std::string readFileContent(const std::string &filePath) {
  std::ifstream file(filePath, std::ios::in | std::ios::binary);
  if (!file.is_open()) {
    std::cerr << "Warning: Cannot open file: " << filePath << std::endl;
    return "";
  }

  std::string content;

  file.seekg(0, std::ios::end);
  size_t size = file.tellg();
  file.seekg(0, std::ios::beg);

  if (size > 0) {
    content.reserve(size);
    content.assign(std::istreambuf_iterator<char>(file),
                   std::istreambuf_iterator<char>());
  }

  file.close();
  return content;
}

std::vector<std::string> readLines(const std::string &filePath) {
  std::vector<std::string> lines;
  std::ifstream file(filePath);

  if (!file.is_open()) {
    std::cerr << "Warning: Cannot open file: " << filePath << std::endl;
    return lines;
  }

  std::string line;
  while (std::getline(file, line)) {
    lines.push_back(line);
  }

  file.close();
  return lines;
}

bool fileExists(const std::string &filePath) {
  try {
    return fs::exists(filePath) && fs::is_regular_file(filePath);
  } catch (const fs::filesystem_error &e) {
    std::cerr << "Filesystem error: " << e.what() << std::endl;
    return false;
  }
}

size_t getFileSize(const std::string &filePath) {
  try {
    if (!fileExists(filePath)) {
      return 0;
    }
    return fs::file_size(filePath);
  } catch (const fs::filesystem_error &e) {
    std::cerr << "Error getting file size: " << e.what() << std::endl;
    return 0;
  }
}

bool directoryExists(const std::string &dirPath) {
  try {
    return fs::exists(dirPath) && fs::is_directory(dirPath);
  } catch (const fs::filesystem_error &e) {
    std::cerr << "Filesystem error: " << e.what() << std::endl;
    return false;
  }
}

bool createDirectory(const std::string &dirPath) {
  try {
    if (directoryExists(dirPath)) {
      return true;
    }
    return fs::create_directories(dirPath);
  } catch (const fs::filesystem_error &e) {
    std::cerr << "Error creating directory: " << e.what() << std::endl;
    return false;
  }
}

std::vector<std::string> listFiles(const std::string &dirPath,
                                   const std::string &extension,
                                   bool recursive) {

  std::vector<std::string> files;

  if (!directoryExists(dirPath)) {
    std::cerr << "Directory does not exist: " << dirPath << std::endl;
    return files;
  }

  try {
    if (recursive) {
      for (const auto &entry : fs::recursive_directory_iterator(dirPath)) {
        if (entry.is_regular_file()) {
          std::string path = entry.path().string();

          if (extension.empty() || entry.path().extension() == extension) {
            files.push_back(path);
          }
        }
      }
    } else {
      for (const auto &entry : fs::directory_iterator(dirPath)) {
        if (entry.is_regular_file()) {
          std::string path = entry.path().string();

          if (extension.empty() || entry.path().extension() == extension) {
            files.push_back(path);
          }
        }
      }
    }
  } catch (const fs::filesystem_error &e) {
    std::cerr << "Error listing files: " << e.what() << std::endl;
  }

  std::sort(files.begin(), files.end());

  return files;
}

size_t countFiles(const std::string &dirPath, const std::string &extension) {

  if (!directoryExists(dirPath)) {
    return 0;
  }

  size_t count = 0;

  try {
    for (const auto &entry : fs::directory_iterator(dirPath)) {
      if (entry.is_regular_file()) {
        if (extension.empty() || entry.path().extension() == extension) {
          count++;
        }
      }
    }
  } catch (const fs::filesystem_error &e) {
    std::cerr << "Error counting files: " << e.what() << std::endl;
  }

  return count;
}

bool loadKeyValueFile(const std::string &filePath,
                      std::map<int, std::string> &output) {

  std::ifstream file(filePath);
  if (!file.is_open()) {
    std::cerr << "Cannot open file: " << filePath << std::endl;
    return false;
  }

  output.clear();
  std::string line;
  int lineNum = 0;

  while (std::getline(file, line)) {
    lineNum++;

    if (line.empty()) {
      continue;
    }

    std::stringstream ss(line);
    int key;
    std::string value;

    if (ss >> key && std::getline(ss, value)) {

      size_t start = value.find_first_not_of(" \t");
      if (start != std::string::npos) {
        value = value.substr(start);
      }
      output[key] = value;
    } else {
      std::cerr << "Warning: Invalid format at line " << lineNum << " in "
                << filePath << std::endl;
    }
  }

  file.close();
  return !output.empty();
}

bool loadKeyValueFile(const std::string &filePath, std::map<int, int> &output) {

  std::ifstream file(filePath);
  if (!file.is_open()) {
    std::cerr << "Cannot open file: " << filePath << std::endl;
    return false;
  }

  output.clear();
  int key, value;

  while (file >> key >> value) {
    output[key] = value;
  }

  file.close();
  return !output.empty();
}

bool loadKeyValueFile(const std::string &filePath,
                      std::map<std::string, std::string> &output) {

  std::ifstream file(filePath);
  if (!file.is_open()) {
    std::cerr << "Cannot open file: " << filePath << std::endl;
    return false;
  }

  output.clear();
  std::string key, value;

  while (file >> key >> value) {
    output[key] = value;
  }

  file.close();
  return !output.empty();
}

bool saveKeyValueFile(const std::string &filePath,
                      const std::map<int, std::string> &data) {

  std::ofstream file(filePath);
  if (!file.is_open()) {
    std::cerr << "Cannot create file: " << filePath << std::endl;
    return false;
  }

  for (const auto &pair : data) {
    file << pair.first << " " << pair.second << "\n";
  }

  file.close();
  return true;
}

bool saveKeyValueFile(const std::string &filePath,
                      const std::map<int, int> &data) {

  std::ofstream file(filePath);
  if (!file.is_open()) {
    std::cerr << "Cannot create file: " << filePath << std::endl;
    return false;
  }

  for (const auto &pair : data) {
    file << pair.first << " " << pair.second << "\n";
  }

  file.close();
  return true;
}

bool saveKeyValueFile(const std::string &filePath,
                      const std::map<std::string, std::string> &data) {

  std::ofstream file(filePath);
  if (!file.is_open()) {
    std::cerr << "Cannot create file: " << filePath << std::endl;
    return false;
  }

  for (const auto &pair : data) {
    file << pair.first << " " << pair.second << "\n";
  }

  file.close();
  return true;
}

std::vector<uint8_t> readBinaryFile(const std::string &filePath) {
  std::ifstream file(filePath, std::ios::binary);
  if (!file.is_open()) {
    std::cerr << "Cannot open binary file: " << filePath << std::endl;
    return std::vector<uint8_t>();
  }

  file.seekg(0, std::ios::end);
  size_t size = file.tellg();
  file.seekg(0, std::ios::beg);

  std::vector<uint8_t> data(size);
  file.read(reinterpret_cast<char *>(data.data()), size);

  file.close();
  return data;
}

bool writeBinaryFile(const std::string &filePath,
                     const std::vector<uint8_t> &data) {

  std::ofstream file(filePath, std::ios::binary);
  if (!file.is_open()) {
    std::cerr << "Cannot create binary file: " << filePath << std::endl;
    return false;
  }

  file.write(reinterpret_cast<const char *>(data.data()), data.size());
  file.close();

  return true;
}

std::string getFileName(const std::string &filePath) {
  try {
    return fs::path(filePath).filename().string();
  } catch (const std::exception &e) {
    std::cerr << "Error getting filename: " << e.what() << std::endl;
    return "";
  }
}

std::string getFileNameWithoutExtension(const std::string &filePath) {
  try {
    return fs::path(filePath).stem().string();
  } catch (const std::exception &e) {
    std::cerr << "Error getting filename without extension: " << e.what()
              << std::endl;
    return "";
  }
}

std::string getFileExtension(const std::string &filePath) {
  try {
    return fs::path(filePath).extension().string();
  } catch (const std::exception &e) {
    std::cerr << "Error getting file extension: " << e.what() << std::endl;
    return "";
  }
}

std::string getDirectory(const std::string &filePath) {
  try {
    return fs::path(filePath).parent_path().string();
  } catch (const std::exception &e) {
    std::cerr << "Error getting directory: " << e.what() << std::endl;
    return "";
  }
}

std::string joinPath(const std::vector<std::string> &parts) {
  if (parts.empty()) {
    return "";
  }

  fs::path result = parts[0];
  for (size_t i = 1; i < parts.size(); ++i) {
    result /= parts[i];
  }

  return result.string();
}

std::string normalizePath(const std::string &path) {
  try {
    return fs::path(path).lexically_normal().string();
  } catch (const std::exception &e) {
    std::cerr << "Error normalizing path: " << e.what() << std::endl;
    return path;
  }
}

bool copyFile(const std::string &sourcePath, const std::string &destPath,
              bool overwrite) {

  try {
    if (!fileExists(sourcePath)) {
      std::cerr << "Source file does not exist: " << sourcePath << std::endl;
      return false;
    }

    if (fileExists(destPath) && !overwrite) {
      std::cerr << "Destination file already exists: " << destPath << std::endl;
      return false;
    }

    auto options = overwrite ? fs::copy_options::overwrite_existing
                             : fs::copy_options::none;

    fs::copy_file(sourcePath, destPath, options);
    return true;

  } catch (const fs::filesystem_error &e) {
    std::cerr << "Error copying file: " << e.what() << std::endl;
    return false;
  }
}

bool deleteFile(const std::string &filePath) {
  try {
    if (!fileExists(filePath)) {
      return true;
    }

    return fs::remove(filePath);

  } catch (const fs::filesystem_error &e) {
    std::cerr << "Error deleting file: " << e.what() << std::endl;
    return false;
  }
}

bool moveFile(const std::string &sourcePath, const std::string &destPath) {

  try {
    if (!fileExists(sourcePath)) {
      std::cerr << "Source file does not exist: " << sourcePath << std::endl;
      return false;
    }

    fs::rename(sourcePath, destPath);
    return true;

  } catch (const fs::filesystem_error &e) {
    std::cerr << "Error moving file: " << e.what() << std::endl;
    return false;
  }
}

} // namespace FileUtils

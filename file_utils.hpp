#ifndef FILE_UTILS_H
#define FILE_UTILS_H

#include <map>
#include <string>
#include <vector>

namespace FileUtils {

std::string readFileContent(const std::string &filePath);

bool fileExists(const std::string &filePath);

std::vector<std::string> listFiles(const std::string &dirPath,
                                   const std::string &extension = ".txt");

bool loadKeyValueFile(const std::string &filePath,
                      std::map<int, std::string> &output);

bool loadKeyValueFile(const std::string &filePath, std::map<int, int> &output);

bool saveKeyValueFile(const std::string &filePath,
                      const std::map<int, std::string> &data);

bool saveKeyValueFile(const std::string &filePath,
                      const std::map<int, int> &data);

}

#endif

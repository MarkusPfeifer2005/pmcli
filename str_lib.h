#ifndef STR_LIB_H
#define STR_LIB_H

#include <string>
#include <vector>

void splitString(std::string str, std::vector<std::string> &result, char delimiter);
std::string removeDublicateWhitespaces(std::string str);
std::string removeWhitespaces(std::string str);
std::string removeNonAlphaNum(std::string str);
std::string getShape(std::string& name, bool extract = false);

#endif
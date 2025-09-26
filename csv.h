#ifndef COMMA_SEPARATED_VALUE
#define COMMA_SEPARATED_VALUE

#include <iostream>
#include <string>
#include <vector>
#include <fstream>


class CSV {
    public:
        CSV(std::string path, char delimiter = ',');
        bool getRow(std::vector<std::string>& row);
        ~CSV();
        size_t getIndexOf(std::string name);
    private:
        std::ifstream file;
        char delimiter;
        std::vector<std::string> header;
};


#endif


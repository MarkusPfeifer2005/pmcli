#include "csv.h"
#include <iosfwd>
#include <stdexcept>
#include <string>
#include <iostream>
#include <fstream>
#include "str_lib.h"


CSV::CSV(std::string path, char delimiter): delimiter(delimiter) {
    this->file.open(path);
    if (!this->file) {
        throw std::ios_base::failure("Failed to open file!");
    }
    this->getRow(this->header);
    std::string line;
    std::streampos prevPos = this->file.tellg();
    while (std::getline(this->file, line)) {
        if (line.length() == 1) {
            prevPos = this->file.tellg();
        }
        else {
            this->file.seekg(prevPos);
            break;
        }
    }
}

bool CSV::getRow(std::vector<std::string>& row) {
    std::string line;
    if (!std::getline(this->file, line, '\n')) {
        return false;
    }
    row.clear();
    line.pop_back();  // Delete carrige return at the end of the line (ASCII 13).
    splitString(line, row, this->delimiter);
    return true;
}

CSV::~CSV() {
    this->file.close();
}

size_t CSV::getIndexOf(std::string name) {
    for (int i = 0; i < this->header.size(); i++) {
        if (name.compare(this->header[i]) == 0) {
            return i;
        }
    }
    throw std::out_of_range("Searched element '" + name + "' not contained in csv header!");
}


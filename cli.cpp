#include <filesystem>  // include before conio.h
#include <conio.h>  // https://zoelabbb.github.io/conio.h/ use the latest release
#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <sys/ioctl.h>
#include <vector>
#include "str_lib.h"


class Part {
    public:
    std::string number, name, location, queryName, shape;
    void print(bool highlighted=false);
    Part(std::string number, std::string name);
};

Part::Part(std::string number, std::string name) : number{number}, name{name} {
    this->queryName = name;
    this->queryName = removeNonAlphaNum(name);
    this->queryName = removeDublicateWhitespaces(this->queryName);
    this->shape = extractShape(name);
}

void Part::print(bool highlighted) {
    winsize terminalWidth;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &terminalWidth);
    if (highlighted) {
        std::cout << "\033[31m";
    }
    std::string filler;
    if ((name.length() + number.length() + 7) > terminalWidth.ws_col) {
        std::cout << name.substr(0, terminalWidth.ws_col - number.length() - 7) << " [...] " << number;
    }
    else {
        std::cout << name << std::string(terminalWidth.ws_col - name.length() - number.length(), ' ') << number;
    }
    if (highlighted) {
        std::cout << "\033[0m";
    }
    std::cout << std::endl;
}

bool partComparison(const Part* a, const Part* b) {
    return a->name < b->name;
}

void readCSV(std::vector<std::vector<std::string>>& data, const std::string path, char delimiter = ',', int numHeaderRows = 0) {
    std::ifstream csvFile(path);
    std::string line;
    for (int i = 0; i < numHeaderRows; i++) {
        std::getline(csvFile, line);
    }
    while (std::getline(csvFile, line)) {
        std::vector<std::string> entries;
        splitString(line, entries, delimiter);
        data.push_back(entries);
    }
    csvFile.close();
}

void loadBricklinkData(std::vector<Part> &database, const std::string path = "Parts.txt") {
    std::vector<std::vector<std::string>> rawData;
    readCSV(rawData, path, '\t', 3);
    for (auto row : rawData) {
        if (row[1].find(", Decorated") == std::string::npos && row[3].find("Duplo") == std::string::npos) {
            database.push_back(Part{row[2], row[3]});
        }
    }
}

void loadLocationData(std::vector<Part> &database, const std::string path = "part_to_location.csv") {
    std::vector<std::vector<std::string>> rawData;
    readCSV(rawData, path);
    for (auto row : rawData) {
        for (auto& part : database) {
            if (row[0] == part.number) {
                part.location = row[1];
                break;
            }
        }
    }
}


void writeLocation(std::vector<Part> parts, const char delimiter = ',', const std::string path = "part_to_location.csv") {
    std::ofstream csvFile(path);
    for (auto part : parts) {
        if (part.location.length() != 0) {
            csvFile << part.number << delimiter << part.location << std::endl;
        }
    }
    csvFile.close();
}

void search(std::string query, std::vector<Part*> &results, std::vector<Part> &database) {
    query = removeDublicateWhitespaces(query);
    std::string shape1 = extractShape(query);
    std::vector<std::string> queryKeywords;
    query = removeNonAlphaNum(query);
    query = removeDublicateWhitespaces(query);
    splitString(query, queryKeywords, ' ');
    for (Part& part : database) {
        std::string brickName = removeDublicateWhitespaces(part.queryName);
        std::vector<std::string> brickKeywords;
        
        splitString(brickName, brickKeywords, ' ');
        bool wasFound1 = true;
        for (auto queryKeyword : queryKeywords) {
            bool wasFound2 = false;
            for (auto brickKeyword : brickKeywords) {
                if (brickKeyword == queryKeyword) {
                    wasFound2 = true;
                    break;
                }
            }
            if (!wasFound2) {
                wasFound1 = false;
                break;
            }
        }
        bool shapesAreEqual = (shape1.length() != 0 && part.shape.length() != 0 && part.shape.length() >= shape1.length() && part.shape.substr(0, shape1.length()) == shape1) || shape1.length() == 0;
        if ((shapesAreEqual && wasFound1) || part.number.find(query) != std::string::npos) {
            results.push_back(&part);
        }
    }
    std::sort(results.begin(), results.end(), partComparison);
}

int main(const int argc, const char* argv[]) {
    std::string storagePrefix;
    if (argc > 1) {
        for (int i = 0; i < argc; i++) {
            if (strcmp(argv[i], "-s") == 0) {
                if (argc >= i + 1) {
                    storagePrefix = argv[i + 1];
                    break;
                }
                else {
                    std::cerr << "-s requires storage prefix as argument!" << std::endl;
                    exit(-1);
                }
            }
        }
    }

    constexpr int maxPageSize = 25;
    std::vector<Part> database;

    loadBricklinkData(database);
    if (database.size() == 0) {
        std::cerr << "No parts loaded!" << std::endl;
        exit(-1);
    }
    loadLocationData(database);

    while (true) {
        std::cout << "Enter part name: ";
        std::string query;
        std::getline(std::cin, query);
        if (query == std::string{":q"}) {
            exit(EXIT_SUCCESS);
        }
        std::vector<Part*> searchResult;
        search(query, searchResult, database);

        if (searchResult.size() == 0) {
            std::cout << "\033[1A" << "\033[K" << "No result found! ";
            continue;
        }
        int maxPadding = 0;
        for (auto result : searchResult) {
            if (result->queryName.length() > maxPadding)
                maxPadding = result->queryName.length();
        }

        int selection = 0,
            pageIndex = 0,
            numPages = searchResult.size() / maxPageSize + (searchResult.size() % maxPageSize == 0 ? 0 : 1);
        bool selecting = true,
             escaped = false;
        std::cout << "\033[1A" << "\033[K";
        while (selecting) {
            for (int i = maxPageSize * pageIndex; i < searchResult.size() && i < maxPageSize * (pageIndex + 1); i++) {
                searchResult[i]->print(i == selection);
            }
            std::cout << "Page " << pageIndex + 1 << " of " << numPages << "\tnavigate with arrow-keys, select with enter" << std::endl;

            int keePress = getch(),
                pageChange = 0;
            if (keePress == 27 && getch() == 91) { // check escape sequence
                keePress = getch();
                if (keePress == 65 && selection > maxPageSize * pageIndex)
                    selection--; // up-arrow
                else if (keePress == 66 && selection < searchResult.size() - 1 && selection < maxPageSize * (pageIndex + 1) - 1)
                    selection++;                                       // down-arrow
                else if (keePress == 67 && pageIndex < numPages - 1) { // right-arrow
                    pageChange = 1;
                    if (pageIndex == numPages - 2)
                        selection = searchResult.size() - 1;
                    else
                        selection += maxPageSize;
                } else if (keePress == 68 && pageIndex > 0) { // left-arrow
                    pageChange = -1;
                    selection -= maxPageSize;
                }
            }
            else if (keePress == 10)
                selecting = false; // enter-key
            else if (keePress == 32) {
                std::string command = "firefox https://www.bricklink.com/v2/search.page?q=";
                command += searchResult[selection]->number;
                command += "#T=P";
                system(command.c_str());
            }
            else if (keePress == 27) {
                selecting = false;
                escaped = true;
            }
            for (int i = maxPageSize * pageIndex; i < searchResult.size() + 1 && i < maxPageSize * (pageIndex + 1) + 1; i++) {
                std::cout << "\033[1A" << "\033[K";
            }
            pageIndex += pageChange;
        }
        if (!escaped) {
            std::string location;
            std::cout << "Selected:\n";
            searchResult[selection]->print();
            std::cout << "Current location: " << searchResult[selection]->location << '\n';
            std::cout << "Enter storage location: " << storagePrefix;
            std::cout.flush();
            std::getline(std::cin, location);
            if (location == std::string{":q"}) {
                exit(EXIT_SUCCESS);
            }
            for (int i = 0; i < 4; i++) {
                std::cout << "\033[1A" << "\033[K";
            }
            if (location != std::string{":c"}) {
                searchResult[selection]->location = storagePrefix + location;
                writeLocation(database);
            }
        }
    }
    return EXIT_SUCCESS;
}


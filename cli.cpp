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
#include <pqxx/pqxx>
#include "db_config.h"

class Part {
    public:
    std::string number, name, location;
    void print(bool highlighted=false);
    Part(std::string number, std::string name, std::string location);
};

Part::Part(std::string number, std::string name, std::string location) : number{number}, name{name}, location{location} {}

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

void search(std::string query, std::vector<Part> &results) {
    query = removeDublicateWhitespaces(query);
    std::string queryShape = getShape(query, true);
    std::vector<std::string> queryKeywords;
    query = removeDublicateWhitespaces(query);
    splitString(query, queryKeywords, ' ');
    if (queryShape.length() > 0) {
        queryKeywords.push_back(queryShape);
    }
    pqxx::connection conn(CONN_STR);
    pqxx::work work(conn);
    std::string request = "SELECT part.number, part.name, COALESCE(owning.location, '') as location FROM part LEFT OUTER JOIN owning ON part.number = owning.number "
                          "WHERE part.name ILIKE ALL (ARRAY[";
    for (auto& queryKeyword : queryKeywords) {
        request.append("'%" + work.esc(queryKeyword) + "%',");
    }
    if (request.back() == ',') {
        request.pop_back();
    }
    request.append("]) OR part.number LIKE '%" + work.esc(query) + "%' ");
    request.append("ORDER BY part.name asc;");
    pqxx::result result = work.exec(request);
    work.commit();
    std::string nameCopy;
    for (const auto row : result) {
        nameCopy = row["name"].as<std::string>();
        if (queryShape.length() == 0 || getShape(nameCopy, false) == queryShape) {
            results.push_back(Part {row["number"].as<std::string>(), row["name"].as<std::string>(), row["location"].as<std::string>()});
        }
    }
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

    while (true) {
        std::cout << "Enter part name: ";
        std::string query;
        std::getline(std::cin, query);
        if (query == std::string{":q"}) {
            exit(EXIT_SUCCESS);
        }
        std::vector<Part> searchResult;
        search(query, searchResult);

        if (searchResult.size() == 0) {
            std::cout << "\033[1A" << "\033[K" << "No result found! ";
            continue;
        }
        int maxPadding = 0;
        for (auto result : searchResult) {
            if (result.name.length() > maxPadding)
                maxPadding = result.name.length();
        }

        int selection = 0,
            pageIndex = 0,
            numPages = searchResult.size() / maxPageSize + (searchResult.size() % maxPageSize == 0 ? 0 : 1);
        bool selecting = true,
             escaped = false;
        std::cout << "\033[1A" << "\033[K";
        while (selecting) {
            for (int i = maxPageSize * pageIndex; i < searchResult.size() && i < maxPageSize * (pageIndex + 1); i++) {
                searchResult[i].print(i == selection);
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
                command += searchResult[selection].number;
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
            searchResult[selection].print();
            std::cout << "Current location: " << searchResult[selection].location << '\n';
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
                pqxx::connection conn(CONN_STR);
                pqxx::work work(conn);
                if (location.length() != 0) {
                    work.exec_params(
                        "INSERT INTO owning (number, location) "
                        "VALUES ($1, $2) "
                        "ON CONFLICT (number) DO UPDATE "
                        "SET location = EXCLUDED.location;",
                        searchResult[selection].number, storagePrefix + location
                    );
                }
                else {
                    work.exec_params("DELETE FROM owning WHERE number = $1;", searchResult[selection].number);
                }
                work.commit();
            }
        }
    }
    return EXIT_SUCCESS;
}

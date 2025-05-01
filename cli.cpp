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

#define ANSI_CURSOR_UP_ONE_LINE "\033[1A"
#define ANSI_ERASE_TO_END_OF_LINE "\033[K"
#define ANSI_START_RED "\033[31m"
#define ANSI_STOP_RED "\033[0m"
#define UP_ARROW 65
#define DOWN_ARROW 66
#define RIGHT_ARROW 67
#define LEFT_ARROW 68
#define ENTER_KEY 10
#define SPACE_BAR 32

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
        std::cout << ANSI_START_RED;
    }
    std::string filler;
    if ((name.length() + number.length() + location.length() + 7) > terminalWidth.ws_col) {
        std::cout << name.substr(0, terminalWidth.ws_col - number.length() - location.length() - 7) << " [...] " << location << number;
    }
    else {
        std::cout << name << std::string(terminalWidth.ws_col - name.length() - number.length() - location.length(), ' ') << location << number;
    }
    if (highlighted) {
        std::cout << ANSI_STOP_RED;
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
    std::string request = "SELECT DISTINCT part.number, part.name, COALESCE(owning.location, '') as location "
                          "FROM (part LEFT OUTER JOIN owning ON part.number = owning.number) LEFT OUTER JOIN alternate_num ON part.number = alternate_num.number "
                          "WHERE "
                          "(part.name ILIKE ALL (ARRAY[";
    for (auto& queryKeyword : queryKeywords) {
        request.append("'%" + work.esc(queryKeyword) + "%',");
    }
    if (request.back() == ',') {
        request.pop_back();
    }
    request.append("]) "
                   "AND part.name NOT LIKE '%Modulex%' "
                   "AND part.name NOT LIKE '%Sticker Sheet for Set%' "
                   "AND part.name NOT LIKE '%(Sticker)%') "
                   "OR location LIKE '%" + work.esc(query) + "%' "
                   "OR part.number LIKE '%" + work.esc(query) + "%' "
                   "OR alternate_num.alt_num LIKE '%" + work.esc(query) + "%' ");
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

int selectEntry(std::vector<Part>& searchResult, int maxPageSize, bool& escaped) {
    int selection = 0,
        pageIndex = 0,
        numPages = searchResult.size() / maxPageSize + (searchResult.size() % maxPageSize == 0 ? 0 : 1);
    bool selecting = true;
    std::cout << ANSI_CURSOR_UP_ONE_LINE << ANSI_ERASE_TO_END_OF_LINE;
    while (selecting) {
        for (int i = maxPageSize * pageIndex; i < searchResult.size() && i < maxPageSize * (pageIndex + 1); i++) {
            searchResult[i].print(i == selection);
        }
        std::cout << "Page " << pageIndex + 1 << " of " << numPages << "\tnavigate with arrow-keys, select with enter" << std::endl;

        int keePress = getch(),
            pageChange = 0;
        if (keePress == 27 && getch() == 91) { // check escape sequence
            keePress = getch();
            if (keePress == UP_ARROW && selection > maxPageSize * pageIndex)
                selection--;
            else if (keePress == DOWN_ARROW && selection < searchResult.size() - 1 && selection < maxPageSize * (pageIndex + 1) - 1)
                selection++;
            else if (keePress == RIGHT_ARROW && pageIndex < numPages - 1) {
                pageChange = 1;
                if (pageIndex == numPages - 2)
                    selection = searchResult.size() - 1;
                else
                    selection += maxPageSize;
            } else if (keePress == LEFT_ARROW && pageIndex > 0) {
                pageChange = -1;
                selection -= maxPageSize;
            }
        }
        else if (keePress == ENTER_KEY)
            selecting = false;
        else if (keePress == SPACE_BAR) {
            std::string command = "firefox https://www.bricklink.com/v2/search.page?q=";
            command += searchResult[selection].number;
            command += "#T=P";
            system(command.c_str());
        }
        else if (keePress == 27 /*esc*/) {
            selecting = false;
            escaped = true;
        }
        for (int i = maxPageSize * pageIndex; i < searchResult.size() + 1 && i < maxPageSize * (pageIndex + 1) + 1; i++) {
            std::cout << ANSI_CURSOR_UP_ONE_LINE << ANSI_ERASE_TO_END_OF_LINE;
        }
        pageIndex += pageChange;
    }
    return selection;
}

int main(const int argc, const char* argv[]) {
    constexpr char cancel[] = ":c";
    constexpr char quit[] = ":q";

    std::string storagePrefix;
    if (argc > 1) {
        if (strcmp(argv[1], "--info") || strcmp(argv[1], "-i")) {
            int maxMar = 0, maxLyt = 0;

            pqxx::connection conn(CONN_STR);
            pqxx::work work(conn);
            std::string request = "SELECT location FROM owning;";
            pqxx::result result = work.exec(request);
            work.commit();
            for (const auto row : result) {
                std::string location = row["location"].as<std::string>();
                std::string storageType = location.substr(0, 3);
                if (storageType.compare("mar") == 0) {
                    std::string details = location.substr(3);
                    std::string number;
                    if (details.find('c') == std::string::npos) {
                        number = details;
                    }
                    else {
                        number = details.substr(0, details.find('c'));
                    }
                    int num = std::stoi(number);
                
                    if (num > maxMar) {
                        maxMar = num;
                    }
                }
                else if (storageType.compare("lyt") == 0) {
                    std::string details = location.substr(3);
                    std::string number;
                    if (details.find('c') == std::string::npos) {
                        number = details;
                    }
                    else {
                        number = details.substr(0, details.find('c'));
                    }
                    int num = std::stoi(number);
                
                    if (num > maxLyt) {
                        maxLyt = num;
                    }
                }
            }

            std::cout << "max mar: " << maxMar << std::endl;
            std::cout << "max lyt: " << maxLyt << std::endl;
            exit(0);
        }

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
        if (query == quit) {
            exit(EXIT_SUCCESS);
        }
        std::vector<Part> searchResult;
        search(query, searchResult);

        if (searchResult.size() == 0) {
            std::cout << ANSI_CURSOR_UP_ONE_LINE << ANSI_ERASE_TO_END_OF_LINE << "No result found! ";
            continue;
        }
        int maxPadding = 0;
        for (auto result : searchResult) {
            if (result.name.length() > maxPadding)
                maxPadding = result.name.length();
        }

        bool escaped = false;
        int selection = selectEntry(searchResult, maxPageSize, escaped);
        if (escaped) {
            continue;
        }

        std::string location;
        std::cout << "Selected:\n";
        searchResult[selection].print();
        std::cout << "Current location: " << searchResult[selection].location << '\n';
        std::cout << "Enter storage location: " << storagePrefix;
        std::cout.flush();
        std::getline(std::cin, location);
        if (location == quit) {
            exit(EXIT_SUCCESS);
        }
        for (int i = 0; i < 4; i++) {
            std::cout << ANSI_CURSOR_UP_ONE_LINE << ANSI_ERASE_TO_END_OF_LINE;
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
    return EXIT_SUCCESS;
}

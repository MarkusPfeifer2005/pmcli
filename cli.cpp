#include <execution>
#include <filesystem>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <ios>
#include <iostream>
#include <ostream>
#include <stdexcept>
#include <string>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <vector>
#include <set>
#include "str_lib.h"
#include "csv.h"
#include <pqxx/pqxx>
#include "libs/pugixml-1.15/src/pugixml.hpp"
#include <termios.h>
#include <unistd.h>
#include <stdio.h>

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

// Keybindings
#define QUIT ":q"
#define CANCEL ":c"

int getch() {
    struct termios oldt, newt;
    int ch;
    tcgetattr(STDIN_FILENO, &oldt);              // save old settings
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);            // disable canonical mode & echo
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);     // apply new settings
    ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);     // restore old settings
    return ch;
}

std::string connect_db() {
    std::string loginString;
    if (std::filesystem::exists("login.txt")) {
        std::ifstream loginFile("login.txt");
        getline(loginFile, loginString);
        loginFile.close();
    }
    else {
        std::string databaseName, userName, password, host, port;
        std::cout << "Database name: ";
        std::cin >> databaseName;
        std::cout << "Username: ";
        std::cin >> userName;
        std::cout << "Password: ";
        std::cin >> password;
        std::cout << "IP-Address: ";
        std::cin >> host;
        std::cout << "Port: ";
        std::cin >> port;

        for (int i = 0; i < 5; i++) {
            std::cout << ANSI_CURSOR_UP_ONE_LINE << ANSI_ERASE_TO_END_OF_LINE;
        }
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        loginString = "dbname=" + databaseName + " user=" + userName + " password=" + password + " host=" + host + " port=" + port;

        std::ofstream loginFile("login.txt");
        loginFile << loginString;
        loginFile.close();

    }
    return loginString;
}


std::string hexToStr(const std::string& hex) {
    // expect "RRGGBB" (6 chars, no '#')
    int r = std::stoi(hex.substr(0, 2), nullptr, 16);
    int g = std::stoi(hex.substr(2, 2), nullptr, 16);
    int b = std::stoi(hex.substr(4, 2), nullptr, 16);

    return "\033[48;2;" + std::to_string(r) + ";" +
           std::to_string(g) + ";" +
           std::to_string(b) + "m  \033[0m";
}

class Color {
    public:
        std::string name,
                    rgb;
        int id;
        Color(int id, std::string name, std::string rgb, int quantity, std::string partID);
        int getQuantity() const;
        void setQuantity(int quantity);
        void print(bool highlighted=false);
        void openBricklink();
    private:
        int quantity;
        std::string partID;
        static unsigned int maxNameLength,
                            maxQtyLength;
};

unsigned int Color::maxNameLength = 0;
unsigned int Color::maxQtyLength = 0;

Color::Color(int id, std::string name, std::string rgb, int quantity, std::string partID) : id(id), name(name), rgb(rgb), quantity(quantity), partID(partID) {
    if (this->name.length() > Color::maxNameLength) {
        Color::maxNameLength = this->name.length();
    }
    if (std::to_string(this->quantity).length() > Color::maxQtyLength) {
        Color::maxQtyLength = std::to_string(this->quantity).length();
    }
};

int Color::getQuantity() const {return this->quantity;}

void Color::setQuantity(int quantity) {
    std::string connStr = connect_db();
    pqxx::connection conn(connStr);
    pqxx::work work(conn);
    if (quantity == 0) {
        work.exec("DELETE FROM stock WHERE number=$1 AND color=$2;", pqxx::params{this->partID, this->id});
    }
    else {
        work.exec(
                "INSERT INTO stock (number, color, quantity) "
                "VALUES($1, $2, $3) "
                "ON CONFLICT (number, color) DO UPDATE "
                "SET quantity = EXCLUDED.quantity;",
                pqxx::params{this->partID, this->id, quantity}
        );
    }
    this->quantity = quantity;
    if (std::to_string(this->quantity).length() > Color::maxQtyLength) {
        Color::maxQtyLength = std::to_string(this->quantity).length();
    }
    work.commit();
}

void Color::print(bool highlighted) {
    std::cout << hexToStr(this->rgb) << " ";
    if (highlighted) {std::cout << ANSI_START_RED;}
    std::cout << this->name;
    if (highlighted) {std::cout << ANSI_STOP_RED;}
    for (int i = 0; i < Color::maxNameLength - this->name.length(); i++) {std::cout << ' ';}
    std::cout << " | ";
    for (int i = 0; i < maxQtyLength - std::to_string(this->quantity).length(); i++) {std::cout << ' ';}
    std::cout << this->quantity << std::endl;
}

void Color::openBricklink() {
    std::string command = "firefox 'https://www.bricklink.com/catalogList.asp?v=2&colorID='";
    command += std::to_string(this->id);
    system(command.c_str());
}

class Part {
    public:
        std::string number,
                    name;
        void print(bool highlighted=false);
        Part(std::string number, std::string name, std::string location);
        std::string getLocation();
        void setLocation(std::string location, std::string storagePrefix);
        void openBricklink();
        void getAvailableColors(std::vector<Color>& colors);
    private:
        std::string location;
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
        std::cout << name << std::string(terminalWidth.ws_col - name.length() - number.length() - location.length(), ' ') << location 
            << number;
    }
    if (highlighted) {
        std::cout << ANSI_STOP_RED;
    }
    std::cout << std::endl;
}

void Part::openBricklink() {
    std::string command = "firefox https://www.bricklink.com/v2/search.page?q=";
    command += this->number;
    command += "#T=P";
    system(command.c_str());
}

std::string Part::getLocation() {return this->location;}

void Part::setLocation(std::string location, std::string storagePrefix) {
    std::string connStr = connect_db();
    pqxx::connection conn(connStr);
    pqxx::work work(conn);
    if (location != std::string{CANCEL}) {
        if (location.length() != 0) {
            work.exec(
                    "INSERT INTO owning (number, location) "
                    "VALUES ($1, $2) "
                    "ON CONFLICT (number) DO UPDATE "
                    "SET location = EXCLUDED.location;",
                    pqxx::params{this->number, storagePrefix + location}
            );
        }
        else {
            work.exec("DELETE FROM owning WHERE number = $1;", pqxx::params{this->number});
        }
        work.commit();
        this->location = location;
    }
} 

void Part::getAvailableColors(std::vector<Color>& colors) {
    std::string connStr = connect_db();
    pqxx::connection conn(connStr);
    pqxx::work work(conn);
    std::string request = 
        "SELECT id, name, rgb, COALESCE(quantity, 0) as quantity "
        "FROM "
            "(SELECT id, name, rgb "
            "FROM available_colors Left OUTER JOIN color "
            "ON available_colors.color_id=color.id "
            "WHERE part_number='" + work.esc(this->number) + "') col "

            "LEFT OUTER JOIN "

            "(SELECT color, quantity "
            "FROM stock "
            "WHERE number='" + work.esc(this->number) + "') stk "

            "ON col.id=stk.color "
        ";";
    pqxx::result result = work.exec(request);
    work.commit();
    std::string nameCopy;
    for (const auto row : result) {
        colors.push_back(Color {
                row["id"].as<int>(),
                row["name"].as<std::string>(),
                row["rgb"].as<std::string>(),
                row["quantity"].as<int>(),
                this->number
        });
    }
}

void editLocation(Part part, std::string storagePrefix) {
    std::cout << "Current location: " << part.getLocation() << '\n';
    std::cout << "Enter new location: ";
    std::string location;
    std::getline(std::cin, location);
    if (location == QUIT) {
        exit(EXIT_SUCCESS);
    }
    for (int i = 0; i < 2; i++) {
        std::cout << ANSI_CURSOR_UP_ONE_LINE << ANSI_ERASE_TO_END_OF_LINE;
    }
    part.setLocation(location, storagePrefix);
}

void search(std::string query, std::vector<Part> &results) {
    std::string connStr = connect_db();
    pqxx::connection conn(connStr);
    pqxx::work work(conn);
    /*fill the results vector with results*/
    query = removeDublicateWhitespaces(query);
    std::string queryShape = getShape(query, true);
    std::vector<std::string> queryKeywords;
    query = removeDublicateWhitespaces(query);
    splitString(query, queryKeywords, ' ');
    if (queryShape.length() > 0) {
        queryKeywords.push_back(queryShape);
    }
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

template<typename Item>
int selectEntry(std::vector<Item>& searchResult, int maxPageSize, bool& escaped) {
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
        else if (keePress == ENTER_KEY) selecting = false;
        else if (keePress == SPACE_BAR) searchResult[selection].openBricklink();
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


void getInfo() {
    std::string connStr = connect_db();
    pqxx::connection conn(connStr);
    pqxx::work work(conn);
    int maxMar = 0, maxLyt = 0;
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
    }

    std::string getPartName() {
    std::cout << "Enter part name: ";
    std::string query;
    std::getline(std::cin, query);
    if (query == QUIT) {
        exit(EXIT_SUCCESS);
    }
    return query;
}

void editColor(Part part) {
    std::vector<Color> colors;
    part.getAvailableColors(colors);
    std::cout << "Part " << part.name << std::endl;
    bool escaped = false;
    while (true) {
        int colorSelection = selectEntry(colors, 25, escaped);
        if (escaped) {
            return;
        }
        std::cout << "Color " << colors[colorSelection].name << std::endl;
        std::cout << "num parts: " << colors[colorSelection].getQuantity() << std::endl;
        std::cout << "Enter new quantity: ";
        std::string input;
        std::getline(std::cin, input);
        int newQuantity;
        try {
            size_t pos;
            newQuantity = std::stoi(input, &pos);
        }
        catch (const std::invalid_argument& e) {
            std::cout << "Invalid input; aborting! Press any key to continue;" << std::endl;
            getch();
            for (int i = 0; i < 4; i++) {
                std::cout << ANSI_CURSOR_UP_ONE_LINE << ANSI_ERASE_TO_END_OF_LINE;
            }
            return;
        }
        colors[colorSelection].setQuantity(newQuantity);
        for (int i = 0; i < 2; i++) {
            std::cout << ANSI_CURSOR_UP_ONE_LINE << ANSI_ERASE_TO_END_OF_LINE;
        }
    }
    std::cout << ANSI_CURSOR_UP_ONE_LINE << ANSI_ERASE_TO_END_OF_LINE;
}

void updateParts(std::string path) {
    std::string connStr = connect_db();
    pqxx::connection conn(connStr);
    pqxx::work work(conn); 

    CSV csv(path, '\t');
    std::vector<std::string> words, altNums;
    size_t numberIdx = csv.getIndexOf("Number"),
           nameIdx = csv.getIndexOf("Name"),
           altIdx = csv.getIndexOf("Alternate Item Number"),
           categoryNameIdx = csv.getIndexOf("Category Name");
    std::set<std::string> altNumsSet;

    while (csv.getRow(words)) {
        if (words[categoryNameIdx].find("Decorated") != std::string::npos) {
            continue;
        }
        work.exec("INSERT INTO part VALUES($1, $2) ON CONFLICT DO NOTHING;",
                pqxx::params{words[numberIdx], words[nameIdx]});
        if (!words[altIdx].empty()) {
            altNums.clear();
            altNumsSet.clear();
            splitString(words[altIdx], altNums, ',');
            for (std::string s : altNums) {
                altNumsSet.insert(s);
            }
            for (std::string s : altNumsSet) {
                work.exec("INSERT INTO alternate_num VALUES($1, $2) ON CONFLICT "
                        "DO NOTHING;", pqxx::params{words[numberIdx], s});
            }
        }
        work.commit();
    }
}

void updateColors(std::string path) {
    std::string connStr = connect_db();
    pqxx::connection conn(connStr);
    pqxx::work work(conn); 

    CSV csv(path, '\t');
    std::vector<std::string> words;
    size_t idIdx = csv.getIndexOf("Color ID"),
           nameIdx = csv.getIndexOf("Color Name"),
           rgbIdx = csv.getIndexOf("RGB");

    while (csv.getRow(words)) {
        work.exec("INSERT INTO color VALUES($1, $2, $3) ON CONFLICT DO NOTHING;",
                pqxx::params{words[idIdx], words[nameIdx], words[rgbIdx]});
        work.commit();
    }
}

void updateAvailableColors(std::string path) {
    std::string connStr = connect_db();
    pqxx::connection conn(connStr);
    pqxx::work work(conn); 

    CSV csv(path, '\t');
    std::vector<std::string> words;
    size_t partNumIdx = csv.getIndexOf("Item No"),
           colorNameIdx = csv.getIndexOf("Color");

    while (csv.getRow(words)) {
        work.exec(
            "INSERT INTO available_colors (part_number, color_id) "
            "SELECT $1, color.id FROM color "
            "WHERE color.name=$2 AND EXISTS (SELECT 1 FROM part WHERE number=$1) "
            "ON CONFLICT DO NOTHING;",
            pqxx::params{words[partNumIdx], words[colorNameIdx]}
        );
        work.commit();
    }
}

void annotateXML(std::string path) {
    pugi::xml_document partList;
    pugi::xml_parse_result result = partList.load_file(path.c_str());
    if (!result) {
        throw std::ios_base::failure("Failed to load " + path);
    }
    std::string connStr = connect_db();
    pqxx::connection conn(connStr);
    pqxx::work work(conn); 
    std::vector<Part> parts;
    for (pugi::xml_node &item : partList.child("INVENTORY").children()) {
        pqxx::result res = work.exec("SELECT number, location FROM owning WHERE number=$1;",
                pqxx::params{item.child("ITEMID").text().as_string()});
        work.commit();
        if (!res.empty()) {
            pugi::xml_node remarks = item.append_child("REMARKS");
            remarks.text() = res[0]["location"].as<std::string>();
        }
    }
    partList.save_file(path.insert(path.find_last_of("."), "_annotated").c_str());
}

int main(const int argc, const char* argv[]) {
    // handle command line arguments
    std::string storagePrefix;
    if (argc > 1) {
        if ((strcmp(argv[1], "--info") == 0) || (strcmp(argv[1], "-i") == 0)) {
            getInfo();
            exit(0);
        }

        // get catalog here: https://www.bricklink.com/catalogDownload.asp
        if ((strcmp(argv[1], "--update") == 0) || (strcmp(argv[1], "-u") == 0)) {
            if (argc < 4) {
                std::cerr << "Too few arguments provided!\n"
                             "-u requires one of [parts, colors, codes] and a path!" << std::endl;
                exit(-1);
            }
            else if (argc > 4) {
                std::cerr << "Too many arguments provided!\n"
                             "-u requires one of [parts, colors, codes] and a path!" << std::endl;
                exit(-1);
            }
            if (strcmp(argv[2], "parts") == 0) {
                std::cout << "Updating parts..." << std::endl;
                updateParts(argv[3]);
                std::cout << "Updated parts successfully." << std::endl;
                exit(0);
            }
            else if (strcmp(argv[2], "colors") == 0) {
                std::cout << "Updating colors..." << std::endl;
                updateColors(argv[3]);
                std::cout << "Updated colors successfully." << std::endl;
                exit(0);
            }
            else if (strcmp(argv[2], "codes") == 0) {
                std::cout << "Updating codes..." << std::endl;
                updateAvailableColors(argv[3]);
                std::cout << "Updated codes successfully." << std::endl;
                exit(0);
            }
            else {
                std::cerr << "Wrong argument supplied!\n"
                             "It has to be one of [parts, colors, codes].";
                exit(-1);
            }
        }

        if ((strcmp(argv[1], "--annotate") == 0) || (strcmp(argv[1], "-a") == 0)) {
            if (argc > 3) {
                std::cerr << "Too many arguments supplied!\n"
                             "'annotate' requires only a path to an XML-file." << std::endl;
                exit(-1);
            }
            else if (argc < 3) {
                std::cerr << "Too few arguments supplied!\n"
                    "'annotate' requires a path to an XML-file." << std::endl;
                exit(-1);
            }
            std::cout << "Annotating partlist..." << std::endl;
            annotateXML(argv[2]);
            std::cout << "Annotated partlist." << std::endl;
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

    // main program loop
    while (true) {
        std::string query = getPartName();
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
        int selection = selectEntry(searchResult, 25, escaped);
        if (escaped) {continue;}

        std::cout << "Selected: " << searchResult[selection].name << ", " 
            << searchResult[selection].number << std::endl;
        if (searchResult[selection].getLocation().length() == 0) {
            editLocation(searchResult[selection], storagePrefix);
        }
        else {
            std::cout << "Edit color or location (c/l)? ";
            std::string answer;
            std::getline(std::cin, answer);
            std::cout << ANSI_CURSOR_UP_ONE_LINE << ANSI_ERASE_TO_END_OF_LINE;
            if (answer == "c") {
                editColor(searchResult[selection]);
            }
            else if (answer == "l") {
                editLocation(searchResult[selection], storagePrefix);
            }
        }
        std::cout << ANSI_CURSOR_UP_ONE_LINE << ANSI_ERASE_TO_END_OF_LINE;
    }
    return EXIT_SUCCESS;
}

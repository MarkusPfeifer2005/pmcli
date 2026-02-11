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
#include "item.h"
#include <pqxx/pqxx>
#include "libs/pugixml-1.15/src/pugixml.hpp"
#include <termios.h>
#include <unistd.h>
#include <stdio.h>
#include <ncurses.h>

#define ANSI_CURSOR_UP_ONE_LINE "\033[1A"
#define ANSI_ERASE_TO_END_OF_LINE "\033[K"

#define PART_NUM_LENGTH 20

bool getUserInput(std::string& out) {
    noecho();

    out.clear();
    int ch;
    while (true) {
        ch = getch();
        switch (ch) {
            case 27:
                echo();
                return false;
            case '\n':
            case KEY_ENTER:
                echo();
                return true;
            case KEY_BACKSPACE:
            case 127:
            case 8:
                if (!out.empty()) {
                    out.pop_back();
                    int x, y;
                    getyx(stdscr, y, x);
                    mvaddch(y, x - 1, ' ');
                    move(y, x - 1);
                }
                break;
            default:
                if (isprint(ch)) {
                    out.push_back(static_cast<char>(ch));
                    addch(ch);
                }
                break;
        }
        refresh();
    }
}

std::string connect_db() {
    const char* homeDir = std::getenv("HOME");
    if (!homeDir) {
        std::cerr << "HOME environment variable not set!" << std::endl;
        exit(EXIT_FAILURE);
    }
    std::string dirPath = std::string(homeDir) + "/.pmcli",
                fileName = dirPath + "/login.txt";
    std::string loginString, password;
    if (std::filesystem::exists(fileName)) {
        std::ifstream loginFile(fileName);
        getline(loginFile, loginString);
        loginFile.close();
    }
    else {
        std::string databaseName, userName, host, port;
        std::cout << "Database name: ";
        std::cin >> databaseName;
        std::cout << "Username: ";
        std::cin >> userName;
        password = getpass("Password: ");
        char savePassword;
        std::cout << "Do you want to save the password? [Y/n] ";
        std::cin >> savePassword;
        std::cout << "IP-Address: ";
        std::cin >> host;
        std::cout << "Port: ";
        std::cin >> port;

        for (int i = 0; i < 6; i++) {
            std::cout << ANSI_CURSOR_UP_ONE_LINE << ANSI_ERASE_TO_END_OF_LINE;
        }
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        loginString = "dbname=" + databaseName + " user=" + userName + " host=" + host + " port=" + port;
        if (savePassword == 'y' || savePassword == 'Y') {
            loginString += " password=" + password;
        }
        if (!std::filesystem::exists(dirPath)) {
            std::filesystem::create_directory(dirPath);
        }
        std::ofstream loginFile(fileName);
        loginFile << loginString;
        loginFile.close();
    }
    if (loginString.find("password") != std::string::npos) {
        return loginString;
    }
    else if (password.empty()) {
        password = getpass("Password: ");
        std::cout << ANSI_CURSOR_UP_ONE_LINE << ANSI_ERASE_TO_END_OF_LINE;
    }
    return loginString += " password=" + password;
}

void editLocation(Part part, std::string storagePrefix, pqxx::connection& conn) {
    printw("Current location: %s\n", part.getLocation().c_str());
    printw("Enter new location: ");
    std::string location;
    if (!getUserInput(location)) {
        clear();
        return;
    }
    clear();
    part.setLocation(location, storagePrefix, conn);
}

void search(std::string query, std::vector<Part> &results, pqxx::connection& conn) {
    pqxx::work work(conn); 
    //fill the results vector with results
    query = removeDublicateWhitespaces(query);
    std::string queryShape = getShape(query, true);
    std::vector<std::string> queryKeywords;
    query = removeDublicateWhitespaces(query);
    splitString(query, queryKeywords, ' ');
    if (queryShape.length() > 0) {
        queryKeywords.push_back(queryShape);
    }
    std::string request = "SELECT DISTINCT part.number, part.name, COALESCE(owning.location, '') as location "
        "FROM (part LEFT OUTER JOIN owning ON part.number = owning.number) "
        "LEFT OUTER JOIN alternate_num ON part.number = alternate_num.number "
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
int selectEntry(std::vector<Item>& searchResult, bool& escaped) {
    int rows, cols;
    getmaxyx(stdscr, rows, cols);
    int maxPageSize = (MAX_DISPLAY < rows) ? MAX_DISPLAY : rows - 1;  // 1 row for page number

    noecho();
    curs_set(0);

    int selection = 0,
        pageIndex = 0,
        numPages = searchResult.size() / maxPageSize + (searchResult.size() % maxPageSize == 0 ? 0 : 1);
    bool selecting = true;
    while (selecting) {
        erase();
        move(0, 0);
        for (int i = maxPageSize * pageIndex; i < searchResult.size() && i < maxPageSize * (pageIndex + 1); i++) {
            searchResult[i].print(i == selection);
        }
        printw("Page %d of %d\t navigate with arrow-keys, select with enter", pageIndex + 1, numPages);
        refresh();

        int pageChange = 0;
        switch (getch()) {
            case 'k':
            case KEY_UP:
                if (selection > maxPageSize * pageIndex) {
                   selection--; 
                }
                break;
            case 'j':
            case KEY_DOWN:
                if (selection < searchResult.size() - 1 && selection < maxPageSize * (pageIndex + 1) - 1) {
                    selection++;
                }
                break;
            case 'l':
            case KEY_RIGHT:
                if (pageIndex < numPages - 1) {
                    pageChange = 1;
                    if (pageIndex == numPages - 2)
                        selection = searchResult.size() - 1;
                    else
                        selection += maxPageSize;
                }
                break;
            case 'h':
            case KEY_LEFT:
                if (pageIndex > 0) {
                    pageChange = -1;
                    selection -= maxPageSize;
                }
                break;
            case '\n':
            case KEY_ENTER:
                selecting = false;
                break;
            case ' ':
                searchResult[selection].openBricklink();
                break;
            case 27:
                selecting = false;
                escaped = true;
                break;
        }
        pageIndex += pageChange;
    }
    clear();
    echo();
    curs_set(1);
    return selection;
}

void getInfo(pqxx::connection& conn) {
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

void editColor(Part part, pqxx::connection& conn) {
    std::vector<Color> colors;
    part.getAvailableColors(colors, conn);
    printw("Part %s\n", part.name.c_str());
    bool escaped = false;
    while (true) {
        int colorSelection = selectEntry(colors, escaped);
        if (escaped) {
            return;
        }
        printw("Color %s\n", colors[colorSelection].name.c_str());
        printw("num parts: %i\n", colors[colorSelection].getQuantity());
        printw("Enter new quantity: ");
        std::string input;
        if (!getUserInput(input)) {
            continue;
        }
        int newQuantity;
        try {
            size_t pos;
            newQuantity = std::stoi(input, &pos);
        }
        catch (const std::invalid_argument& e) {
            printw("Invalid input; aborting! Press any key to continue;\n");
            getch();
            move(getcury(stdscr) - 4, 0); // Move cursor up one line
            clrtoeol(); // Erase to end of line
            refresh(); // Update the screen
            return;
        }
        colors[colorSelection].setQuantity(newQuantity);
        move(getcury(stdscr) - 2, 0); // Move cursor up one line
        clrtoeol(); // Erase to end of line
        refresh(); // Update the screen
    }
    move(getcury(stdscr) - 1, 0); // Move cursor up one line
    clrtoeol(); // Erase to end of line
    refresh(); // Update the screen
}

void updateParts(std::string path, pqxx::connection& conn) {
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

void updateColors(std::string path, pqxx::connection& conn) {
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

void updateAvailableColors(std::string path, pqxx::connection& conn) {
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

void annotateXML(std::string path, pqxx::connection& conn) {
    pqxx::work work(conn);
    pugi::xml_document partList;
    pugi::xml_parse_result result = partList.load_file(path.c_str());
    if (!result) {
        throw std::ios_base::failure("Failed to load " + path);
    }
    for (pugi::xml_node &item : partList.child("INVENTORY").children()) {
        pqxx::result res = work.exec("SELECT number, location FROM owning WHERE number=$1;",
                pqxx::params{item.child("ITEMID").text().as_string()});
        work.commit();
        if (res.empty()) {
            continue;
        }
        std::string info = res[0]["location"].as<std::string>();
        res = work.exec("SELECT * from stock WHERE number=$1;",
                pqxx::params{item.child("ITEMID").text().as_string()});
        work.commit();
        if (res.empty()) {
            info.append(" empty");
        }
        else {
            info.append(" filled");
        }
        pugi::xml_node remarks = item.append_child("REMARKS");
        remarks.text() = info;
    }
    partList.save_file(path.insert(path.find_last_of("."), "_annotated").c_str());
}

void validateXml(std::string path, std::vector<Item>& extParts, std::vector<Item>& intParts, pqxx::connection& conn) {
    pqxx::work work(conn);
    pugi::xml_document partList;
    pugi::xml_parse_result result = partList.load_file(path.c_str());
    if (!result) {
        throw std::ios_base::failure("Failed to load " + path);
    }
    for (pugi::xml_node item : partList.child("INVENTORY").children()) {
        std::string pID = item.child("ITEMID").text().as_string();
        if (pID.length() < PART_NUM_LENGTH) {
            pID.append(PART_NUM_LENGTH - pID.length(), ' ');
        }
        Item listPart(
                pID,
                item.child("COLOR").text().as_uint(),
                item.child("MINQTY").text().as_uint(),
                conn
                );
        pqxx::result result = work.exec(
                "SELECT * FROM stock "
                "WHERE number=$1 AND color=$2",
                pqxx::params{listPart.partID, listPart.colorID}
                );
        work.commit();
        unsigned int cID,
                     quantity;
        if (result.empty()) {
            pqxx::result result2 = work.exec(
                    "SELECT * FROM available_colors WHERE "
                    "part_number=$1 AND color_id=$2;",
                    pqxx::params{listPart.partID, listPart.colorID}
                    );
            work.commit();
            if (result2.empty()) {
                std::string message = "That part - color combination does not exist!\n"
                                      "part ID:  " + pID + "\n"
                                      "color ID: " + std::to_string(listPart.colorID) + "\n";
                throw std::out_of_range(message);
            }
            cID = item.child("COLOR").text().as_uint();
            quantity = 0;
        }
        else {
            cID = result[0]["color"].as<unsigned int>();
            quantity = result[0]["quantity"].as<unsigned int>();
        }
        Item dbPart(pID, cID, quantity, conn);
        extParts.push_back(listPart);
        intParts.push_back(dbPart);
    }
}

void extractParts(std::string path, pqxx::connection& conn) {
    std::vector<Item>listParts, dbParts;
    validateXml(path, listParts, dbParts, conn);
    for (auto lp = listParts.begin(), dp = dbParts.begin(); lp != listParts.end(); lp++, dp++) {
        if (dp->getQuantity() < lp->getQuantity()) {
            throw std::domain_error("There have to be more parts in stock than can be extracted!");
        }
    }
    for (auto lp = listParts.begin(), dp = dbParts.begin(); lp != listParts.end(); lp++, dp++) {
        *dp - *lp;
    }
}

void returnParts(std::string path, pqxx::connection& conn) {
    std::vector<Item>listParts, dbParts;
    validateXml(path, listParts, dbParts, conn);
    pqxx::work work(conn);
    for (auto lp = listParts.begin(), dp = dbParts.begin(); lp != listParts.end(); lp++, dp++) {
        pqxx::result result = work.exec("SELECT * from owning WHERE number=$1;",
                                        pqxx::params{lp->partID});
        work.commit();
        if (result.empty()) {
            std::string message = "There have to be parts with location in the inventory!\n"
                                  "part ID: '" + lp->partID + "'\n"; 
            throw std::domain_error(message);
        }
    }
    for (auto lp = listParts.begin(), dp = dbParts.begin(); lp != listParts.end(); lp++, dp++) {
        *dp + *lp;
    }
}

void getMissing(std::string path, pqxx::connection& conn) {
    std::vector<Item>listParts, dbParts;
    validateXml(path, listParts, dbParts, conn);
    pugi::xml_document missingList;
    pugi::xml_node inventory = missingList.append_child("INVENTORY");
    for (auto lp = listParts.begin(), dp = dbParts.begin(); lp != listParts.end(); lp++, dp++) {
        if (lp->getQuantity() <= dp->getQuantity()) {
            continue;
        }
        pugi::xml_node item = inventory.append_child("ITEM");
        item.append_child("ITEMTYPE").text() = "P";
        std::string pID = lp->partID;
        size_t endpos = pID.find_first_of(' ');
        pID = pID.erase(endpos);
        item.append_child("ITEMID").text() = pID;
        item.append_child("COLOR").text() = lp->colorID;
        item.append_child("MINQTY").text() = lp->getQuantity() - dp->getQuantity();
    }
    missingList.save_file(path.insert(path.find_last_of("."), "_missing").c_str());
}

int main(const int argc, const char* argv[]) {
    std::string connStr = connect_db();
    pqxx::connection conn(connStr);

    // handle command line arguments
    std::string storagePrefix;
    if (argc > 1) {
        if ((strcmp(argv[1], "--info") == 0) || (strcmp(argv[1], "-i") == 0)) {
            getInfo(conn);
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
                updateParts(argv[3], conn);
                std::cout << "Updated parts successfully." << std::endl;
                exit(0);
            }
            else if (strcmp(argv[2], "colors") == 0) {
                std::cout << "Updating colors..." << std::endl;
                updateColors(argv[3], conn);
                std::cout << "Updated colors successfully." << std::endl;
                exit(0);
            }
            else if (strcmp(argv[2], "codes") == 0) {
                std::cout << "Updating codes..." << std::endl;
                updateAvailableColors(argv[3], conn);
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
            annotateXML(argv[2], conn);
            std::cout << "Annotated partlist." << std::endl;
            exit(0);
        }

        if ((strcmp(argv[1], "--extract") == 0) || (strcmp(argv[1], "-e") == 0)) {
            if (argc > 3) {
                std::cerr << "Too many arguments supplied!\n"
                             "'extract' requires only a path to an XML-file." << std::endl;
                exit(-1);
            }
            else if (argc < 3) {
                std::cerr << "Too few arguments supplied!\n"
                    "'extract' requires a path to an XML-file." << std::endl;
                exit(-1);
            }
            std::cout << "Extracting parts..." << std::endl;
            extractParts(argv[2], conn);
            std::cout << "Extracted parts." << std::endl;
            exit(0);
        }

        if ((strcmp(argv[1], "--return") == 0) || (strcmp(argv[1], "-r") == 0)) {
            if (argc > 3) {
                std::cerr << "Too many arguments supplied!\n"
                             "'return' requires only a path to an XML-file." << std::endl;
                exit(-1);
            }
            else if (argc < 3) {
                std::cerr << "Too few arguments supplied!\n"
                    "'return' requires a path to an XML-file." << std::endl;
                exit(-1);
            }
            std::cout << "Returning parts..." << std::endl;
            returnParts(argv[2], conn);
            std::cout << "Returned parts." << std::endl;
            exit(0);
        }

        if ((strcmp(argv[1], "--missing") == 0) || (strcmp(argv[1], "-m") == 0)) {
            if (argc > 3) {
                std::cerr << "Too many arguments supplied!\n"
                             "'missing' requires only a path to an XML-file." << std::endl;
                exit(-1);
            }
            else if (argc < 3) {
                std::cerr << "Too few arguments supplied!\n"
                    "'missing' requires a path to an XML-file." << std::endl;
                exit(-1);
            }
            std::cout << "Calculating missing parts..." << std::endl;
            getMissing(argv[2], conn);
            std::cout << "Calculated missing parts." << std::endl;
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
    setlocale(LC_ALL, "");
    initscr();
    set_escdelay(25);   // milliseconds
    cbreak();              // read input immediately (no line buffering)
    keypad(stdscr, TRUE);  // decode function & arrow keys
    start_color();
    use_default_colors();

    init_pair(CONTRAST_COLOR_ID, COLOR_RED, COLOR_BLACK);

    while (true) {
        char query[1000];
        printw("Enter part name: ");
        getstr(query);

        std::vector<Part> searchResult;
        search(query, searchResult, conn);
        if (searchResult.size() == 0) {
            clear();
            printw("No result found! ");
            continue;
        }

        int maxPadding = 0;
        for (auto result : searchResult) {
            if (result.name.length() > maxPadding)
                maxPadding = result.name.length();
        }

        clear();
        bool escaped = false;
        int selection = selectEntry(searchResult, escaped);
        if (escaped) {continue;}

        printw("Selected %s, %s\n",
                searchResult[selection].name.c_str(),
                searchResult[selection].number.c_str());
        if (searchResult[selection].getLocation().length() == 0) {
            editLocation(searchResult[selection], storagePrefix, conn);
        }
        else {
            printw("Edit color or location (c/l)? ");
            char buffer[256];
            getstr(buffer);
            std::string answer = buffer;
            move(getcury(stdscr) - 1, 0); // Move cursor up one line
            clrtoeol(); // Erase to end of line
            refresh(); // Update the screen

            if (answer == "c") {
                editColor(searchResult[selection], conn);
            }
            else if (answer == "l") {
                editLocation(searchResult[selection], storagePrefix, conn);
            }
        }
        move(getcury(stdscr) - 1, 0); // Move cursor up one line
        clrtoeol(); // Erase to end of line
        refresh(); // Update the screen

    }
    return EXIT_SUCCESS;
}

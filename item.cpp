#include "item.h"
#include <ncurses.h>
#include <pqxx/pqxx>

//##############
//Item
//##############
Item::Item(std::string pID, unsigned int cID, unsigned int quantity, pqxx::connection& conn):
    partID{pID}, colorID{cID}, quantity(quantity), conn(conn) {}

bool Item::operator!=(const Item& other) {
    if (this->colorID != other.colorID) {
        return true;
    }
    if (this->partID != other.partID) {
        return true;
    }
    return false;
}

void Item::operator+(const Item& other) {
    if ((*this) != other) {
        throw std::invalid_argument("Cannot add items with different color or part ID!");
    }
    this->setQuantity(this->getQuantity() + other.getQuantity());
}

void Item::operator-(const Item& other) {
    if ((*this) != other) {
        throw std::invalid_argument("Cannot subtract items with different color or part ID!");
    }
    this->setQuantity(this->getQuantity() - other.getQuantity());
}

unsigned int Item::getQuantity() const {return this->quantity;}

void Item::setQuantity(unsigned int qty) {
    pqxx::work work(this->conn);
    if (qty == 0) {

        work.exec("DELETE FROM stock WHERE number=$1 AND color=$2;", pqxx::params{this->partID, this->colorID});
    }
    else {
        work.exec(
                "INSERT INTO stock (number, color, quantity) "
                "VALUES($1, $2, $3) "
                "ON CONFLICT (number, color) DO UPDATE "
                "SET quantity = EXCLUDED.quantity;",
                pqxx::params{this->partID, this->colorID, qty}
        );
    }
    this->quantity = qty;
    work.commit();
}

//###################
//Color
//###################
unsigned int Color::maxNameLength = 0;
unsigned int Color::maxQtyLength = 0;
short Color::maxNcursesID = 2;

Color::Color(int cID, std::string name, std::string rgb, int quantity, std::string partID, pqxx::connection& conn):
    Item(partID, cID, quantity, conn),
    name(name),
    rgb(rgb) {
        if (this->name.length() > Color::maxNameLength) {
            Color::maxNameLength = this->name.length();
        }
        if (std::to_string(quantity).length() > Color::maxQtyLength) {
            Color::maxQtyLength = std::to_string(quantity).length();
        }

        // create color pair from hex
        // expect "RRGGBB" (6 chars, no '#')
        int r = std::stoi(this->rgb.substr(0, 2), nullptr, 16);
        int g = std::stoi(this->rgb.substr(2, 2), nullptr, 16);
        int b = std::stoi(this->rgb.substr(4, 2), nullptr, 16);
        this->ncursesID = Color::maxNcursesID++;
        init_color(this->ncursesID, static_cast<short>(r*1000/255), static_cast<short>(g*1000/255), static_cast<short>(b*1000/255));
        init_pair(this->ncursesID, COLOR_BLACK, this->ncursesID);
}

void Color::setQuantity(unsigned int qty) {
    Item::setQuantity(qty);
    if (std::to_string(this->getQuantity()).length() > Color::maxQtyLength) {
        Color::maxQtyLength = std::to_string(this->getQuantity()).length();
    }
}

void Color::print(bool highlighted) {
    attron(COLOR_PAIR(this->ncursesID));
    printw("  ");
    attroff(COLOR_PAIR(this->ncursesID));
    if (highlighted) {attron(COLOR_PAIR(1));}
    printw("%s", this->name.c_str());
    if (highlighted) {attroff(COLOR_PAIR(1));}
    printw("%s", std::string(Color::maxNameLength - this->name.length(), ' ').c_str());
    printw(" | ");
    printw("%s", std::string(maxQtyLength - std::to_string(this->getQuantity()).length(), ' ').c_str());
    printw("%i\n", this->getQuantity());
}

void Color::openBricklink() {
    std::string command = "firefox 'https://www.bricklink.com/catalogList.asp?v=2&colorID='";
    command += std::to_string(this->colorID);
    system(command.c_str());
}

//###############
//Part
//###############

Part::Part(std::string number, std::string name, std::string location) : number{number}, name{name}, location{location} {}

void Part::print(bool highlighted) {
    init_pair(1, COLOR_RED, COLOR_BLACK);
    int rows, cols;
    getmaxyx(stdscr, rows, cols);
    if (highlighted) {
        attron(COLOR_PAIR(1));
    }
    if ((name.length() + number.length() + location.length() + 7) > cols) {
        printw("%s [...] %s%s", name.substr(0, cols - number.length() - location.length() - 7).c_str(), location.c_str(), number.c_str());
    }
    else {
        printw("%s%s%s%s",
                name.c_str(),
                std::string(cols - name.length() - number.length() - location.length(), ' ').c_str(),
                location.c_str(),
                number.c_str());
    }
    if (highlighted) {
        attroff(COLOR_PAIR(1));
    }
}

void Part::openBricklink() {
    std::string command = "firefox https://www.bricklink.com/v2/search.page?q=";
    command += this->number;
    command += "#T=P";
    system(command.c_str());
}

std::string Part::getLocation() {return this->location;}

void Part::setLocation(std::string location, std::string storagePrefix, pqxx::connection& conn) {
    pqxx::work work(conn);
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

void Part::getAvailableColors(std::vector<Color>& colors, pqxx::connection& conn) {
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
                this->number,
                conn
        });
    }
}


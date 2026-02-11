#ifndef ITEM_H
#define ITEM_H

#include <array>
#include <string>
#include <vector>
#include <pqxx/pqxx>

#define CONTRAST_COLOR_ID 1
#define MIN_PAIR_ID 10
#define MAX_DISPLAY 25

class Item {
    public:
        Item(std::string, unsigned int, unsigned int, pqxx::connection&);
        unsigned int getQuantity() const;
        void setQuantity(unsigned int);
        void operator+(const Item&);  // void is unconventional but allowed
        void operator-(const Item&);
        bool operator!=(const Item&);
        const std::string partID;
        const unsigned int colorID;
    protected:
        pqxx::connection& conn;
    private:
        unsigned int quantity;
};

class Color : public Item {
    public:
        std::string name,
                    rgb;
        Color(int, std::string, std::string, int, std::string, pqxx::connection&);
        void setQuantity(unsigned int);
        void print(bool = false);
        void openBricklink();
    private:
        static unsigned int maxNameLength,
                            maxQtyLength;
        unsigned short pairID;  // color pair id for ncurses
        static unsigned short maxPairID;
};

class Part {
    public:
        std::string number,
                    name;
        void print(bool = false);
        Part(std::string, std::string, std::string);
        std::string getLocation();
        void setLocation(std::string, std::string, pqxx::connection&);
        void openBricklink();
        void getAvailableColors(std::vector<Color>&, pqxx::connection&);
    private:
        std::string location;
};

#endif

#ifndef ITEM_H
#define ITEM_H

#include <string>
#include <vector>
#include <pqxx/pqxx>

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
        short ncursesID;
        Color(int, std::string, std::string, int, std::string, pqxx::connection&);
        void setQuantity(unsigned int);
        void print(bool = false);
        void openBricklink();
    private:
        static unsigned int maxNameLength,
                            maxQtyLength;
        static std::vector<std::string> createdColors;
        static short maxNcursesID;
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

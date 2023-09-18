#ifndef Order_hpp
#define Order_hpp


#include <string>

struct Order {
    std::string action;
    int id;
    double price;
    int quantity;
    bool operator<(const Order& other) const {
        return price < other.price;
    }
    bool operator>(const Order& other) const {
        return price > other.price;
    }

};

#endif
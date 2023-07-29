#pragma once
#include <vector>
#include <string>
#include <unordered_map>

using namespace std;

enum class Direction {
  buy,
  sell
};

struct Order {
  string symbol;
  unsigned lots;
  int price;
};

struct Trade {
  string symbol;
  unsigned lots;
  int price;
};

using OrderBookHalf = unordered_map<string, vector<Order>>;

class OrderBook {
  public:
    vector<Trade> add_buy(Order order);
    vector<Trade> add_sell(Order order);
  private:
    vector<Trade> add_order(Direction direction, Order order);
    OrderBookHalf buy_half, sell_half;
};
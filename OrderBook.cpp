#include "OrderBook.hpp"

using namespace std;

vector<Trade> OrderBook::add_buy(Order order) {
  return add_order(Direction::buy, order);
}

vector<Trade> OrderBook::add_sell(Order order) {
  return add_order(Direction::sell, order);
}

vector<Trade> OrderBook::add_order(Direction direction, Order order) {
  vector<Trade> matches{};
  if (order.lots == 0) {
    return matches;
  }

  int multiplier = direction == Direction::buy ? 1 : -1;
  auto& counter_orders = direction == Direction::buy ? sell_half[order.symbol] : buy_half[order.symbol];
  auto& existing_orders = direction == Direction::buy ? buy_half[order.symbol] : sell_half[order.symbol];

  for (auto& counter_order : counter_orders) {
    if (multiplier * (order.price - counter_order.price) >= 0) {
      auto lots_filled = std::min(counter_order.lots, order.lots);
      order.lots -= lots_filled;
      counter_order.lots -= lots_filled;
      Trade match {order.symbol, lots_filled, counter_order.price };
      matches.push_back(match);
    }
  }

  counter_orders.erase(remove_if(counter_orders.begin(), counter_orders.end(), [](const Order& co) {return co.lots == 0; }), counter_orders.end());
  if (order.lots > 0) {
    existing_orders.push_back(order);
  }

  return matches;
}
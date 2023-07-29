#include <iostream>
#include <fstream>

#include "OrderBook.hpp"

using namespace std;

constexpr auto INPUT_FILE = "input_orders.txt";

int main() {
  cout << "Welcome to TSM Orderbook." << endl;

  ifstream inf{ INPUT_FILE };

  if (!inf) {
    cout << "Could not open" << INPUT_FILE << " - exiting." << endl;
    return 1;
  }

  cout << "Opened " << INPUT_FILE << " successfully. Reading trades now." << endl;

  OrderBook book;

  string direction;
  Order order;

  while (inf >> direction >> order.symbol >> order.lots >> order.price) {
    vector<Trade> matches;
    if (order.lots <= 0) {
      cout << "lots <= 0 detected – exiting.";
      return 1;
    }
    if (direction == "BUY") {
      matches = book.add_buy(order);
    }
    else if (direction == "SELL") {
      matches = book.add_sell(order);
    }
    else {
      cout << "Unrecognized direction: " << direction << " - exiting." << endl;
      return 1;
    }
    for (auto match : matches) {
      cout << "Traded " << match.lots << " " << match.symbol << " AT " << match.price << endl;
    }
  }

  return 0;
}
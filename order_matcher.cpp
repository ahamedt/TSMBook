#include <iostream>
#include <fstream>
#include <queue>
#include <string>
#include <chrono>
#include <random>
#include <thread>
#include <vector>
#include <algorithm>
#include <mutex>

struct Order {
    std::string action; // Added action to the Order struct
    int id;
    double price;
    int quantity;
    bool operator<(const Order& other) const {
        return price < other.price; // For the max heap (buy orders)
    }
    bool operator>(const Order& other) const {
        return price > other.price; // For the min heap (sell orders)
    }
};

void generate_orders(int order_count, const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open " << filename << std::endl;
        return;
    }

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> price_dist(100.0, 200.0);
    std::uniform_int_distribution<> quantity_dist(1, 10);
    std::uniform_int_distribution<> action_dist(0, 1);

    for (int i = 0; i < order_count; ++i) {
        std::string action = (action_dist(gen) == 0) ? "BUY" : "SELL";
        double price = price_dist(gen);
        int quantity = quantity_dist(gen);
        file << action << " " << i << " " << price << " " << quantity << "\n";
    }

    file.close();
}

std::mutex heap_mutex;
std::priority_queue<Order> buyOrders; // Max heap
std::priority_queue<Order, std::vector<Order>, std::greater<Order> > sellOrders; // Min heap

void match_orders_threaded(std::vector<Order> orders) {
    for (const Order& order : orders) {
        heap_mutex.lock(); // Lock the shared heaps

        if (order.action == "BUY") {
            buyOrders.push(order);
        } else if (order.action == "SELL") {
            sellOrders.push(order);
        }

        // Try to match orders
        while (!buyOrders.empty() && !sellOrders.empty() &&
               buyOrders.top().price >= sellOrders.top().price) {
            Order buy = buyOrders.top();
            Order sell = sellOrders.top();
            int matchedQuantity = std::min(buy.quantity, sell.quantity);

            std::cout << "Matched BUY Order ID: " << buy.id << " with SELL Order ID: " << sell.id;
            std::cout << " for " << matchedQuantity << " units at " << sell.price << std::endl;

            buy.quantity -= matchedQuantity;
            sell.quantity -= matchedQuantity;

            if (buy.quantity == 0) {
                buyOrders.pop();
            } else {
                buyOrders.pop();
                buyOrders.push(buy);
            }
            if (sell.quantity == 0) {
                sellOrders.pop();
            } else {
                sellOrders.pop();
                sellOrders.push(sell);
            }
        }

        heap_mutex.unlock(); // Unlock the shared heaps
    }
}

std::vector<Order> read_orders(const std::string& filename) {
    std::ifstream inputFile(filename);
    if (!inputFile.is_open()) {
        std::cerr << "Failed to open orders file." << std::endl;
        exit(1);
    }

    std::vector<Order> orders;
    std::string action;
    Order tempOrder;
    while (inputFile >> action >> tempOrder.id >> tempOrder.price >> tempOrder.quantity) {
        tempOrder.action = action;
        orders.push_back(tempOrder);
    }

    inputFile.close();
    return orders;
}

int main() {

    const int order_count = 1000000; // Adjust the number of orders as needed
    const std::string filename = "order_generate.txt";

    generate_orders(order_count, filename);

    const int num_threads = 4;
    std::vector<Order> orders = read_orders("order_generate.txt");
    int orders_per_thread = orders.size() / num_threads;

    std::vector<std::thread> threads;

    // Measure the start time
    auto start_time = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < num_threads; ++i) {
        std::vector<Order>::iterator start = orders.begin() + i * orders_per_thread;
        std::vector<Order>::iterator end = (i == num_threads - 1) ? orders.end() : start + orders_per_thread;

        threads.push_back(std::thread(match_orders_threaded, std::vector<Order>(start, end)));
    }

    for (std::thread& thread : threads) {
        thread.join();
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed_time = end_time - start_time;

    std::cout << "Processed " << orders.size() << " orders in " << elapsed_time.count() << " seconds." << std::endl;
    return 0;
}

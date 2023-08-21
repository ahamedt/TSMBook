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
#include <condition_variable>
#include <atomic>

struct Order {
    std::string action;
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

std::mutex master_heap_mutex;
std::condition_variable master_heap_cv;
std::priority_queue<Order> masterBuyOrders;
std::priority_queue<Order, std::vector<Order>, std::greater<Order>> masterSellOrders;
std::atomic<bool> done(false); // To indicate when all threads are done processing orders.

int globalCounter = 0;

void match_orders(std::priority_queue<Order>& buyOrders, std::priority_queue<Order, std::vector<Order>, std::greater<Order>>& sellOrders) {
    while (!buyOrders.empty() && !sellOrders.empty() &&
           buyOrders.top().price >= sellOrders.top().price) {
        Order buy = buyOrders.top();
        Order sell = sellOrders.top();
        int matchedQuantity = std::min(buy.quantity, sell.quantity);

        globalCounter++;

        // std::cout << "Matched BUY Order ID: " << buy.id << " with SELL Order ID: " << sell.id;
        // std::cout << " for " << matchedQuantity << " units at " << sell.price << std::endl;

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
}


void match_orders_threaded(std::vector<Order> orders) {
    std::priority_queue<Order> localBuyOrders;
    std::priority_queue<Order, std::vector<Order>, std::greater<Order>> localSellOrders;

    int orders_processed = 0;
    const int push_threshold = 7000; // Push to master heap after every 5000 orders processed.

    for (const Order& order : orders) {
        if (order.action == "BUY") {
            localBuyOrders.push(order);
        } else if (order.action == "SELL") {
            localSellOrders.push(order);
        }

        match_orders(localBuyOrders, localSellOrders);

        if (++orders_processed % push_threshold == 0) {
            std::unique_lock<std::mutex> lock(master_heap_mutex);
            while (!localBuyOrders.empty()) {
                masterBuyOrders.push(localBuyOrders.top());
                localBuyOrders.pop();
            }
            while (!localSellOrders.empty()) {
                masterSellOrders.push(localSellOrders.top());
                localSellOrders.pop();
            }
            lock.unlock();
            master_heap_cv.notify_one();
        }
    }

    std::unique_lock<std::mutex> lock(master_heap_mutex);
    while (!localBuyOrders.empty()) {
        masterBuyOrders.push(localBuyOrders.top());
        localBuyOrders.pop();
    }
    while (!localSellOrders.empty()) {
        masterSellOrders.push(localSellOrders.top());
        localSellOrders.pop();
    }
    lock.unlock();
    master_heap_cv.notify_one();
}

void match_orders_master() {
    while (!done) {
        std::unique_lock<std::mutex> lock(master_heap_mutex);
        master_heap_cv.wait(lock, [] { return !masterBuyOrders.empty() || !masterSellOrders.empty() || done; });
        match_orders(masterBuyOrders, masterSellOrders);
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

    const int num_threads = 50;
    std::vector<Order> orders = read_orders("order_generate.txt");
    int orders_per_thread = orders.size() / num_threads;

    std::thread masterThread(match_orders_master);
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

    done = true;
    master_heap_cv.notify_one();
    masterThread.join();

    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed_time = end_time - start_time;

    std::cout << "Processed " << orders.size() << " orders in " << elapsed_time.count() << " seconds." << std::endl;
    std::cout << "Total number of orders matched: " << globalCounter << std::endl;
    return 0;
}

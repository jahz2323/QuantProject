#include "Connector.hpp"
#include "Handlers.hpp"
#include "OrderBook.hpp"

#include <csv/document.hpp>

#include <iostream>
#include <vector>
#include <memory>
#include <atomic>
#include <csignal>
#include <chrono>
#include <thread>

std::atomic<bool> g_running{true};

void signal_handler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        g_running.store(false, std::memory_order_relaxed);
    }
}

int main() {
    std::signal(SIGINT, signal_handler);

    CSV::Document doc;
    doc.load_from_file(Config::BINANCE_HISTORICAL_DATASET_DIR);
    return 0;
}
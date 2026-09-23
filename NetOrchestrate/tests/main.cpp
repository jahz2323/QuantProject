#include "connector.hpp"
#include "handlers.hpp"
#include "OrderBook.hpp"

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

    // 1. Define exchange connection endpoints
    const std::string uri = "stream.binance.com";
    const std::string port = "9443";
    // const std::string target = "/stream?streams="
    //                            "btcusdt@depth20@100ms/"
    //                            "btcusdt@aggTrade/"
    //                            "ethusdt@bookTicker";
    std::string combined_target = "/stream?streams=ethusdt@bookTicker/btcusdt@aggTrade/btcusdt@depth20@100ms";
    

    // 3. Instantiate orchestrator factory
    ConnectionFactory factory;

    std::cout << "[Engine] Launching multi-stream market data pipeline...\n";

    // 2. Define full streams and MarketQueues 
    // 4096 bytes per frame, 1024 frames in the queue+
    auto btcusdt_depth20_queue = std::make_unique<MarketQueue>();
    auto btcusdt_aggTrade_queue = std::make_unique<MarketQueue>();
    auto ethusdt_bookTicker_queue = std::make_unique<MarketQueue>();


    std::unordered_map<std::string, MarketQueue*> stream_targets = {
        {"/ws/btcusdt@depth20@100ms", &btcusdt_depth20_queue.get()},
        {"/ws/btcusdt@aggTrade", &btcusdt_aggTrade_queue.get()},
        {"/ws/ethusdt@bookTicker", &ethusdt_bookTicker_queue.get()}
    };

    //4. Launch ocnnections 
    factory.launch_connections<WebSocketBinanceConnector>(uri, port, combined_target, stream_targets, g_running);

    std::cout << "[Engine] Pipeline running. Press Ctrl+C to stop.\n";

    // 5. Keep main thread alive until shutdown signal
    while (g_running.load(std::memory_order_relaxed)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    std::cout << "[Engine] Shutdown complete.\n";
    return 0;
}
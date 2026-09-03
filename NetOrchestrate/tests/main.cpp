#include "connector.hpp"
#include "OrderBook.hpp"
#include <iostream>


int main() {
    // std::string uri = "stream.binance.com";
    // std::string port = "9443";
    // std::string target = "/ws/btcusdt@depth20@100ms";

    // ConnectorClient client(uri, port, target);
    // client.create_binance_connection();
    ConnectionFactory factory(4); // Allow up to 10 clients
    BUMP_ALLOCATOR* allocator = factory.GetAllocator();
    printf("Allocator block size: %zu, number of blocks: %zu\n", allocator->getNodeSize(), allocator->getNumNodes());

    // Define endpoints - currently only works for Diff. Depth streams 
    std::vector<Endpoint_entry> endpoints = {
        {"stream.binance.com", "9443", "/ws/btcusdt@depth20@100ms"},
        {"stream.binance.com", "9443", "/ws/ethusdt@depth10@100ms"},
        {"stream.binance.com", "9443", "/ws/solusdt@depth5@100ms"},
        {"stream.binance.com", "9443", "/ws/bnbusdt@depth15@100ms"},
    };

    try {
        factory.startClients(endpoints);
    } catch (const std::exception& e) {
        std::cerr << "Error starting clients: " << e.what() << std::endl;
    }

    return 0;
}
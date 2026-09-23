#pragma once
// Class to connect to external WebSocket server 
#include <boost/asio/ssl.hpp>
#include <boost/beast.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/json.hpp>
#include <boost/thread.hpp>
#include <boost/asio.hpp>
#include <boost/lockfree/spsc_queue.hpp>

//Multithread 
#include <boost/thread.hpp>

//stl imports
#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

// local includes
#include "OrderBook.hpp"
#include "MemoryAllocators.hpp" // has FREE_LIST and BUMP_ALLOCATOR
#include "StreamConfig.hpp"

// Common namespace aliases used in Beast examples.
namespace asio = boost::asio;
namespace ssl = asio::ssl;
namespace beast = boost::beast;
namespace http = beast::http;
namespace json = boost::json;
namespace websocket = beast::websocket;

// Convenience alias for TCP networking.
using tcp = asio::ip::tcp;
using namespace boost::this_thread; 


struct RawMarketFrame 
{
    char data[4096];
    size_t length{0};
};

using MarketQueue = boost::lockfree::spsc_queue<RawMarketFrame, boost::lockfree::capacity<1024>>;


class IConnectorClient 
{
    public:  
    virtual ~IConnectorClient() = default;
    virtual void register_route(const std::string_view stream_id, std::unique_ptr<MarketQueue> queue) = 0; 
    virtual void run(const std::string& uri, const std::string& port, const std::string& target, const std::atomic<bool>& running) =0;
}; 

class WebSocketBinanceConnector : public IConnectorClient
{
    public:
    std::unordered_map<std::string, std::unique_ptr<MarketQueue>> m_routes;
    WebSocketBinanceConnector() = default;
    ~WebSocketBinanceConnector() = default;
    void register_route(const std::string_view full_stream, std::unique_ptr<MarketQueue> queue) override;
    void run(const std::string& uri, const std::string& port, const std::string& target, const std::atomic<bool>& running) override;
};

//ConnectionFactory 
class ConnectionFactory 
{
    private: 
    BUMP_ALLOCATOR* m_client_allocator; 

    std::vector<std::unique_ptr<IConnectorClient>> m_clients;
    std::vector<std::unique_ptr<MarketQueue>> m_queues;
    std::vector<std::thread> m_threads;
    public:  
    ConnectionFactory() = default;
    ~ConnectionFactory() {
        stop_and_join_all();
    }

    void stop_and_join_all() {
        for (auto& thread : m_threads) {
            if (thread.joinable()) {
                thread.join();
            }
        }
    }

    inline BUMP_ALLOCATOR* GetAllocator() { return m_client_allocator; }
    
        
    template<typename ConnectorType>
    void launch_connections(
        const std::string& uri,
        const std::string& port,
        const std::string& combined_target,
        std::unordered_map<std::string, MarketQueue*> full_stream_routes,
        const std::atomic<bool>& running
        
    ) {
        //Create a new connector instance of the specified type
        auto connector = std::make_unique<ConnectorType>(); 

        // Register routes for each full_stream in the provided map
        for(const auto& [full_stream, queue] : full_stream_routes) {
            connector->register_route(full_stream, std::make_unique<MarketQueue>());
        }

        //Get raw pointer to the connector for thread management
        IConnectorClient* connector_ptr = connector.get();
        
        //add the connector to the list of clients
        m_clients.push_back(std::move(connector));

        // Capture raw_connector_ptr directly in the lambda 
        m_threads.emplace_back([connector_ptr, uri, port, combined_target, &running]() {
            connector_ptr->run(uri, port, combined_target, running);
        });
    }
};



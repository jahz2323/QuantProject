//handlers.hpp
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

// local includes
#include "OrderBook.hpp"
#include "Connector.hpp"
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


class IHandler
{
    // Base class for all handlers
    public:
    virtual void process_loop(const std::atomic<bool>& running) = 0; // pure virtual function
    virtual ~IHandler() = default; // virtual destructor
};

class BookTickerHandler : public IHandler
{
    void process_loop(const std::atomic<bool>& running) override;
}; 

class AggregateTradeHandler : public IHandler
{
    void process_loop(const std::atomic<bool>& running) override;
};

class OrderBookHandler  : public IHandler
{
    // OrderBookHandler consumes frames from shared queue with ConnectorClient and updates the TradeBook instance
    private: 
    TradeBook<20> m_trade_book; // TradeBook instance to hold the order book data
    MarketQueue& m_queue; // SPSC queue for raw market data
    boost::json::stream_parser m_parser; // JSON parser for incoming market data

    public: 
    OrderBookHandler(const std::string& target, MarketQueue& queue) : m_trade_book(StreamConfig::parse_string_target(target).symbol), m_queue(queue) {}
    void process_loop(const std::atomic<bool>& running) override;
    void updateTradeBook(TradeBook<20>& book, const boost::json::value& jv);
};


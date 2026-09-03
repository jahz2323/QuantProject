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
#include "MemoryAllocators.hpp" // has FREE_LIST and BUMP_ALLOCATOR
#include "handlers.hpp" // has RAWMarketFrame, MarketQueue

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


struct StreamConfig
{
    boost::core::string_view symbol;
    boost::core::string_view update_speed;
    int depth = 20;

    static StreamConfig parse_string_target(std::string_view target)
    {
        StreamConfig config;
        // Example target: "/ws/btcusdt@depth20@100ms"
        if (target.starts_with("/ws/")){
            target.remove_prefix(4); // Remove "/ws/" -> "btcusdt@depth20@100ms"
        }

        //extract symbol 
        auto at_1 = target.find("@depth");
        if (at_1 != std::string_view::npos){
            //symbol is everything before @depth
            config.symbol = target.substr(0, at_1);
            //remove symbol from target
            target.remove_prefix(at_1); // "@depth20@100ms"
        }

        //extract depth
        auto at_2 = target.find("@depth");
        if (at_2 != std::string_view::npos){
            //depth is after @depth and before next @ 
            //@depth is 6 characters long
            auto depth_start = at_2 + 6;
            auto depth_end = target.find("@", depth_start);
            // if no @ found, depth is everything after @depth
            if (depth_end == std::string_view::npos){
                depth_end = target.size();
                target.remove_prefix(depth_start); // remove everything before depth
                config.depth = std::stoi(std::string(target)); // convert to int
        
            }
            // if @ found, depth is everything between @depth and next @
            else{
                target.remove_prefix(depth_start); // remove everything before depth
                config.depth = std::stoi(boost::core::string_view(target.substr(0, depth_end - depth_start)));
                
                // extract update_speed 
                target.remove_prefix(depth_end - depth_start); // remove everything before update_speed
                if (target.starts_with("@")){
                    target.remove_prefix(1); // remove @
                    config.update_speed = target; // everything after @ is update_speed
                }
            }
        }

        return config;
    }
};


class BookTickerHandler{

}; 

class AggregateTradeHandler{

};

class OrderBookHandler 
{
    // OrderBookHandler consumes frames from shared queue with ConnectorClient and updates the TradeBook instance
    private: 
    TradeBook<20> m_trade_book; // TradeBook instance to hold the order book data
    MarketQueue& m_queue; // SPSC queue for raw market data
    boost::json::stream_parser m_parser; // JSON parser for incoming market data

    public: 
    OrderBookHandler(const std::string& target, MarketQueue& queue) : m_trade_book(StreamConfig::parse_string_target(target).symbol), m_queue(queue) {}

    void process_loop(const std::atomic<bool>& running)
    {
        RawMarketFrame frame;
        while(running.load())
        {
            if (m_queue.pop(frame))
            {
                
                boost::string_view sv(static_cast<const char*>(frame.data), frame.length); 
                m_parser.reset();
                m_parser.write(sv);
                m_parser.finish();
                // Access the parsed JSON value
                boost::json::value jv = m_parser.release();
                //std::cout << "Received JSON: " << jv << std::endl;
        
                const std::string symbol = m_trade_book.getSymbol();
                std::cout << "Updating order book for symbol: " << symbol << std::endl;
                //log to m_trade_book
                updateTradeBook(m_trade_book, jv);
            }
            // else queue is empty, sleep for a short time to avoid busy waiting
            else
            {
                boost::this_thread::yield();
            }
        }
    }

    
    void updateTradeBook(TradeBook<20>& book, const boost::json::value& jv)
    {
        if (!jv.is_object()) {
            throw std::invalid_argument("JSON value is not an object");
        }   
        boost::json::object obj = jv.as_object(); 

        // update the first and last update IDs
        if (book.first_update_id == 0) {
            book.first_update_id = boost::json::value_to<uint64_t>(obj["lastUpdateId"]); // set first update ID on first update
        }
        book.last_update_id = boost::json::value_to<uint64_t>(obj["lastUpdateId"]); // update last update ID on every update
        const boost::json::array* bids_array = nullptr; 
        const boost::json::array* asks_array = nullptr;
        
        // update bids - iterate through incoming bids as an array [price, quantity]
        for(const auto& bv: obj["bids"].as_array())
        {
            const auto& level = bv.as_array();
            float price = std::stof(std::string(level[0].as_string()));
            float quantity = std::stof(std::string(level[1].as_string()));
            book.bids.Update(price, quantity);
        }

        // update asks
        for(const auto& av: obj["asks"].as_array())
        {
            const auto& level = av.as_array();
            float price = std::stof(std::string(level[0].as_string()));
            float quantity = std::stof(std::string(level[1].as_string()));
            book.asks.Update(price, quantity);
        }
        //print out the current state of the order book
        std::cout << "Current Order Book for " << m_trade_book.getSymbol() << ":" << std::endl;
        std::cout << "Bids:" << std::endl;
        m_trade_book.bids.PrintOrderBook();
        std::cout << "Asks:" << std::endl;
        m_trade_book.asks.PrintOrderBook();
    }
};


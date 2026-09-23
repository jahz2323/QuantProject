//handlers.cpp
#include "handlers.hpp"


void OrderBookHandler::process_loop(const std::atomic<bool>& running)
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

    
void OrderBookHandler::updateTradeBook(TradeBook<20>& book, const boost::json::value& jv)
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



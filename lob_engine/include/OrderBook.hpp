/**
 * Requirements: 
 * Hold table of Bids and Asks 
 * OrderBook size based on depth of book
 * 
 * Bids to be sorted by price Descending
 * Asks to be sorted by price Ascending
 * 
 * bids should show 
 * Price, Quantity 
 * 
 * Methods 
 * UpdateBid() 
 * UpdateAsk()
 * DeleteBid()
 * DeleteAsk()
 * Trade() 
 * 
 * Metric Tracking 
 * GetBestBid()
 * GetBestAsk()
 * GetMidPrice() 
 * GetBidAskSpread()
 * GetOrderFlowImbalance()
 * 
 * 
 */
#pragma once
#include <iostream>
#include <limits>
#include <cstdint>

enum class OrderType
{
    BID, ASK
};

// need a datastructure to hold bids and asks, 
struct OrderBookEntry
{
    float price; 
    float quantity; 
};

template <OrderType order_type, int depth>
struct OrderBook
{   
    int m_depth = depth;
    OrderBookEntry entries[depth];
    OrderBook()
    {
        
        if constexpr (order_type == OrderType::BID)
        {
            for (int i = 0; i < depth; i++)
            {
                entries[i] = {0.0, 0};
            }
        }
        if constexpr (order_type == OrderType::ASK)
        {
            for (int i = 0; i < depth; i++)
            {
                entries[i] = {std::numeric_limits<float>::max(), 0};
            }
        }
        
    }
    void PrintOrderBook()
    {
        std::cout << "INDEX \t PRICE \t QUANTITY" << std::endl;
        for (int i =0; i < depth; i++)
        {
            std::cout << i << "\t" << entries[i].price << "\t" << entries[i].quantity << std::endl;   
        }
    }
    
    // Universal Update Function

    void Update(float in_price, float in_quantity)
    {
        for(int i =0; i < m_depth; i++)
        {
            if (entries[i].price == in_price )
            {   
                // if matched and new quantity > 0, update quantity
                if(in_quantity > 0) {
                    entries[i].quantity = in_quantity;
                } else {
                    // if matched and new quantity == 0, delete entry and shift entries up by 1
                    for (int j = i; j < m_depth - 1; ++j){
                        entries[j] = entries[j+1];
                    }
                }
                if constexpr (order_type == OrderType::BID) {
                    entries[m_depth - 1] = {0.0, 0}; // Reset last entry for bids
                } else {
                    entries[m_depth - 1] = {std::numeric_limits<float>::max(), 0}; // Reset last entry for asks
                }
                //NOTE: trailing entries will be stale 
                return; 
            }
            bool should_insert = false;
            if constexpr (order_type == OrderType::BID){
                should_insert = (in_price > entries[i].price);
            } else {
                should_insert = (in_price < entries[i].price);
            }
            if(should_insert)
            {
    
                //if i+1 exists, move all entries down by 1 
                for(int j = m_depth - 1; j > i; j--)
                {
                    entries[j] = entries[j-1]; 
                }
                entries[i] = {in_price, in_quantity};
            
                return; // exit early
            }
            
        }
    }

};

template <int depth = 20>
class TradeBook 
{
    private: 
    public:
    uint64_t first_update_id = 0;
    uint64_t last_update_id; 
    std::string m_symbol;
    OrderBook<OrderType::BID, depth> bids;
    OrderBook<OrderType::ASK, depth> asks;
    TradeBook(std::string symbol) : m_symbol(symbol), bids(), asks() {};
    ~TradeBook() = default;
    std::string getSymbol() const { return m_symbol; }

    void GetOrderFlowImbalance() 
    {
        //OFI =  (sum(bid_depths) - sum(ask_depths)) / (sum(bid_depths) + sum(ask_depths) + epsilon)
    }
}; 



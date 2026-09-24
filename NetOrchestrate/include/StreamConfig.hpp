//StreamConfig.hpp
#pragma once
#include <boost/asio/ssl.hpp>
#include <boost/beast.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/json.hpp>
#include <boost/thread.hpp>
#include <boost/asio.hpp>
#include <boost/lockfree/spsc_queue.hpp>

#include <string>
#include <iostream>

struct StreamConfig
{
    boost::core::string_view update_speed;
    boost::core::string_view symbol;
    boost::core::string_view full_stream;
    boost::core::string_view stream_type;

    int depth = 20; // default is set to 20, but can be changed based on specified depth
    bool is_valid() const { return !full_stream.empty();}
    static StreamConfig ExtractStreamConfig(std::string_view raw_json);
    static StreamConfig parse_string_target(std::string_view target);
    static StreamConfig parse_combined_stream(std::string_view combined_stream);
    void printStreamConfig() const;
};

struct AggTradeTick {
    uint64_t agg_trade_id; 
    double price; 
    double quantity;
    uint64_t first_trade_id;
    uint64_t last_trade_id;
    uint64_t timestamp;
    bool is_buyer_maker;
    bool is_best_match;

    static AggTradeTick parse_agg_trade_tick(std::string_view line);
};
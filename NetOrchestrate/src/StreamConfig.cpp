//StreamConfig.cpp
#include "StreamConfig.hpp"

StreamConfig StreamConfig::ExtractStreamConfig(std::string_view raw_json)
{
    const std::string stream_key = "\"stream\":\"";
    auto stream_pos = raw_json.find(stream_key);
    if(stream_pos == std::string_view::npos) return {}; 
    
    auto symbol_start = stream_pos + stream_key.size();
    if(symbol_start == std::string_view::npos) return {};

    auto at_start = raw_json.find('@', symbol_start);
    if(at_start == std::string_view::npos) return {};

    auto end_quote = raw_json.find('"', at_start);
    if(end_quote == std::string_view::npos) return {};

    StreamConfig config;
    config.symbol = raw_json.substr(symbol_start, at_start - symbol_start);
    config.stream_type = raw_json.substr(at_start + 1, end_quote - at_start - 1);
    config.full_stream = raw_json.substr(stream_pos, end_quote - stream_pos + 1);
    return config;
}
StreamConfig StreamConfig::parse_string_target(std::string_view target)
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
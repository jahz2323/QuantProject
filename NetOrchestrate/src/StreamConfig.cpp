//StreamConfig.cpp
#include "StreamConfig.hpp"

void StreamConfig::printStreamConfig() const {
    std::cout << "StreamConfig: {"
              << "update_speed: " << update_speed << ", "
              << "symbol: " << symbol << ", "
              << "full_stream: " << full_stream << ", "
              << "stream_type: " << stream_type << ", "
              << "depth: " << depth
              << "}" 
              << std::endl;
}

StreamConfig StreamConfig::ExtractStreamConfig(std::string_view raw_json)
{
    int depth = 20;
    std::string_view update_speed;

    const std::string stream_key = "\"stream\":\"";
    auto stream_pos = raw_json.find(stream_key);
    if(stream_pos == std::string_view::npos) return {}; 
    
    auto comma_pos = raw_json.find(',', stream_pos);
    if(comma_pos == std::string_view::npos) return {};

    auto symbol_start = stream_pos + stream_key.size();
    if(symbol_start == std::string_view::npos) return {};

    auto at_start = raw_json.find('@', symbol_start);
    if(at_start == std::string_view::npos) return {};

    auto at_depth_pos = raw_json.find("@depth", at_start);
    if(at_depth_pos != std::string_view::npos)
    {
        auto depth_start = at_depth_pos + 6;
        auto depth_end = raw_json.find('@', depth_start);
        auto update_speed_start = raw_json.find('@', depth_end); 
        if(depth_end == std::string_view::npos)
        {
            auto depth_str = raw_json.substr(depth_start, depth_end - depth_start); 
            try{
                depth = std::stoi(std::string(depth_str));
                update_speed = raw_json.substr(update_speed_start + 1, comma_pos - update_speed_start - 1);
            } catch(const std::exception& e) {
                std::cerr << "Error parsing depth: " << e.what() << std::endl;
                depth = 20; // fallback to default
            }
        }
    }

    
    auto end_quote = raw_json.find('"', at_start);
    if(end_quote == std::string_view::npos) return {};
    

    StreamConfig config;
    config.symbol = raw_json.substr(symbol_start, at_start - symbol_start);
    config.stream_type = raw_json.substr(at_start + 1, end_quote - at_start - 1);
    config.update_speed = (update_speed.empty()) ? "NAN" : update_speed; // default to NAN if not specified
    config.full_stream = raw_json.substr(symbol_start, end_quote - symbol_start);//stop before ,  
    config.depth = (depth != 0) ? depth : 20; // default depth is 20 if not specified

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

AggTradeTick AggTradeTick::parse_agg_trade_tick(std::string_view line)
{
    AggTradeTick tick;
    
}
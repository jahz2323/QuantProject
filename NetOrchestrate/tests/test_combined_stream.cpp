#include "connector.hpp"
#include "OrderBook.hpp"
#include "handlers.hpp"
#include <iostream>



int main() 
{
    /**
     * Endpoint wss://stream.binance.com:9443
     * Individual Symbol BookTicker{symbol}@bookTicker
     * Aggregate Trade Streams:{symbol}@aggTrade
     * Different Depth Streams: {symbol}@depth{levels}@{speed}
     * Example Combined Streams: 
     * /stream?streams={symbol1}@bookTicker/{symbol2}@bookTicker/{symbol3}@aggTrade/{symbol4}@depth{levels}@{speed}
     */
    // std::string_view combined_starget =
    // "/stream?streams=btcusdt@depth20@100ms/ethusdt@depth10@100ms/solusdt@depth5@100ms/bnbusdt@depth15@100ms";
    std::string combined_starget = "/stream?streams=ethusdt@bookTicker/btcusdt@aggTrade/btcusdt@depth20@100ms";
    std::cout << combined_starget << std::endl;
    const std::string uri = "stream.binance.com";
    const std::string port = "9443";

    //Create io context
    asio::io_context ioc;
    ssl::context ctx(ssl::context::sslv23);
    ctx.set_default_verify_paths();
    
    //create resolver and socket
    tcp::resolver resolver(ioc);
    websocket::stream<beast::ssl_stream<beast::tcp_stream>> ws(ioc, ctx);

    // Resolve the host
    /**
     *  On Windows, host names may be defined in the file c:\windows\system32\drivers\etc\hosts. Remote host name resolution is performed using DNS. Operating systems may use additional locations when resolving host names (such as NETBIOS names on Windows). 
     */
    // auto const results = resolver.resolve("host","service");
    auto const results = resolver.resolve(uri, port);

    // connect underlying tcp socket to resolved endpoint
    beast::get_lowest_layer(ws).expires_after(std::chrono::seconds(5));
    beast::get_lowest_layer(ws).connect(results);

    // Perform the SSL handshake
    beast::get_lowest_layer(ws).expires_after(std::chrono::seconds(5));
    ws.next_layer().handshake(ssl::stream_base::client);

    //perform websocket handshake
    beast::get_lowest_layer(ws).expires_after(std::chrono::seconds(5));
    ws.handshake(uri, combined_starget);

    //print out raw ip address of connected endpoint
    std::cout << "Connected to: " << beast::get_lowest_layer(ws).socket().remote_endpoint() << std::endl;
    beast::get_lowest_layer(ws).expires_never();

    std::cout << "Reading in Logs from WebSocket server..." << std::endl; 
    std::cout << "Press Ctrl+C to exit." << std::endl;

    while(true){
        beast::flat_buffer buffer;
        ws.read(buffer);

        std::string message = beast::buffers_to_string(buffer.data());
        //std::cout << "Received message: " << message << std::endl;

        StreamConfig config = StreamConfig::ExtractStreamConfig(message);
        if(config.is_valid()){
            std::cout << "Symbol: " << config.symbol << ", Stream Type: " << config.stream_type << std::endl;
        }
    }
}
#include "Connector.hpp"

void WebSocketBinanceConnector::register_route(const std::string_view full_stream, std::unique_ptr<MarketQueue> queue) {
    m_routes[std::string(full_stream)] = std::move(queue);
}
void WebSocketBinanceConnector::run(const std::string& uri, const std::string& port, const std::string&  target, const std::atomic<bool>& running) {
    try 
        {
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
            ws.handshake(uri, target);

            //print out raw ip address of connected endpoint
            std::cout << "Connected to: " << beast::get_lowest_layer(ws).socket().remote_endpoint() << std::endl;
            beast::get_lowest_layer(ws).expires_never();

            std::cout << "Reading in Logs from WebSocket server..." << std::endl; 
            std::cout << "Press Ctrl+C to exit." << std::endl;

            beast::flat_buffer buffer; 
            while(running.load()) 
            {
                buffer.clear(); 
                ws.read(buffer);

                RawMarketFrame frame;
                frame.length = std::min(buffer.size(), sizeof(frame.data));
                std::memcpy(frame.data, buffer.data().data(), frame.length);

                boost::core::string_view sv(static_cast<const char*>(frame.data), frame.length);

                StreamConfig config = StreamConfig::ExtractStreamConfig(sv);
                config.printStreamConfig(); // Print the extracted StreamConfig for debugging
                if (m_routes.count(config.full_stream)) {
                    // Found a matching route, push the frame to the corresponding queue
                    m_routes[config.full_stream]->push(frame);
                } else {
                    std::cerr << "No registered route for full_stream: " << config.full_stream << std::endl;
                }
            }
            // // Close the WebSocket connection
            ws.close(websocket::close_code::normal);
        }
        catch (const std::exception& e) 
        {
            std::cerr << "Error attempting to connect: " << e.what() << std::endl;
        }
}

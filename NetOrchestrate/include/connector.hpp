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

struct Endpoint_entry
{
    std::string uri;
    std::string port;
    std::string target;
};

struct RawMarketFrame 
{
    char data[4096];
    size_t length{0};
};
using MarketQueue = boost::lockfree::spsc_queue<RawMarketFrame, boost::lockfree::capacity<1024>>;


class ConnectorClient 
{
    private:
    MarketQueue& m_queue; 
    public:
    explicit ConnectorClient (MarketQueue& queue) : m_queue(queue) {}
    ~ConnectorClient() = default;
    
    void ssl_connection(const std::string& uri, const std::string& port, const std::string& target, const std::atomic<bool>& running) {
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

                if(!m_queue.push(frame))
                {
                    std::cerr << "Queue is full, dropping frame" << std::endl;
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
}; 


/**
 * Manage a family of ConnectorClient instances, 
 * Should work with multiple threads, each thread can have its own ConnectorClient instance
 * Each ConnectorClient instance can connect to a different WebSocket server and manage its own order book
 * 
 **/

class ConnectionFactory 
{
private: 
    BUMP_ALLOCATOR* m_client_allocator; 
    int m_max_clients = 10; 

public: 
    explicit ConnectionFactory(int max_clients) : m_max_clients(max_clients) {
        m_client_allocator = BUMP_ALLOCATOR::Get_Instance(max_clients, sizeof(ConnectorClient));
    }  
    BUMP_ALLOCATOR* GetAllocator() { return m_client_allocator; }

    // Start a thread that creates and manages a ConnectorClient instance
    void startClients(const std::vector<Endpoint_entry>& endpoints)
    {
        int n = endpoints.size();
        std::atomic<bool> running(true); // Atomic flag to control the running state of the threads
        
        std::vector<std::unique_ptr<MarketQueue>> queues(n); // Create a queue for each client
        queues.reserve(n); // Reserve space for n queues

        std::vector<std::thread> threads; // Store the threads for later joining
        std::vector<std::unique_ptr<OrderBookHandler>> handlers; // Store the handlers for each client
        std::vector<std::unique_ptr<ConnectorClient>> clients; // Store the clients for each thread

        if (n > m_max_clients || n <= 0) {
            throw std::runtime_error("Number of endpoints exceeds maximum number of clients or is zero");
        }
        
        for (size_t i = 0; i < n; ++i)
        {
            const auto& endpoint = endpoints[i];

            //Create shared queue 
            auto queue = std::make_unique<MarketQueue>();
            MarketQueue& ref_queue = *queue; // Reference to the queue for passing to client and handler
            queues.emplace_back(std::move(queue)); 

            // Allocate client & handler 
            auto* client_mem = m_client_allocator->allocate();
            auto* client = new (client_mem) ConnectorClient(ref_queue); // Placement new to construct the client in allocated memory
            clients.emplace_back(client);

            auto handler = std::make_unique<OrderBookHandler>(endpoint.target, ref_queue);
            OrderBookHandler* handler_ptr = handler.get(); // Get raw pointer for thread capture
            handlers.emplace_back(std::move(handler));

            // Spawn seperate Producer and Consumer threads for each client
            threads.emplace_back([client, endpoint, &running](){
                client->ssl_connection(endpoint.uri, endpoint.port, endpoint.target, running);
            }); 

            threads.emplace_back([handler_ptr, &running](){
                handler_ptr->process_loop(running);
            });
        }

        //RUN FACTORY FOR 10 SECONDS, THEN EXIT
        boost::this_thread::sleep_for(boost::chrono::seconds(10));

        //set running to false to stop all threads
        running.store(false);

        //join all threads
        for (auto& t : threads) {
            if (t.joinable()) {
                t.join();
            }
        }

        // Deallocate clients and handlers
        for (size_t i=0; i < n; ++i) {
            m_client_allocator->deallocate(static_cast<void*>(clients[i].get()));
            clients[i]->~ConnectorClient();
            handlers[i]->~OrderBookHandler();
        }
        std::cout << "Exiting..." << std::endl;
    }
};



/**
 * test github  api with asio
 */
#include <boost/asio/ssl.hpp>
#include <boost/beast.hpp>
#include <boost/json.hpp>
#include <iostream>

// lob_engine includes
#include "connector.hpp"

#include "k.h" // kdb+ c api header

// Common namespace aliases used in Beast examples.
namespace asio = boost::asio;
namespace ssl = asio::ssl;
namespace beast = boost::beast;
namespace http = beast::http;
namespace json = boost::json;

// Convenience alias for TCP networking.
using tcp = asio::ip::tcp;

std::string getEnvVar(const std::string& varName){
    char* value = std::getenv(varName.c_str());
    if(value == nullptr){
        throw std::runtime_error("Environment variable " + varName + " not found.");
    }
    return std::string(value);
}

json::value fetchGitHubAPI(const std::string& apiKey, const std::string& endpoint) {
    // Create an I/O context
    asio::io_context ioc;

    // Create a resolver and a socket
    tcp::resolver resolver(ioc);
    ssl::context ctx(ssl::context::tlsv12_client);
    ssl::stream<tcp::socket> stream(ioc, ctx);

    // Resolve the host
    auto const results = resolver.resolve("api.github.com", "443");

    // Connect to the host
    asio::connect(stream.next_layer(), results.begin(), results.end());

    // Perform the SSL handshake
    stream.handshake(ssl::stream_base::client);

    // Create an HTTP GET request
    http::request<http::string_body> req{http::verb::get, endpoint, 11};
    req.set(http::field::host, "api.github.com");
    req.set(http::field::user_agent, "Boost.Beast GitHub API Client");
    req.set(http::field::authorization, "token " + apiKey);

    // Send the HTTP request
    http::write(stream, req);

    // Receive the HTTP response
    beast::flat_buffer buffer;
    http::response<http::string_body> res;
    http::read(stream, buffer, res);

    // Parse the JSON response
    json::value jv = json::parse(res.body());

    // Gracefully shut down TLS
    beast::error_code ec;
    stream.shutdown(ec);
    if (ec == asio::ssl::error::stream_truncated) {
        ec = {};
    }
    if (ec == asio::error::eof) {
        ec = {};
    }
    if (ec) {
        throw beast::system_error(ec);
    }

    return jv;
}

void connect_with_kdb_port()
{

    int handle = khpun("127.0.0.1", 5001, "jahz:password123", 2000); // Connect to kdb+ on localhost:5001

    if (handle < 0) {
        std::cerr << "Failed to connect to kdb+ on port 5001" << std::endl;
        return;
    }
    else if (handle > 0) {
        std::cout << "Successfully connected to kdb+ on port 5001" << std::endl;
    }
    else {
        std::cerr << "Connection to kdb+ on port 5001 returned unexpected value: " << handle << std::endl;
        return;
    }
    kclose(handle); // Close the connection when done
    return;
}

int main()
{
    try
    {
        // //get apikey or generate one from user 
        // const std::string apiKey = getEnvVar("GH_TOKEN");
        // //printf("API Key: %s\n", apiKey.c_str());
        // json::value response = fetchGitHubAPI(apiKey, "/user");
        // // Process the response as needed
        // std::cout << "Authenticated user info: " << response.as_object().at("login").as_string() << std::endl;
        // std::cout << "Public Repos: " << response.as_object().at("public_repos").as_int64() << std::endl;
    
        // Connect to kdb+ on port 5001
        // connect_with_kdb_port();

        //Try Connect to Binance WebSocket Stream 
        // ConnectorClient wsClient("stream.binance.com", "9443", "/ws/btcusdt@depth20");
        // wsClient.create_binance_connection();
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error attemptying to connect: " << e.what() << std::endl;
    }
}
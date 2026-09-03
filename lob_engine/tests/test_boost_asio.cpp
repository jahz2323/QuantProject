/**
 * The following example demonstrates how to:

    Resolve a hostname to an IP address (DNS)

    Establish a TCP connection

    Establish a TLS/SSL encrypted connection (HTTPS)

    Send an HTTP GET request

    Receive an HTTP response

    Parse JSON using Boost.JSON

    Extract an exchange rate


 * 
 */
#include <boost/asio/ssl.hpp>
#include <boost/beast.hpp>
#include <boost/json.hpp>
#include <iostream>

// Common namespace aliases used in Beast examples.
namespace asio = boost::asio;
namespace ssl = asio::ssl;
namespace beast = boost::beast;
namespace http = beast::http;
namespace json = boost::json;

// Convenience alias for TCP networking.
using tcp = asio::ip::tcp;

int main()
{
    try
    {
        //------------------------------------------------------------------
        // Step 1: Define the currency conversion we want.
        //------------------------------------------------------------------

        std::string from = "EUR";
        std::string to = "GBP";

        double amount = 100.0;

        //------------------------------------------------------------------
        // Step 2: Define the remote web server.
        //
        // host:
        //     DNS name of the server.
        // port:
        //     443 is the standard HTTPS port.
        //------------------------------------------------------------------

        std::string host = "api.frankfurter.dev";
        std::string port = "443";

        //------------------------------------------------------------------
        // Build the HTTP URL path.
        //
        // Result:
        //     /v1/latest?base=<from>&symbols=<to>
        //------------------------------------------------------------------

        std::string target =
            "/v1/latest?base=" + from +
            "&symbols=" + to;

        //------------------------------------------------------------------
        // Step 3: Create the Asio I/O context.
        //
        // The io_context owns the operating system networking resources.
        // Almost every Asio program starts by creating one.
        //------------------------------------------------------------------

        asio::io_context ioc;

        //------------------------------------------------------------------
        // Step 4: Create an SSL/TLS context.
        //
        // This object stores TLS configuration:
        //     - certificates
        //     - trusted certificate authorities
        //     - protocol versions
        //     - verification settings
        //------------------------------------------------------------------

        ssl::context ctx(ssl::context::tls_client);

        //------------------------------------------------------------------
        // Load system default certificate locations.
        //
        // On Windows this often isn't sufficient by itself,
        // but it doesn't hurt to call it.
        //------------------------------------------------------------------

        ctx.set_default_verify_paths();

        //------------------------------------------------------------------
        // Step 5: Create a DNS resolver.
        //
        // DNS converts:
        //     api.frankfurter.dev
        // into one or more IP addresses.
        //------------------------------------------------------------------

        tcp::resolver resolver(ioc);

        //------------------------------------------------------------------
        // Step 6: Create the HTTPS stream.
        //
        // Layer stack:
        //     ssl::stream
        //         ↓
        //     beast::tcp_stream
        //         ↓
        //       socket
        //
        // The TCP stream performs network I/O. The SSL stream adds encryption.
        //------------------------------------------------------------------

        ssl::stream<beast::tcp_stream> stream(ioc, ctx);

        //------------------------------------------------------------------
        // Step 7: Load trusted certificate authorities.
        //
        // This file contains a list of trusted root certificates.
        //
        // During the TLS handshake the server presents a certificate.
        // OpenSSL verifies that the certificate chains back to one
        // of these trusted authorities.
        //------------------------------------------------------------------

        // ctx.load_verify_file(
        //     "<YOUR PATH>\\cacert.pem");

        //------------------------------------------------------------------
        // Require certificate verification.
        //
        // Production code should verify certificates.
        //------------------------------------------------------------------

        stream.set_verify_mode(ssl::verify_peer);

        //------------------------------------------------------------------
        // Step 8: DNS resolution.
        //
        // Example:
        //     api.frankfurter.dev
        //
        // might resolve to:
        //     104.21.xx.xx
        //     172.67.xx.xx
        //------------------------------------------------------------------

        auto endpoints = resolver.resolve(host, port);

        //------------------------------------------------------------------
        // Step 9: Establish the TCP connection.
        //
        // At this point:
        //     Client <----TCP----> Server
        //
        // There is NOT yet any encryption.
        //------------------------------------------------------------------

        beast::get_lowest_layer(stream).connect(endpoints);

        //------------------------------------------------------------------
        // Step 10: Configure SNI (Server Name Indication).
        //
        // Many HTTPS servers host multiple websites on the same IP.
        // SNI tells the server which hostname we want.
        // Modern HTTPS connections often fail without this.
        //------------------------------------------------------------------

        if (!SSL_set_tlsext_host_name(
            stream.native_handle(),
            host.c_str()))
        {
            throw beast::system_error(
                beast::error_code(
                    static_cast<int>(::ERR_get_error()),
                    asio::error::get_ssl_category()));
        }

        //------------------------------------------------------------------
        // Step 11: Perform TLS handshake.
        //
        // During this phase:
        //     - encryption algorithms are negotiated
        //     - certificates are exchanged
        //     - certificates are verified
        //     - session keys are created
        //
        // After this succeeds, all traffic is encrypted.
        //------------------------------------------------------------------

        stream.handshake(ssl::stream_base::client);

        //------------------------------------------------------------------
        // Step 12: Build an HTTP GET request.
        //
        // HTTP version 11 = HTTP/1.1
        //------------------------------------------------------------------

        http::request<http::empty_body> req{
            http::verb::get,
            target,
            11
        };

        req.set(http::field::host, host);

        req.set(http::field::user_agent,
            "Boost.Beast Exchange Demo");

        //------------------------------------------------------------------
        // Step 13: Send the HTTP request.
        //
        // Browser equivalent:
        //     GET /v1/latest?... HTTP/1.1
        //     Host: api.frankfurter.dev
        //------------------------------------------------------------------

        http::write(stream, req);

        //------------------------------------------------------------------
        // Step 14: Receive the HTTP response.
        //------------------------------------------------------------------

        beast::flat_buffer buffer;

        http::response<http::string_body> res;

        http::read(stream, buffer, res);

        std::cout
            << "HTTP Status: "
            << res.result_int()
            << "\n\n";

        //------------------------------------------------------------------
        // Step 15: Parse JSON.
        //
        // Example response:
        // {
        //   "rates":
        //   {
        //      "EUR": 0.87
        //   }
        // }
        //------------------------------------------------------------------

        json::value jv = json::parse(res.body());

        auto const& obj = jv.as_object();

        //------------------------------------------------------------------
        // Extract:
        //
        //     rates -> EUR
        //------------------------------------------------------------------

        double rate =
            obj.at("rates")
            .as_object()
            .at(to)
            .as_double();

        //------------------------------------------------------------------
        // Convert the amount.
        //------------------------------------------------------------------

        double converted = amount * rate;

        std::cout
            << amount << ' '
            << from
            << " = "
            << converted << ' '
            << to
            << "\n";

        //------------------------------------------------------------------
        // Step 16: Gracefully shut down TLS.
        //
        // Some servers close the connection without sending the
        // TLS close_notify message. OpenSSL reports this as stream_truncated.
        //
        // For simple HTTPS clients this is commonly ignored.
        //------------------------------------------------------------------

        beast::error_code ec;

        stream.shutdown(ec);

        if (ec == asio::ssl::error::stream_truncated)
        {
            ec = {};
        }

        if (ec == asio::error::eof)
        {
            ec = {};
        }

        if (ec)
        {
            throw beast::system_error(ec);
        }
    }
    catch (std::exception const& e)
    {
        std::cerr
            << "Error: "
            << e.what()
            << "\n";
    }
}
#include <boost/asio/ssl.hpp>
#include <boost/beast.hpp>
#include <boost/json.hpp>
#include <iostream>

//pybind11 includes
#include <pybind11/pybind11.h>

// lob_engine includes
#include "Connector.hpp"

// Common namespace aliases used in Beast examples.
namespace asio = boost::asio;
namespace ssl = asio::ssl;
namespace beast = boost::beast;
namespace http = beast::http;
namespace json = boost::json;

// Convenience alias for TCP networking.
using tcp = asio::ip::tcp;

namespace py = pybind11;

// PYBIND11_MODULE(lob_pybind11, m)
// {
//     m.doc() = "Pybind11, Initate WebSocket Connection Example";

//     py::class_<ConnectorClient>(m, "ConnectorClient")
//         .def(py::init<const std::string&, const std::string&, const std::string&>())
//         .def("create_connection", &ConnectorClient::create_binance_connection);
// }
#ifndef ApiService_h
#define ApiService_h

#include <cpprest/http_client.h>
#include <cpprest/filestream.h>
#include <string>
#include <unordered_map>

using namespace utility;
using namespace web;
using namespace web::http;
using namespace web::http::client;
using namespace concurrency::streams;
class ApiService
{
    std::string baseUrl;
public:
    ApiService(std::string);
    pplx::task<http_response> request(method, std::string, std::unordered_map<std::string, std::string>, bool);
};

#endif
#ifndef ApiService_h
#define ApiService_h

#include <cpprest/http_client.h>
#include <cpprest/filestream.h>
#include <string>
#include <vector>
#include <utility>

using namespace utility;
using namespace web;
using namespace web::http;
using namespace web::http::client;
using namespace concurrency::streams;
class ApiService
{
    std::string baseUrl, apiKey;

public:
    ApiService(std::string);
    void setApiKey(std::string);
    pplx::task<http_response> request(method, std::string,
                                      std::vector<std::pair<std::string, std::string>> &,
                                      bool isSaveAsJson = false, std::string fileName = "");
    void getQueryString(std::vector<std::pair<std::string, std::string>> &, std::string &);
};

#endif
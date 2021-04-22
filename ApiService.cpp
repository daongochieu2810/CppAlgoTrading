#include "ApiService.h"
#include <iostream>

ApiService::ApiService(std::string _baseUrl)
{
    baseUrl = _baseUrl;
}

void ApiService::setApiKey(std::string _apiKey)
{
    apiKey = _apiKey;
}

void ApiService::getQueryString(std::unordered_map<std::string, std::string> &params, std::string &copy)
{
    uri_builder builder;

    for (auto param : params)
    {
        builder.append_query(U(param.first), U(param.second));
    }

    copy = builder.query();
}

pplx::task<http_response> ApiService::request(method _method,
                                              std::string url,
                                              std::unordered_map<std::string, std::string> &params,
                                              bool isSaveAsJson,
                                              std::string fileName)
{
    pplx::task<http_response> response;
    http_client client(U(baseUrl));
    http_request request(_method);
    uri_builder builder(U(url));

    for (auto param : params)
    {
        builder.append_query(U(param.first), U(param.second));
    }

    request.set_request_uri(builder.to_string());
    request.headers().add("X-MBX-APIKEY", apiKey);
    response = client.request(request);

    if (!isSaveAsJson)
    {
        return response;
    }

    auto fileStream = std::make_shared<ostream>();

    pplx::task<void> requestTask =
        fstream::open_ostream(U(fileName))
            .then([=](ostream outFile) {
                *fileStream = outFile;
                return response;
            })
            .then([=](http_response response) {
                return response.body().read_to_end(fileStream->streambuf());
            })
            .then([=](size_t) {
                fileStream->close();
            });

    try
    {
        requestTask.wait();
    }
    catch (const std::exception &e)
    {
        printf("Error exception:%s\n", e.what());
    }

    return response;
}

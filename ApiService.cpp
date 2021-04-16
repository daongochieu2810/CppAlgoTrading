#include "ApiService.h"

ApiService::ApiService(std::string _baseUrl)
{
    baseUrl = _baseUrl;
}

pplx::task<http_response> ApiService::request(method _method,
                                              std::string url, std::unordered_map<std::string, std::string> params)
{
    http_client client(U(baseUrl));
    uri_builder builder(U(url));
    
    for (auto param : params)
    {
        builder.append_query(U(param.first), U(param.second));
    }
    return client.request(_method, builder.to_string());
}
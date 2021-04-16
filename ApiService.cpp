#include "ApiService.h"

ApiService::ApiService(std::string _baseUrl)
{
    baseUrl = _baseUrl;
}

pplx::task<http_response> ApiService::request(method _method,
                                              std::string url, 
                                              std::unordered_map<std::string, std::string> params,
                                              bool isSaveAsJson)
{
    pplx::task<http_response> response;
    http_client client(U(baseUrl));
    uri_builder builder(U(url));
    
    for (auto param : params)
    {
        builder.append_query(U(param.first), U(param.second));
    }
    response = client.request(_method, builder.to_string()); 
    if (!isSaveAsJson) {
        return response;
    }

    auto fileStream = std::make_shared<ostream>();

    pplx::task<void> requestTask = fstream::open_ostream(U("exchange_info.json")).then([=](ostream outFile) {
        *fileStream = outFile;
        return request(methods::GET, "/api/v3/exchangeInfo", params);
    }).then([=](http_response response){
        return response.body().read_to_end(fileStream->streambuf());
    }).then([=](size_t) {
        fileStream->close();
    });

    try {
        requestTask.wait();
    } catch (const std::exception &e) {
        printf("Error exception:%s\n", e.what());
    }

    return response;
}

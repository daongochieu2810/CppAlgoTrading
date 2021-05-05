#pragma once

#include <cpprest/json.h>

web::json::value readJsonFile(std::string const &jsonFileName)
{
    web::json::value output;

    try
    {
        std::ifstream f(jsonFileName);

        std::stringstream strStream;
        strStream << f.rdbuf();
        f.close();

        output = web::json::value::parse(strStream);
    }
    catch (web::json::json_exception excep)
    {
        throw web::json::json_exception("Error Parsing JSON file " + jsonFileName);
    }

    return output;
}

void getTime(long &time)
{
    struct timeval tp;
    gettimeofday(&tp, NULL);
    time = (long)tp.tv_sec * 1000L + tp.tv_usec / 1000;
}
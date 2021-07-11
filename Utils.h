#pragma once

#include <cpprest/json.h>
#include <stdlib.h>

using namespace std::chrono;

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

inline auto binary_to_hex_digit(unsigned a) -> char
{
    return a + (a < 10 ? '0' : 'a' - 10);
}

auto binary_to_hex(unsigned char const *binary, unsigned binary_len) -> std::string
{
    std::string r(binary_len * 2, '\0');
    for (unsigned i = 0; i < binary_len; ++i)
    {
        r[i * 2] = binary_to_hex_digit(binary[i] >> 4);
        r[i * 2 + 1] = binary_to_hex_digit(binary[i] & 15);
    }
    return r;
}

void benchmarkPerformance(void fnc())
{
    auto t1 = high_resolution_clock::now();
    fnc();
    auto t2 = high_resolution_clock::now();
    duration<double, std::milli> ms_double = t2 - t1;
    std::cout << ms_double.count() << "ms\n";
}

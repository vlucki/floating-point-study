#ifndef LOGGER_H
#define LOGGER_H

#include <iostream>

#define print(x) std::cout << x << std::endl;

#define print_log(verbosity, message) print("[" << verbosity << "](" << __FILE__ << ":" << __LINE__ << ") " << message)

#define LOG_LEVEL_ERROR     0
#define LOG_LEVEL_WARN      LOG_LEVEL_ERROR + 1
#define LOG_LEVEL_INFO      LOG_LEVEL_WARN  + 1
#define LOG_LEVEL_VERBOSE   LOG_LEVEL_INFO  + 1

#ifndef LOG_LEVEL
#define LOG_LEVEL LOG_LEVEL_VERBOSE
#endif

#if LOG_LEVEL >= LOG_LEVEL_VERBOSE
#define log_v(x) print_log("VERBOSE", x)
#define log_v_silent(x) print(x)
#else
#define log_v(x)
#define log_v_silent(x)
#endif

#if LOG_LEVEL >= LOG_LEVEL_INFO
#define log_i(x) print_log("INFO", x)
#define log_i_silent(x) print(x)
#else
#define log_i(x)
#define log_i_silent(x)
#endif

#if LOG_LEVEL >= LOG_LEVEL_WARN
#define log_w(x) print_log("WARN", x)
#define log_w_silent(x) print(x)
#else
#define log_w(x)
#define log_w_silent(x)
#endif

#if LOG_LEVEL >= LOG_LEVEL_ERROR
#define log_e(x) print_log("ERROR", x)
#define log_e_silent(x) print(x)
#else
#define log_e(x)
#define log_e_silent(x)
#endif

template<typename T>
std::string GetBitString(const T& bitPattern, bool skipZeroes = false)
{
    std::string result {};
    for(int i = 8 * sizeof(T) - 1; i >= 0; --i)
    {
        const uint bit = (bitPattern & (1 << i));
        if(!bit && skipZeroes) // skip all left zeroes
        {
            continue;
        }
        skipZeroes = false;
        result += (bit ? "1" : "0");
        if(i != 0 && i % 4 == 0)
            result += " ";
    }

    return result;
}
#endif /* LOGGER_H */

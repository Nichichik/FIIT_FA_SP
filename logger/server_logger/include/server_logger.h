#ifndef MATH_PRACTICE_AND_OPERATING_SYSTEMS_SERVER_LOGGER_H
#define MATH_PRACTICE_AND_OPERATING_SYSTEMS_SERVER_LOGGER_H

#include <logger.h>
#include <unordered_map>
// #include <httplib.h>

class server_logger_builder;
class server_logger final:
    public logger
{
    std::string _destination;
    // httplib::Client _client;
    std::mutex _mut;

    std::unordered_map<logger::severity ,std::pair<std::string, bool>> _streams;

    server_logger(const std::string& dest, const std::unordered_map<logger::severity ,std::pair<std::string, bool>>& streams);

    friend server_logger_builder;

    static int inner_getpid();

private:
    std::string _format;

public:

    server_logger(server_logger const &other);

    server_logger &operator=(server_logger const &other);

    server_logger(server_logger &&other) noexcept;

    server_logger &operator=(server_logger &&other) noexcept;

    ~server_logger() noexcept final;

    enum class flag
    { DATE, TIME, SEVERITY, MESSAGE, NO_FLAG };

private:
    void send_to_server(const std::string& message) const;
    void send_streams_to_server() const;
    [[nodiscard]] std::string make_format(const std::string& message, severity sev) const;
    static flag char_to_flag(char c) noexcept;
    uint16_t _port = 9200;

public:

    [[nodiscard]] logger& log(
        const std::string &message,
        logger::severity severity) & override;

};

#endif //MATH_PRACTICE_AND_OPERATING_SYSTEMS_SERVER_LOGGER_H

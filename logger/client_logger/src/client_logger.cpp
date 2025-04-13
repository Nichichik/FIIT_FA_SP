#include <string>
#include <sstream>
#include <algorithm>
#include <utility>
#include "../include/client_logger.h"
#include <not_implemented.h>

std::unordered_map<std::string, std::pair<size_t, std::ofstream>> client_logger::refcounted_stream::_global_streams;


logger& client_logger::log(
    const std::string &text,
    logger::severity severity) &
{
    auto it = _output_streams.find(severity);
    if (it == _output_streams.end())
    {
        return *this;
    }

    std::string formatted = make_format(text, severity);
    if (it->second.second) {
        std::cout << formatted << std::endl;
    }
    auto& stream_list = it->second.first;
    for (auto& stream : stream_list)
    {
        if (stream._stream.second && stream._stream.second->is_open())
        {
            *(stream._stream.second) << formatted << std::endl;
        }
    }
    return *this;
}


std::string client_logger::make_format(const std::string &message, severity sev) const
{
    std::string result;
    for (size_t i = 0; i < _format.size(); i++)
    {
        if (_format[i] == '%' && i + 1 < _format.size())
        {
            switch (char_to_flag(_format[i+1]))
            {
            case flag::DATE:
                result += logger::current_date_to_string();
                break;
            case flag::TIME:
                result += logger::current_time_to_string();
                break;
            case flag::SEVERITY:
                result += logger::severity_to_string(sev);
                break;
            case flag::MESSAGE:
                result += message;
                break;
            default:
                result += _format[i];
                result += _format[i+1];
            }
            i++;
        }
        else
        {
            result += _format[i];
        }
    }
    return result;
}

client_logger::client_logger(
        const std::unordered_map<logger::severity, std::pair<std::forward_list<refcounted_stream>, bool>> &streams,
        std::string format)
{
    _output_streams = streams;
    _format = format;
    for (auto& pair : _output_streams)
    {
        auto& stream_list = pair.second.first;
        for (auto& stream : stream_list)
        {
            stream.open();
        }
    }
}

client_logger::flag client_logger::char_to_flag(char c) noexcept
{
    switch (c) {
        case 'd':
            return flag::DATE;
        case 't':
            return flag::TIME;
        case 's':
            return flag::SEVERITY;
        case 'm':
            return flag::MESSAGE;
        default:
            return flag::NO_FLAG;
    }
}

client_logger::client_logger(const client_logger &other)
{
    _output_streams = other._output_streams;
    _format = other._format;
}

client_logger &client_logger::operator=(const client_logger &other)
{
    if (this != &other)
    {
        _output_streams = other._output_streams;
        _format = other._format;
    }
    return *this;
}

client_logger::client_logger(client_logger &&other) noexcept
{
    _output_streams= std::move(other._output_streams);
    _format = std::move(other._format);
    other._output_streams.clear();
    other._format.clear();
}

client_logger &client_logger::operator=(client_logger &&other) noexcept
{
    if (this != &other)
    {
        _output_streams = std::move(other._output_streams);
        _format = std::move(other._format);
        other._output_streams.clear();
        other._format.clear();
    }
    return *this;
}

client_logger::~client_logger() noexcept
{
    for (auto& pair : _output_streams)
    {
        auto& stream_list = pair.second.first;
        stream_list.clear();
    }
}

client_logger::refcounted_stream::refcounted_stream(const std::string &path)
{
    _stream.first = path;
    _stream.second = nullptr;
}

client_logger::refcounted_stream::refcounted_stream(const client_logger::refcounted_stream &oth)
{
    _stream.first = oth._stream.first;
    _stream.second = nullptr;
    open();
}

client_logger::refcounted_stream &
client_logger::refcounted_stream::operator=(const client_logger::refcounted_stream &oth)
{
    if (this != &oth)
    {
        this->~refcounted_stream();
        _stream.first = oth._stream.first;
        _stream.second = nullptr;
        open();
    }
    return *this;
}

client_logger::refcounted_stream::refcounted_stream(client_logger::refcounted_stream &&oth) noexcept
{
    _stream = std::move(oth._stream);
    oth._stream.second = nullptr;
}

client_logger::refcounted_stream &client_logger::refcounted_stream::operator=(client_logger::refcounted_stream &&oth) noexcept
{
    if (this != &oth)
    {
        this->~refcounted_stream();
        _stream = std::move(oth._stream);
        oth._stream.second = nullptr;
    }
    return *this;
}

#include <filesystem>

void client_logger::refcounted_stream::open()
{
    if (_stream.second != nullptr) {
        return;
    }

    auto& global_entry = _global_streams[_stream.first];
    std::filesystem::path log_path(_stream.first);
    std::filesystem::path dir = log_path.parent_path();
    if (!dir.empty() && !std::filesystem::exists(dir)) {
        std::filesystem::create_directories(dir);
    }

    if (!global_entry.second.is_open()) {
        std::ofstream file_stream(_stream.first, std::ios::app);
        if (file_stream.is_open()) {
            global_entry.second = std::move(file_stream);
        } else {
            std::cerr << "Failed to open log file: " << _stream.first << std::endl;
            return;
        }
    }
    global_entry.first += 1;
    _stream.second = &global_entry.second;
}




client_logger::refcounted_stream::~refcounted_stream()
{
    if (_stream.second == nullptr)
    {
        return;
    }
    auto it = _global_streams.find(_stream.first);
    if (it != _global_streams.end())
    {
        if (--it->second.first == 0)
        {
            if (it->second.second.is_open())
            {
                it->second.second.close();
            }
            _global_streams.erase(it);
        }
    }
    _stream.second = nullptr;
}

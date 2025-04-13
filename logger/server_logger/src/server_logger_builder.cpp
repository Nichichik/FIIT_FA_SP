#include <not_implemented.h>
#include "../include/server_logger_builder.h"
#include <fstream>

logger_builder& server_logger_builder::add_file_stream(
    std::string const &stream_file_path,
    logger::severity severity) &
{
    if (_output_streams.contains(severity)) {
        _output_streams[severity].first = stream_file_path;
    } else {
        _output_streams[severity] = std::make_pair(stream_file_path, false);
    }
    return *this;
}

logger_builder& server_logger_builder::add_console_stream(
    logger::severity severity) &
{
    if (_output_streams.contains(severity)) {
        _output_streams[severity].second = true;
    } else {
        _output_streams[severity] = std::make_pair("", true);
    }
    return *this;
}

logger_builder& server_logger_builder::transform_with_configuration(
    std::string const &configuration_file_path,
    std::string const &configuration_path) &
{
    std::ifstream configuration_file(configuration_file_path);
    if (!configuration_file.is_open())
    {

    }
    nlohmann::json config;
    configuration_file >> config;
    auto node = config;
    for (const auto& key : configuration_path | std::views::split('/')) {
        std::string k(key.begin(), key.end());
        node = node.at(k);
    }

    if (node.contains("destination")) {
        _destination = node["destination"].get<std::string>();
    }

    if (node.contains("format")) {
        set_format(node["format"].get<std::string>());
    }

    if (node.contains("streams")) {
        for (auto& [severity_str, stream_info] : node["streams"].items()) {
            logger::severity severity;

            if (severity_str == "trace") {
                severity = logger::severity::trace;
            } else if (severity_str == "debug") {
                severity = logger::severity::debug;
            } else if (severity_str == "information" || severity_str == "info") {
                severity = logger::severity::information;
            } else if (severity_str == "warning") {
                severity = logger::severity::warning;
            } else if (severity_str == "error") {
                severity = logger::severity::error;
            } else if (severity_str == "critical") {
                severity = logger::severity::critical;
            } else {
                continue;
            }
            if (stream_info.contains("console") && stream_info["console"].get<bool>()) {
                add_console_stream(severity);
            } else if (stream_info.contains("file")) {
                add_file_stream(stream_info["file"].get<std::string>(), severity);
            }
        }
    }

    return *this;
}

logger_builder& server_logger_builder::clear() &
{
    _output_streams.clear();
    _destination = "127.0.0.1";
    _format.clear();
    return *this;
}

logger *server_logger_builder::build() const
{
    return new server_logger(_destination, _output_streams);
}

logger_builder& server_logger_builder::set_destination(const std::string& dest) &
{
    _destination = dest;
    return *this;
}

logger_builder& server_logger_builder::set_format(const std::string &format) &
{
    _format = format;
    return *this;
}

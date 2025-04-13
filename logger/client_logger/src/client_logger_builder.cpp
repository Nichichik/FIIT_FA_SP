#include <filesystem>
#include <logger.h>
#include <utility>
#include <set>
#include <not_implemented.h>
#include "../include/client_logger_builder.h"

using namespace nlohmann;

logger_builder& client_logger_builder::add_console_stream(logger::severity severity) & {
    _output_streams[severity].second = true;
    return *this;
}

logger_builder& client_logger_builder::add_file_stream(std::string const &stream_file_path, logger::severity severity) & {
    auto& stream_list = _output_streams[severity].first;
    for (const auto& stream : stream_list) {
        if (stream._stream.first == stream_file_path) {
            return *this;
        }
    }
    stream_list.push_front(client_logger::refcounted_stream{stream_file_path});
    return *this;
}


logger_builder& client_logger_builder::transform_with_configuration(
    const std::string& configuration_file_path,
    const std::string& configuration_path) &
{
    auto parse_severity = [](const std::string& s) -> logger::severity {
        if (s == "error")
            return logger::severity::error;
        if (s == "warning")
            return logger::severity::warning;
        if (s == "trace")
            return logger::severity::trace;
        if (s == "debug")
            return logger::severity::debug;
        if (s == "information")
            return logger::severity::information;
        if (s == "critical")
            return logger::severity::critical;
        std::cerr << "Unknown severity string: " << s << "\n";
    };

    nlohmann::json config_json;
    std::ifstream config_file(configuration_file_path);

    if (!config_file.is_open()) {
        std::cerr << "Failed to open config file: " << configuration_file_path << std::endl;
        return *this;
    }

    try {
        config_file >> config_json;
    } catch (const std::exception& e) {
        std::cerr << "Failed to parse config file: " << configuration_file_path
                  << "\nReason: " << e.what() << std::endl;
        return *this;
    }

    if (!config_json.contains(configuration_path)) {
        std::cerr << "Configuration path not found: " << configuration_path << std::endl;
        return *this;
    }

    for (const auto& entry : config_json[configuration_path]) {
        if (!entry.contains("severity") || !entry.contains("destination")) {
            std::cerr << "Invalid config entry: missing severity or destination." << std::endl;
            continue;
        }

        logger::severity sev = parse_severity(entry["severity"]);
        std::string dest = entry["destination"];

        if (dest == "console") {
            add_console_stream(sev);
        } else if (dest == "file" && entry.contains("file_path")) {
            add_file_stream(entry["file_path"], sev);
        } else {
            std::cerr << "Invalid destination or missing file_path in entry." << std::endl;
        }

        if (entry.contains("format")) {
            set_format(entry["format"]);
        }
    }

    return *this;
}


logger_builder& client_logger_builder::clear() &
{
    _output_streams.clear();
    return *this;
}

logger *client_logger_builder::build() const
{
    return new client_logger(_output_streams, _format);
}

logger_builder& client_logger_builder::set_format(const std::string &format) &
{
    _format = format;
    return *this;
}
void client_logger_builder::parse_severity(logger::severity sev, nlohmann::json& j)
{
    std::string severity_string;
    switch (sev)
    {
        case logger::severity::error:
            severity_string = "error";
            break;
        case logger::severity::warning:
            severity_string = "warning";
            break;
        case logger::severity::trace:
            severity_string = "trace";
            break;
        case logger::severity::debug:
            severity_string = "debug";
            break;
        case logger::severity::information:
            severity_string = "information";
            break;
        case logger::severity::critical:
            severity_string = "critical";
            break;
        default:
            severity_string = "unknown";
            break;
    }
    j["severity"] = severity_string;
}

logger_builder& client_logger_builder::set_destination(const std::string &format) &
{
    _format = format;
    return *this;
}

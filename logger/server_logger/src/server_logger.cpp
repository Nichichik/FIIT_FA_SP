#include <not_implemented.h>
#include <mutex>
// #include <httplib.h>
#include "../include/server_logger.h"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#define CLOSE_SOCKET closesocket
#include <process.h>
#else
#include <unistd.h>
#include <arpa/inet.h>
#define CLOSE_SOCKET close
#endif
#include <ctime>
#include <iomanip>
#include <sstream>
#include <iostream>
#include <fstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

server_logger::~server_logger() noexcept
{
}

std::string server_logger::make_format(const std::string& message, severity sev) const {
    if (_format.empty()) {
        // Если формат пуст, просто возвращаем сообщение как есть
        return message;
    }

    std::string result;
    for (size_t i = 0; i < _format.size(); i++) {
        if (_format[i] == '%' && i + 1 < _format.size()) {
            switch (char_to_flag(_format[i + 1])) {
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
                result += _format[i + 1];
            }
            i++;
        } else {
            result += _format[i];
        }
    }
    return result;
}


server_logger::flag server_logger::char_to_flag(char c) noexcept {
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

logger& server_logger::log(const std::string& text, logger::severity sev) & {
    std::string formatted = make_format(text, sev);
    int pid = inner_getpid();

    std::ostringstream message;
    message << static_cast<int>(sev) << "|" << pid << "|" << formatted;

    send_to_server(message.str());

    return *this;
}


void server_logger::send_to_server(const std::string& message) const {
#ifdef _WIN32
    // Инициализация WinSock
    WSADATA wsaData;
    int wsaInit = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (wsaInit != 0) {
        std::cerr << "[WSAStartup Error] Code: " << wsaInit << std::endl;
        return;
    }
#endif

    // Создание сокета
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        std::cerr << "[Client Error] Failed to create socket\n";
#ifdef _WIN32
        WSACleanup();
#endif
        return;
    }

    // Установка параметров сервера
    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(_port);
#ifdef _WIN32
    const char* wsl_ip = "172.20.253.139"; // IP адрес для WSL
    if (inet_pton(AF_INET, wsl_ip, &server_addr.sin_addr) <= 0) {
        std::cerr << "[Client Error] Invalid WSL IP address format\n";
        CLOSE_SOCKET(sock);
        return;
    }
#else
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr); // Для Linux (localhost)
#endif

    // Подключение к серверу
    if (connect(sock, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "[Client Error] Failed to connect to server\n";
        CLOSE_SOCKET(sock);
#ifdef _WIN32
        WSACleanup();
#endif
        return;
    }

    // Отправка сообщения на сервер
    send(sock, message.c_str(), static_cast<int>(message.size()), 0);

    // Закрытие сокета
    CLOSE_SOCKET(sock);

#ifdef _WIN32
    WSACleanup();
#endif
}

server_logger::server_logger(const std::string& dest,
                             const std::unordered_map<logger::severity, std::pair<std::string, bool>>& streams) {
    _destination = dest;
    _streams = streams;
    _port = 9200; // Порт для подключения

    int pid = inner_getpid(); // Получаем PID процесса
    std::ostringstream oss;
    oss << pid << "|CONFIG|"; // Начало строки конфигурации

    bool first = true;
    for (const auto& [sev, pair] : streams) {
        if (!first) oss << ";";  // Разделитель для нескольких потоков
        first = false;

        // Каждая конфигурация потока: severity:path:is_file
        oss << static_cast<int>(sev) << ":" << pair.first << ":" << (pair.second ? "1" : "0");
    }

    // Отправляем конфигурацию на сервер
    send_to_server(oss.str());
}


int server_logger::inner_getpid()
{
#ifdef _WIN32
    return ::_getpid();
#elif
    return getpid();
#endif
}

server_logger::server_logger(const server_logger &other)
{
    _destination = other._destination;
    _streams = other._streams;
    _format = other._format;
}

server_logger &server_logger::operator=(const server_logger &other)
{
    if (this != &other) {
        _destination = other._destination;
        _streams = other._streams;
        _format = other._format;
    }
    return *this;
}

server_logger::server_logger(server_logger &&other) noexcept
{
    _destination = std::move(other._destination);
    _streams = std::move(other._streams);
    _format = std::move(other._format);
    other._destination.clear();
    other._streams.clear();
    other._format.clear();
}

server_logger &server_logger::operator=(server_logger &&other) noexcept
{
    if (this != &other) {
        _destination = std::move(other._destination);
        _streams = std::move(other._streams);
        _format = std::move(other._format);
        other._destination.clear();
        other._streams.clear();
        other._format.clear();
    }
    return *this;
}

#include "server.h"
#include <mutex>
#include <logger_builder.h>
#include <fstream>
#include <iostream>
#include <sstream>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#define CLOSE_SOCKET closesocket
#else
#include <unistd.h>
#include <arpa/inet.h>
#define CLOSE_SOCKET close
#endif

// Конструктор сервера, инициализация порта и сокета
server::server(uint16_t port) : _port(port) {
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
    _server_socket = socket(AF_INET, SOCK_STREAM, 0);
#else
    _server_socket = socket(AF_INET, SOCK_STREAM, 0);
#endif

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(_port);

    bind(_server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr));
    listen(_server_socket, 5);  // Ожидаем до 5 соединений в очереди
}


void server::handle_client(int client_socket) {
    char buffer[2048];
    int bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received <= 0) {
        CLOSE_SOCKET(client_socket);
        return;
    }

    buffer[bytes_received] = '\0';
    std::string received(buffer);
    std::istringstream iss(received);

    std::string part1, part2;
    std::getline(iss, part1, '|');
    std::getline(iss, part2, '|');

    if (part2 == "CONFIG") {
        int pid = std::stoi(part1);
        std::string config_data;
        std::getline(iss, config_data);

        std::unordered_map<logger::severity, std::pair<std::string, bool>> pid_streams;
        std::istringstream config_stream(config_data);
        std::string entry;
        while (std::getline(config_stream, entry, ';')) {
            if (entry.empty()) continue;

            std::istringstream entry_stream(entry);
            std::string sev_str, path, is_file_str;

            std::getline(entry_stream, sev_str, ':');
            std::getline(entry_stream, path, ':');
            std::getline(entry_stream, is_file_str, ':');

            logger::severity sev = static_cast<logger::severity>(std::stoi(sev_str));
            bool is_file = (is_file_str == "1");

            pid_streams[sev] = { path, is_file };
        }

        std::unique_lock lock(_mut);
        _streams[pid] = std::move(pid_streams);
        std::cout << "[Server] Config received from PID " << pid << "\n";
        std::cout << "==== Server stream config ====\n";

        for (const auto& [pid, severity_map] : _streams) {
            std::cout << "PID " << pid << ":\n";
            for (const auto& [sev, pair] : severity_map) {
                std::string sev_str = std::to_string(static_cast<int>(sev));
                std::cout << "  Severity " << sev_str
                          << " -> Path: \"" << pair.first << "\", "
                          << "Console: " << (pair.second ? "Yes" : "No") << "\n";
            }
        }

        std::cout << "==============================" << std::endl;
        CLOSE_SOCKET(client_socket);
        return;
    }
    std::string severity_str = part1;
    std::string pid_str = part2;
    std::string message;
    std::getline(iss, message);

    int severity_val = std::stoi(severity_str);
    logger::severity sev = static_cast<logger::severity>(severity_val);
    int pid = std::stoi(pid_str);
    std::string full_message = "[PID " + pid_str + "] " + message;

    std::shared_lock lock(_mut);
    auto pid_it = _streams.find(pid);
    if (pid_it != _streams.end()) {
        const auto& severity_map = pid_it->second;
        auto sev_it = severity_map.find(sev);
        if (sev_it != severity_map.end()) {
            const auto& [path, is_file] = sev_it->second;
            if (!path.empty()) {
                std::ofstream out(path, std::ios::app);
                if (out.is_open()) {
                    out << full_message << '\n';
                    std::cout << "[Server] Message written to file: " << path << std::endl;
                } else {
                    std::cerr << "[Server] Cannot open log file: " << path << "\n";
                }
            }
            if (is_file) {
                std::cout << full_message << std::endl;
            }

        } else {
            std::cerr << "[Server] No stream for severity " << severity_val << " (PID " << pid << ")\n";
        }
    } else {
        std::cerr << "[Server] No stream config for PID: " << pid << "\n";
    }

    CLOSE_SOCKET(client_socket);
}


// Метод для запуска сервера
void server::start() {
    while (true) {
        int client_socket = accept(_server_socket, nullptr, nullptr);  // Принимаем соединение
        if (client_socket < 0) {
            std::cerr << "[Server] Error accepting connection" << std::endl;
            continue;
        }

        // Создаем новый поток для обработки клиента
        std::thread client_thread(&server::handle_client, this, client_socket);
        client_thread.detach();  // Отсоединяем поток, чтобы он мог работать самостоятельно
    }
}

// Метод для очистки конфигурации потоков
void server::clear() {
    std::unique_lock lock(_mut);  // Блокируем мьютекс для эксклюзивного доступа
    _streams.clear();  // Очищаем все потоки вывода
    std::cout << "[Server] All stream configurations have been cleared." << std::endl;
}
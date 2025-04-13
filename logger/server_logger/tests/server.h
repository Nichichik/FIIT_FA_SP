#ifndef MP_OS_SERVER_H
#define MP_OS_SERVER_H

#include <unordered_map>
#include <logger.h>
#include <string>
#include <thread>
#include <iostream>
#include <shared_mutex>

#ifdef _WIN32
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#endif

class server
{
private:
    uint16_t _port;  // Порт для сервера
    std::unordered_map<int, std::unordered_map<logger::severity, std::pair<std::string, bool>>> _streams;
    std::shared_mutex _mut;  // Мьютекс для синхронизации доступа к _streams

#ifdef _WIN32
    SOCKET _server_socket;  // Сокет для Windows
#else
    int _server_socket;  // Сокет для Unix-подобных систем
#endif

public:
    explicit server(uint16_t port = 9200);  // Конструктор
    void handle_client(int client_socket);  // Метод для обработки каждого клиента
    void start();  // Метод для старта сервера

    void clear();  // Метод для очистки конфигурации потоков

    ~server() noexcept = default;  // Деструктор
};

#endif //MP_OS_SERVER_H


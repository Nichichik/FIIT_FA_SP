//
// Created by Des Caldnd on 3/27/2024.
//
#include "server.h"

int main(int argc, char* argv[])
{
    server s(9200);
    s.start();
    std::this_thread::sleep_for(std::chrono::minutes(2));
    s.clear();
    return 0;
}
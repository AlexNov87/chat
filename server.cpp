#include "srv.h"



int main()
{
    setlocale(LC_ALL, "Ru-ru");
    const unsigned num_threads = std::thread::hardware_concurrency();
    net::io_context ioc(num_threads);
    MainServer server(ioc);
    server.Listen();
    Service::MtreadRunContext(ioc);
}

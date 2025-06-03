#include "srv.h"



int main()
{
    const unsigned num_threads = std::thread::hardware_concurrency();
    net::io_context ioc(num_threads);
    MainServer server(ioc);
    server.Listen();
    Service::MtreadRunContext(ioc);
}

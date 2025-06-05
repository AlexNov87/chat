#include "srv.h"
void MainServer::ServerSession::HandleSession()
{
    if (!Service::IsAliveSocket(*socket_))
    {
        Service::ShutDownSocket(*socket_);
        return;
    }
    auto self = this->shared_from_this();
    net::async_read_until(*socket_, readbuf_, '\n', [self](err ec, size_t bytes)
                          {
                    
                    if(!ec){
                    
                    auto obj = Service::ExtractObjectsfromBuffer(self->readbuf_, bytes);
                    Service::PrintUmap(obj);
                    self->readbuf_.consume(bytes);
                    
                    net::async_write(*(self->socket_), 
                    net::buffer(ServiceChatroomServer::MakeAnswerError("TESTR"," TESTR")),[self](err ec, size_t bytes){
                        if(!ec){
                         self->HandleSession();
                        }
                    });
                    
                    }});

    return;
};

/*

    // auto tasks = Service::DoubleGuardedExcept<std::vector<task>>([self]()
    // { return Service::ExtractObjectsfromSocket(*(self->socket_)); }, "HandleSession");
    // condition = false;
*/

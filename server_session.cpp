#include "srv.h"
void AbstractSession::HandleSession()
{
    if (!Service::IsAliveSocket(*socket_))
    {
        Service::ShutDownSocket(*socket_);
        return;
    }
    auto self = this->shared_from_this();
    net::async_read_until(*socket_, readbuf_, CONSTANTS::SERIAL_SYM, [self](err ec, size_t bytes)
                          {
                    
                    if(!ec){
                    
                    auto obj = Service::ExtractSharedObjectsfromBuffer(self->readbuf_, bytes);
                    std::string responce = self->GetStringResponceToSocket(obj);
                    self->readbuf_.consume(bytes);
                    
                    Service::PrintUmap(*obj);
                    net::async_write(*(self->socket_), 
                    net::buffer(responce),[self](err ec, size_t bytes){
                        if(!ec){
                          net::dispatch(*self->strand_,[self]{ self->HandleSession();});
                        }
                    });}

                    });

    return;
};

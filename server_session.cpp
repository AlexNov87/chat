#include "srv.h"
void MainServer::ServerSession::HandleSession()
{
    if (!Service::IsAliveSocket(*socket_))
    {
        Service::ShutDownSocket(*socket_);
        return;
    }

    std::atomic_bool condition = true;
    net::steady_timer timer(server_->ioc_);
    auto self = this->shared_from_this();
    timer.async_wait([self, &condition](const boost::system::error_code &ec)
                     {
         if(condition){return;}
         std::cout << "Timer expired!" << std::endl;
         Service::ShutDownSocket(*(self->socket_)); });
    timer.expires_after(std::chrono::milliseconds(300));

    net::async_read_until(*socket_, readbuf_, '\0', [self](err ec, size_t bytes)
                          {
                    
                    if(!ec){
                    const char* data = net::buffer_cast<const char*>(self->readbuf_.data());
                    std::string str(data, bytes);
                    boost::algorithm::trim(str);
                    auto obj = Service::DeserializeUmap<std::string, std::string>(str);
                    
                    
                    Service::PrintUmap(obj);
                    self->readbuf_.consume(bytes);
                    
                    


                    net::async_write(*(self->socket_), 
                    net::buffer(ServiceChatroomServer::MakeAnswerError("TESTR"," TESTR")),[self](err ec, size_t bytes){
                         self->HandleSession();
                    });
                    
                    }});

    return;
};

/*

    // auto tasks = Service::DoubleGuardedExcept<std::vector<task>>([self]()
    // { return Service::ExtractObjectsfromSocket(*(self->socket_)); }, "HandleSession");
    // condition = false;
*/

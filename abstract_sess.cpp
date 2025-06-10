#include"srv.h"
std::atomic_int AbstractSession::exempslars = 0;
void AbstractSession::HandleSession()
{
    //ПРОВЕРЯЕМ ЖИВ ЛИ СОКЕТ
    if (!Service::IsAliveSocket(*socket_))
    {
        Service::ShutDownSocket(*socket_);
        return;
    }
    auto self = this->shared_from_this();
    http::async_read(*socket_, readbuf_, request_, [self](err ec, size_t bytes) {
                              if (!ec)
                              {
                                  //ИЗВЛЕКАЕМ ЗНАЧЕНИЕ 
                                  auto action = Service::ExtractSharedObjectsfromRequestOrResponce(self->request_);
                                  //ЧИСТА БУФЕРА
                                  self->readbuf_.consume(bytes);
                                  
                                  std::string responce_body;
                                  responce_body = self->GetStringResponceToSocket(action);
                                  if(responce_body.empty()){return;}
                                  
                                  
                                  //КЛАДЕМ В ОЧЕРЕДЬ???
                                  self->mess_queue_.push_back(responce_body);

                                  auto resobj = Service::DeserializeUmap<std::string, std::string>(responce_body);       
                                  /*
                                  
                                  */
                                  http::status stat = http::status::ok;
                                  response rsp(Service::MakeResponce(11, true, stat, std::move(responce_body))); 
                                  //ПИШЕМ В СОКЕТ
                                  http::async_write(*(self->socket_), rsp, [self](err ec, size_t bytes) {});//async write
                                }//if !ec outer
            }//async read until lam
            );//async read until

    return;
};
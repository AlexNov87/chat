#include"srv.h"
void AbstractSession::HandleSession(bool need_check)
{
    //ПРОВЕРЯЕМ ЖИВ ЛИ СОКЕТ
    if (!Service::IsAliveSocket(*socket_))
    {
        Service::ShutDownSocket(*socket_);
        return;
    }
    auto self = this->shared_from_this();
    net::async_read_until(*socket_, readbuf_, CONSTANTS::SERIAL_SYM, [self, need_check](err ec, size_t bytes)
                          {
                              if (!ec)
                              {
                                  //ИЗВЛЕКАЕМ ЗНАЧЕНИЕ 
                                  auto action = Service::ExtractSharedObjectsfromBuffer(self->readbuf_, bytes);
                                  
                                  std::string responce;
                                  //ЕСЛИ НУЖНО ПРОВЕРЯТЬ
                                  //(Если вызов из подключенного сокета - поверка будет осуществленна в ChatRoom::AwaitSocket)
                                  if (need_check)
                                  {
                                      //РЯД ПРОВЕРОК ПРИ ЗПРОСЕ К СЕРВЕРУ
                                      auto reason = ServiceChatroomServer::CHK_Chr_CheckErrorsChatServer(*action);
                                      if (reason)
                                      {
                                          responce =  ServiceChatroomServer::MakeAnswerError(*reason, __func__);
                                      }
                                      else 
                                      {
                                           //ВОЗВРАЩАЕМ ОТВЕТ ДЛЯ СОКЕТА
                                           responce = self->GetStringResponceToSocket(action);
                                      }
                                  }
                                  else {
                                    //ВОЗВРАЩАЕМ ОТВЕТ ДЛЯ СОКЕТА
                                    responce = self->GetStringResponceToSocket(action);
                                  }
                                  //ЧИСТА БУФЕРА
                                  self->readbuf_.consume(bytes);
                                  
                                  //КЛАДЕМ В ОЧЕРЕДЬ???
                                  self->mess_queue_.push_back(responce);

                                  auto resobj = Service::DeserializeUmap<std::string, std::string>(responce);
                                  Service::PrintUmap(resobj);
                                  //ПИШЕМ В СОКЕТ
                                  net::async_write(*(self->socket_), net::buffer(responce), [self](err ec, size_t bytes) {
                                                      if(!ec){net::post(*self->strand_,[self]{ self->HandleSession();});}//if !ec inner
                                                    });//async write
                                }//if !ec outer
            }//async read until lam
            );//async read until

    return;
};
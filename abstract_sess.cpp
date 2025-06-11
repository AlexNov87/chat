#include "srv.h"
std::atomic_int AbstractSession::exempslars = 0;
void AbstractSession::HandleSession()
{

    //  ПРОВЕРЯЕМ ЖИВ ЛИ СОКЕТ
    if (!Service::IsAliveSocket(stream_->socket()))
    {
        Service::ShutDownSocket(stream_->socket());
        return;
    }
    auto self = this->shared_from_this();
    http::async_read(*stream_, readbuf_, request_, [self](err ec, size_t bytes)
                     {
                        try{

                        
                         if (!ec)
                         {
                             // ИЗВЛЕКАЕМ ЗНАЧЕНИЕ
                             auto action = Service::ExtractSharedObjectsfromRequestOrResponce(self->request_);
                             // ЧИСТА БУФЕРА
                             std::string responce_body;
                             
                             try{
                             // ПРОВЕРКИ ЕСТЬ ЛИ ДЕЙСТВИЕ
                             auto reason = ServiceChatroomServer::CHK_FieldExistsAndNotEmpty(*action, CONSTANTS::LF_ACTION);
                             if (reason)
                             {
                                 responce_body = ServiceChatroomServer::MakeAnswerError(*reason, "HandleSession()", CONSTANTS::UNKNOWN);
                             }
                             else
                             {
                                 responce_body = self->GetStringResponceToSocket(action);
                             }

                             if (responce_body.empty())
                             {
                                 return;
                             }
                            }
                            catch(const std::exception& ex){
                              responce_body = ServiceChatroomServer::MakeAnswerError(ex.what(), "HandleSession() Exc1", CONSTANTS::UNKNOWN);

                            }

                             // КЛАДЕМ В ОЧЕРЕДЬ???
                             self->mess_queue_.push_back(responce_body);

                             auto resobj = Service::DeserializeUmap<std::string, std::string>(responce_body);
                        
                             http::status stat = http::status::ok;
                             response rsp(Service::MakeResponce(11, true, stat, std::move(responce_body)));
                             // ПИШЕМ В СОКЕТ
                             http::async_write(*(self->stream_), rsp, [self](err ec, size_t bytes) {}); // async writ              
                         } // if !ec outer
                     } // async read until lam
                     catch(const std::exception&ex){
                            ZyncPrint("");                          
                     } }); // async read until

    return;
};
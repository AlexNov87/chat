#include "srv.h"
 void Chatuser::Read()
    {
        auto buf = Service::MakeSharedFlatBuffer();
        auto handler = [self = shared_from_this(), buf](err ec, size_t bytes)
        {
            ZyncPrint("LAMPDA READ...............");
            if (ec)
            {
                ZyncPrint(ec.what());
                return;
            }

            ZyncPrint("Body", self->request_.body(), self->request_.body().empty());

            auto i = Service::ExtractSharedObjectsfromRequestOrResponce(self->request_);
            self->request_.clear();
            Service::PrintUmap(*i);
            auto rsp = Service::MakeResponce(11, true, http::status::ok, ServiceChatroomServer::MakeAnswerError("TEST", "TEST", "TEST"));
            http::write(*self->socket_, rsp);
            self->Read();
        };
        ZyncPrint("READINASYNC.............");
        net::post(*strand_, [self = shared_from_this(), handler, buf]
                  { http::async_read(*self->socket_, self->buffer_, self->request_, handler); });
    }

    void Chatuser::Run()
    {
        net::post(*strand_, [self = shared_from_this()]
                  { 
        auto responce = Service::MakeResponce(11, true , http::status::ok,
        ServiceChatroomServer::Srv_MakeSuccessLogin("TTTY", "ROOOOM", "-----------")); 
        http::async_write(*self->socket_, responce, [self](err ec, size_t bytes){
              net::post([self] { self->Read();});
        }); });
    }

    void Chatuser::IncomeMessage(response resp)
    {
        net::post(*strand_, [self = shared_from_this(), resp]
                  { http::write(*self->socket_, resp); self->Read(); });
    }

    std::string Chatuser::ExecuteReadySesion(shared_task action)
    {

        try{
        auto reason = ServiceChatroomServer::CHK_FieldExistsAndNotEmpty(*action, CONSTANTS::LF_ACTION);
        if (reason)
        {
            return ServiceChatroomServer::MakeAnswerError(*reason, __func__, CONSTANTS::UNKNOWN);
        }
        std::string act_to_send = action->at(CONSTANTS::LF_ACTION);

        if (action->at(CONSTANTS::LF_DIRECTION) == CONSTANTS::RF_DIRECTION_CHATROOM)
        {
            auto reason = ServiceChatroomServer::CHK_Chr_CheckErrorsChatRoom(*action);
            if (reason)
            {
                return ServiceChatroomServer::MakeAnswerError(*reason, __func__, act_to_send);
            }
            // socket использоваться не будет, передан только в качестве реализации интерфейса
            ChatRoomSession session(room_.lock().get(), socket_);
            return session.ExecuteReadySession(action);
        }
        else if (action->at(CONSTANTS::LF_DIRECTION) == CONSTANTS::RF_DIRECTION_SERVER)
        {

            auto reason = ServiceChatroomServer::CHK_Chr_CheckErrorsChatRoom(*action);
            if (reason)
            {
                return ServiceChatroomServer::MakeAnswerError(*reason, __func__, act_to_send);
            }
            // socket использоваться не будет, передан только в качестве реализации интерфейса
            ServerSession session(room_.lock().get()->mainserv_, socket_);
            return session.ExecuteReadySession(action, socket_);
        }
        }
        catch(const std::exception &ex){
            return ServiceChatroomServer::MakeAnswerError("Exception Chatuser::ReadySesion", __func__, CONSTANTS::UNKNOWN); 
        }

        return ServiceChatroomServer::MakeAnswerError("", __func__, ""); 
    };

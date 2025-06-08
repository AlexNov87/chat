#include "srv.h"

std::string Chatroom::ChatRoomSession::GetStringResponceToSocket(shared_task action)
{
    return HandleAction(action);
};

std::string Chatroom::ChatRoomSession::HandleAction(shared_task action)
{

    Service::ACTION current_action =
        Service::Additional::action_scernario.at(CONSTANTS::LF_ACTION);

    if (!chatroom_->HasToken(action->at(CONSTANTS::LF_TOKEN)))
    {
        return ServiceChatroomServer::MakeAnswerError("TOKEN NOT FOUND", __func__);
    }

    switch (current_action)
    {
    case Service::ACTION::SEND_MESSAGE:
    {
        chatroom_->SendMessages(action->at(CONSTANTS::LF_TOKEN), action->at(CONSTANTS::LF_MESSAGE));
        return ServiceChatroomServer::Chr_MakeSuccessSendMessage();
    }
    break;
    case Service::ACTION::DISCONNECT:
    {
        chatroom_->DeleteUser(action->at(CONSTANTS::LF_TOKEN)); 
        return ServiceChatroomServer::MakeAnswerError("USER DELETED", __func__);
    }
    break;
    }
    return ServiceChatroomServer::MakeAnswerError("UNKNOWN ACTION", __func__);
}
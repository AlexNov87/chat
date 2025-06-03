#include "srv.h"

void Chatroom::ChatRoomSession::HandleExistingSocket(shared_socket socket, shared_task action)
{
    Service::DoubleGuardedExcept<void>([&]()
                                       { HandleAction(socket ,action); }, "HandleExistingSocket");
};

void Chatroom::ChatRoomSession::HandleTaskFromServer(shared_socket socket, shared_task action)
{
    HandleAction(socket, action);
}

void Chatroom::ChatRoomSession::HandleAction(shared_socket socket, shared_task action)
{

    auto action_error = ServiceChatroomServer::CHK_Chr_CheckErrorsChatRoom(*action);
    if (action_error)
    {
        ServiceChatroomServer::WriteErrorToSocket(socket, *action_error, CONSTANTS::RF_ERR_INITIATOR_CHATROOM);
        return;
    };

    Service::ACTION current_action =
        Service::Additional::action_scernario.at(CONSTANTS::LF_ACTION);

    switch (current_action)
    {
    case Service::ACTION::SEND_MESSAGE:
    {
        if (!chatroom_->HasToken(action->at(CONSTANTS::LF_TOKEN)))
        {
            ServiceChatroomServer::WriteErrorToSocket(socket, CONSTANTS::RF_ERR_PERMISSION_DENIDED, CONSTANTS::RF_ERR_INITIATOR_CHATROOM);
            return;
        }
        

        chatroom_->SendMessages(action->at(CONSTANTS::LF_TOKEN), action->at(CONSTANTS::LF_MESSAGE));
    }
    break;
    case Service::ACTION::DISCONNECT:
    {
        if (!chatroom_->HasToken(action->at(CONSTANTS::LF_TOKEN)))
        {
            ServiceChatroomServer::
                WriteErrorToSocket(socket, CONSTANTS::RF_ERR_PERMISSION_DENIDED, CONSTANTS::RF_ERR_INITIATOR_CHATROOM);
            return;
        }
        chatroom_->DeleteUser(action->at(CONSTANTS::LF_TOKEN));
    }
    break;
    }
}
#include "chatroom.h"

void Chatroom::ChatRoomSession::HandleExistingSocket(tcp::socket &socket, std::unordered_map<std::string, std::string> task)
{
    Service::DoubleGuardedExcept<void>([&]()
                                       { HandleAction(std::move(task), socket); }, "HandleExistingSocket");
};

void Chatroom::ChatRoomSession::HandleTaskFromServer(std::unordered_map<std::string, std::string> action, tcp::socket socket)
{
    HandleAction(action, socket);
}

void Chatroom::ChatRoomSession::HandleAction(std::unordered_map<std::string, std::string> action, tcp::socket &socket)
{

    auto action_error = ServiceChatroomServer::Chr_CheckErrorsChatRoom(action);
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
        if (!HasToken(action.at(CONSTANTS::LF_TOKEN)))
        {
            ServiceChatroomServer::WriteErrorToSocket(socket, CONSTANTS::RF_ERR_PERMISSION_DENIDED, CONSTANTS::RF_ERR_INITIATOR_CHATROOM);
            return;
        }
        chatroom_->SendMessages(action.at(CONSTANTS::LF_TOKEN), action.at(CONSTANTS::LF_MESSAGE));
    }
    break;
    case Service::ACTION::DISCONNECT:
    {
        if (!HasToken(action.at(CONSTANTS::LF_TOKEN)))
        {
            ServiceChatroomServer::
                WriteErrorToSocket(socket, CONSTANTS::RF_ERR_PERMISSION_DENIDED, CONSTANTS::RF_ERR_INITIATOR_CHATROOM);
            return;
        }
        chatroom_->DeleteUser(action.at(CONSTANTS::LF_TOKEN));
    }
    break;
    }
}
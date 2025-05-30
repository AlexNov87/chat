#include "chatroom.h"

void Chatroom::ChatRoomSession::HandleExistingSocket(tcp::socket &socket, std::unordered_map<std::string, std::string> task)
{
        Service::DoubleGuardedExcept<void>([&](){
        HandleAction(std::move(task), socket, Chatroom::COMED_FROM::COMED_FROM_OLD_SOCKET);
        }, "HandleExistingSocket");
};

void Chatroom::ChatRoomSession::HandleTaskFromServer(std::unordered_map<std::string, std::string> action, tcp::socket socket)
{
    HandleAction(action, socket, Chatroom::COMED_FROM::COMED_FROM_SERVER);
}

void Chatroom::ChatRoomSession::HandleAction(std::unordered_map<std::string, std::string> action, tcp::socket &socket, Chatroom::COMED_FROM from)
{

    auto action_error = Service_Chatroom::CheckErrorsChatRoom(action);
    if (action_error)
    {
        Service::WriteErrorToSocket(socket, *action_error, CONSTANTS::RF_ERR_INITIATOR_CHATROOM);
        return;
    };

    Service::ACTION current_action =
        Service::Additional::action_scernario.at(CONSTANTS::LF_ACTION);

    switch (current_action)
    {
    case Service::ACTION::CONNECT:
    {
        chatroom_->AddUser(std::move(socket), action.at(CONSTANTS::LF_NAME), action.at(CONSTANTS::LF_TOKEN));
    }
    break;
    case Service::ACTION::GET_USERS:
    {
        chatroom_->RoomMembers(socket, from);
    }
    break;
    case Service::ACTION::SEND_MESSAGE:
    {
       if(!HasToken(action.at(CONSTANTS::LF_TOKEN))){
            Service::WriteErrorToSocket(socket, CONSTANTS::RF_ERR_PERMISSION_DENIDED , CONSTANTS::RF_ERR_INITIATOR_CHATROOM);
        return;
       }
       
       chatroom_->SendMessages(action.at(CONSTANTS::LF_TOKEN), action.at(CONSTANTS::LF_MESSAGE));
    }
    break;
    case Service::ACTION::DISCONNECT:
    {
          if(!HasToken(action.at(CONSTANTS::LF_TOKEN))){
            Service::WriteErrorToSocket(socket, CONSTANTS::RF_ERR_PERMISSION_DENIDED , CONSTANTS::RF_ERR_INITIATOR_CHATROOM);
        return;
       }
       chatroom_->DeleteUser(action.at(CONSTANTS::LF_TOKEN));
    }
    break;
    }
}
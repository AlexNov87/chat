#include "srv.h"

std::string MainServer::ServerSession::GetStringResponceToSocket(shared_task action)
{
    auto reason = ServiceChatroomServer::CHK_Chr_CheckErrorsChatServer(*action);
    if (reason)
    {
        return ServiceChatroomServer::MakeAnswerError(*reason, __func__);
    }
    return ExectuteReadySession(action, socket_);
}

void MainServer::ServerSession::ExecuteTask(shared_task action)
{
    ExectuteReadySession(action, socket_);
}

std::string MainServer::ServerSession::ExectuteReadySession(shared_task action, shared_socket socket)
{
    try
    {
        Service::ACTION act = Service::Additional::action_scernario.at(action->at(CONSTANTS::LF_ACTION));
        switch (act)
        {
        case Service::ACTION::CREATE_ROOM:    
            ZyncPrint("CREATE_ROOM:");
            return server_->CreateRoom(std::move(action->at(CONSTANTS::LF_ROOMNAME)));
            break;
        case Service::ACTION::CREATE_USER:
             ZyncPrint("CREATE_USER:");
            return server_->AddUserToSQL(action->at(CONSTANTS::LF_NAME), action->at(CONSTANTS::LF_PASSWORD));
            break;
        case Service::ACTION::GET_USERS:            
             ZyncPrint("GET_USERS:");
            return server_->GetRoomUsersList(action->at(CONSTANTS::LF_ROOMNAME));
            break;
        case Service::ACTION::LOGIN:           
             ZyncPrint("LOGIN:");
            return server_->LoginUser(action, socket);
            break;
        case Service::ACTION::ROOM_LIST:           
           ZyncPrint("::ROOM_LIST:");
           return server_->GetRoomsList();
            break;
        }
        return ServiceChatroomServer::MakeAnswerError("UNRECOGNIZED ACTION SERVSESSION ", __func__);
    }
    catch (const std::exception &ex)
    {
        return ServiceChatroomServer::MakeAnswerError(ex.what(), __func__);
    }
}

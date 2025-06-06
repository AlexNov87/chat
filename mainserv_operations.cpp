#include "srv.h"

std::string MainServer::LoginUser(shared_task action, shared_socket socket)
{
    try
    {
        const auto &name = action->at(CONSTANTS::LF_NAME);
        const auto &pass = action->at(CONSTANTS::LF_PASSWORD);
        // ЕСЛИ АВТОРИЗОВАН
        if (!IsAutorizatedUser(name, pass))
        {
            return ServiceChatroomServer::MakeAnswerError("YOU ARE NOT AUTHORIZED", __func__);
        }
        // Генерируем токен
        std::string token = tokezier_.GenerateHEXToken();

        bool added = false;
        std::string last_messages;
        // Блокируем возможность модифицировать комнаты
        {/* Обязательно область видимости!!! Service::GuardLock
                   пока не выпоннит задачу, требующую блокировки
                */
        Service::GuardLockConditional(sync_.mod_users_, sync_.mtx_lock_mod_users_ , sync_.condition_).Lock();
        // ЕСЛИ ЕСТЬ ТАКАЯ КОМНАТА
        std::string &roomname = action->at(CONSTANTS::LF_ROOMNAME);
        if (!rooms_.count(roomname) > 0)
        {
            return ServiceChatroomServer::MakeAnswerError("NO ROOM: " + roomname, __func__);
        };
        auto room = rooms_.at(roomname);
        
        //Послелние сообщения комнтаты
        last_messages = room->msg_man_.LastMessages();
        // УДАЛОСЬ ЛИ ДОБАВТЬ
        bool added = room->AddUser(socket, name, token);
        
        }//конец блокировки
        
        if (added)
        {
            // ОТВЕЧАЕМ УСПЕХОМ
            return ServiceChatroomServer::Srv_MakeSuccessLogin(
                std::move(token), std::move(action->at(CONSTANTS::LF_ROOMNAME)), std::move(last_messages));
        }
        return ServiceChatroomServer::MakeAnswerError("FAILED TO ADD USER", __func__);
    }
    catch (const std::exception &ex)
    {
        return ServiceChatroomServer::MakeAnswerError("FAILED TO ADD USER", __func__);
    }
}

std::string MainServer::CreateRoom(std::string room)
{
    try
    {
        Service::GuardLockConditional(sync_.mod_users_ , sync_.mtx_lock_mod_users_ , sync_.condition_).Lock();
        if(rooms_.contains(room)){
           return ServiceChatroomServer::MakeAnswerError("FAILED TO CREATE ROOM, ROOM WITH THIS NAME IS EXISTS", __func__);
        }
        
        rooms_[room] = std::make_shared<Chatroom>(this->ioc_);
        return ServiceChatroomServer::Srv_MakeSuccessCreateRoom(room);
    }
    catch (const std::exception &ex)
    {
        return ServiceChatroomServer::MakeAnswerError("FAILED TO CREATE ROOM", __func__);
    }
}


 std::string MainServer::GetRoomUsersList(const std::string &roomname)
    {
        try
        {
            //БЛОКИРУЕМ И ДОБАЛЕНИЕ И УДАЛЕНИЕ КОМНАТ 
            Service::GuardLockConditional(sync_.mod_users_ , sync_.mtx_lock_mod_users_, sync_.condition_).Lock();
            if (!rooms_.contains(roomname))
            {    
                return ServiceChatroomServer::MakeAnswerError("THERE IS NO ROOM: " + roomname, __func__);
            }
            std::string userlist = rooms_.at(roomname)->RoomMembers();
            return ServiceChatroomServer::Srv_MakeSuccessGetUsers(std::move(userlist));
        }
        catch (const std::exception &ex)
        {
            return ServiceChatroomServer::MakeAnswerError(ex.what(), __func__);
        }
    }

    std::string MainServer::GetRoomsList()
    {
        try
        {
            std::stringstream strm;
            int rms = static_cast<int>(rooms_.size());
            int cnt = 0;
            strm << "[ ";
            {
                /* Обязательно область видимости!!! Service::GuardLock
                   пока не выпоннит задачу, требующую блокировки
                */
                Service::GuardLockConditional 
                (sync_.mod_users_ , sync_.mtx_lock_mod_users_, sync_.condition_).Lock();
                
                for (auto &&room : rooms_)
                {
                    strm << room.first << ' ';
                    ++cnt;
                    if (cnt + 1 == rms)
                    {
                        break;
                    }
                    strm << " , ";
                }
            }
            strm << " ]";
            return ServiceChatroomServer::Srv_MakeSuccessRoomList(std::move(strm.str()));
        }
        catch (const std::exception &ex)
        {
            return ServiceChatroomServer::MakeAnswerError(ex.what(), __func__);
        }
    }


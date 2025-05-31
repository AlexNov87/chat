#include "service.h"
// SUCESS CHATROOM
namespace ServiceChatroomServer
{
    std::unordered_map<std::string, std::string> GetSuccess()
    {
        return {
            {CONSTANTS::LF_RESULT, CONSTANTS::RF_SUCCESS},
        };
    }

    // ОТВЕТ СЕРВЕРА  НА УСПЕШНОЕ ПОСЛАНИЕ СООБЩЕНИЯ
    std::string Chr_MakeSuccessSendMessage()
    {
        std::unordered_map<std::string, std::string> res = GetSuccess();
        res[CONSTANTS::LF_ACTION] = CONSTANTS::ACT_SEND_MESSAGE;
        return Service::SerializeUmap<std::string, std::string>(res);
    };

}

// SUCESS SERVER
namespace ServiceChatroomServer
{
    // ОТВЕТ СЕРВЕРА НА УСПЕШНОЕ ПОЛУЧЕНИЕ ПОЛЬЗОВАТЕЛЕЙ
    std::string Srv_MakeSuccessGetUsers(std::string userlist)
    {
        std::unordered_map<std::string, std::string> res = GetSuccess();
        res[CONSTANTS::LF_ACTION] = CONSTANTS::ACT_GET_USERS;
        res[CONSTANTS::LF_USERS] = userlist;
        return Service::SerializeUmap<std::string, std::string>(res);
    };

    // ОТВЕТ СЕРВЕРА НА УСПЕШНОЕ ДОБАВЛЕНИЕ ПОЛЬЗОВАТЕЛЯ
    std::string Srv_MakeSuccessAddUser(std::string token, std::string roomname)
    {
        std::unordered_map<std::string, std::string> res = GetSuccess();
        res[CONSTANTS::LF_ACTION] = CONSTANTS::ACT_CONNECT;
        res[CONSTANTS::LF_TOKEN] = std::move(token);
        res[CONSTANTS::LF_ROOMNAME] = std::move(roomname);
        return Service::SerializeUmap<std::string, std::string>(res);
    };

    std::string Srv_MakeSuccessLogin(std::string name)
    {
        std::unordered_map<std::string, std::string> res = GetSuccess();
        res[CONSTANTS::LF_ACTION] = CONSTANTS::ACT_LOGIN;

        return Service::SerializeUmap<std::string, std::string>(res);
    };
    std::string Srv_MakeSuccessCreateUser(std::string name)
    {
        std::unordered_map<std::string, std::string> res = GetSuccess();
        res[CONSTANTS::LF_ACTION] = CONSTANTS::ACT_CREATE_USER;

        return Service::SerializeUmap<std::string, std::string>(res);
    };
    std::string Srv_MakeSuccessCreateRoom(std::string name)
    {
        std::unordered_map<std::string, std::string> res = GetSuccess();
        res[CONSTANTS::LF_ACTION] = CONSTANTS::ACT_CREATE_ROOM;
        res[CONSTANTS::LF_ROOMNAME] = std::move(name);

        return Service::SerializeUmap<std::string, std::string>(res);
    };
    std::string Srv_MakeSuccessRoomList(std::string roomlist)
    {

        std::unordered_map<std::string, std::string> res = GetSuccess();
        res[CONSTANTS::LF_ACTION] = CONSTANTS::ACT_ROOM_LIST;
        res[CONSTANTS::LF_ROOMLIST] = std::move(roomlist);

        return Service::SerializeUmap<std::string, std::string>(res);
    };

}
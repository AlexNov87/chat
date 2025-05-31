#include "service.h"
using namespace std;

const std::string CONSTANTS::ACT_CONNECT = "CONNECT";
const std::string CONSTANTS::ACT_DISCONNECT = "DISCONNECT";
const std::string CONSTANTS::ACT_GET_USERS = "GET_USERS";
const std::string CONSTANTS::ACT_SEND_MESSAGE = "SEND_MESSAGE";
const std::string CONSTANTS::ACT_LOGIN = "LOGIN";
const std::string CONSTANTS::ACT_CREATE_ROOM = "CREATE_ROOM";
const std::string CONSTANTS::ACT_ROOM_LIST = "ROOM_LIST";
const std::string CONSTANTS::ACT_CREATE_USER = "CREATE_USER";

// Map Parameters
const std::string CONSTANTS::LF_ACTION = "ACTION";
const std::string CONSTANTS::LF_NAME = "NAME";
const std::string CONSTANTS::LF_TOKEN = "TOKEN";
const std::string CONSTANTS::LF_MESSAGE = "MESSAGE";
const std::string CONSTANTS::LF_REASON = "REASON";
const std::string CONSTANTS::LF_RESULT = "RESULT";
const std::string CONSTANTS::LF_INITIATOR = "INITIATOR";
const std::string CONSTANTS::LF_USERS = "USERS";
const std::string CONSTANTS::LF_ROOMNAME = "ROOMNAME";
const std::string CONSTANTS::LF_LOGIN = "LOGIN";

const std::string CONSTANTS::RF_SUCCESS = "SUCCESS";
const std::string CONSTANTS::RF_ERROR = "ERROR";

//
const std::string CONSTANTS::RF_ERR_INITIATOR_SERVER = "INITIATOR_SERVER";
const std::string CONSTANTS::RF_ERR_INITIATOR_CHATROOM = "INITIATOR_CLENT";
const std::string CONSTANTS::RF_ERR_PERMISSION_DENIDED = "YOUR TOKEN INCORRECT. PERMISSION DENIDED";

const std::string CONSTANTS::LF_DIRECTION = "DIRECTION";
const std::string CONSTANTS::RF_DIRECTION_SERVER = "DIRECTION_SERVER";
const std::string CONSTANTS::RF_DIRECTION_CHATROOM = "DIRECTION_CHATROOM";

const size_t CONSTANTS::N_DISCONNECT = 3;       // direction, action, token
const size_t CONSTANTS::N_SEND_MESSAGE = 5 - 1; // direction, action, token, message

const size_t CONSTANTS::N_TOKEN_LEN = 32;
const size_t CONSTANTS::N_MAX_MESSAGE_LEN = 512;

const size_t CONSTANTS::N_CONNECT = 4;     // direction, action, roomname, username
const size_t CONSTANTS::N_GET_USERS = 3;   // direction, action, roomname

//IN WORK
const size_t CONSTANTS::N_LOGIN = 0;       // direction, action,
const size_t CONSTANTS::N_CREATE_ROOM = 0; // direction, action,
const size_t CONSTANTS::N_ROOM_LIST = 0;   // direction, action,
const size_t CONSTANTS::N_CREATE_USER = 0; // direction, action,

namespace Service
{

    void MtreadRunContext(net::io_context &ioc)
    {
        for (int i = 0; i < std::thread::hardware_concurrency(); ++i)
        {
            jthread jth([&ioc]
                        { ioc.run(); });
        }
    }

    std::unordered_map<std::string, std::string> GetTaskFromBuffer(net::streambuf &buffer)
    {
        const char *data = boost::asio::buffer_cast<const char *>(buffer.data());
        std::size_t size = buffer.size();
        std::string ln(data, size - 1);
        std::unordered_map<std::string, std::string> task =
            Service::DeserializeUmap<std::string, std::string>(ln);
        TrimContainer(task);
        return task;
    }

    void TrimContainer(std::unordered_map<std::string, std::string> &action)
    {
        for (auto &pair : action)
        {
            boost::algorithm::trim(pair.second);
        }
    }

    const std::unordered_map<std::string, ACTION> Additional::action_scernario{
        {CONSTANTS::ACT_CONNECT, ACTION::CONNECT},
        {CONSTANTS::ACT_DISCONNECT, ACTION::DISCONNECT},
        {CONSTANTS::ACT_SEND_MESSAGE, ACTION::SEND_MESSAGE},
        {CONSTANTS::ACT_GET_USERS, ACTION::GET_USERS}};

    const std::unordered_set<std::string> Service::Additional::chatroom_actions{
        CONSTANTS::ACT_DISCONNECT, CONSTANTS::ACT_SEND_MESSAGE};

    const std::unordered_set<std::string> Service::Additional::server_actions{
        CONSTANTS::ACT_GET_USERS, CONSTANTS::ACT_CONNECT};

    const std::unordered_set<std::string> Service::Additional::request_directions{
        CONSTANTS::RF_DIRECTION_SERVER, CONSTANTS::RF_DIRECTION_CHATROOM};
}

namespace ServiceChatroomServer
{
    //ОТВЕТ СЕРВЕРА НА ОШИБКУ
    std::string MakeAnswerError(std::string reason, string initiator)
    {
        unordered_map<string, string> res{
            {CONSTANTS::LF_RESULT, CONSTANTS::RF_ERROR},
            {CONSTANTS::LF_REASON, std::move(reason)},
            {CONSTANTS::LF_INITIATOR, std::move(initiator)}};
        return Service::SerializeUmap(res);
    };

    void WriteErrorToSocket(tcp::socket &socket, std::string reason, std::string initiator)
    {

        Service::DoubleGuardedExcept<void>(
            [&]()
            {
                socket.write_some(net::buffer(MakeAnswerError(reason, initiator)));
            },
            "WriteErrorToSocket");
    };
}

namespace ServiceChatroomServer
{

}

namespace ServiceChatroomServer
{
    namespace UserInterface
    {
        void ChrSetChatroomDirection(unordered_map<string, string> &action)
        {
            action[CONSTANTS::LF_DIRECTION] = CONSTANTS::RF_DIRECTION_CHATROOM;
        }
        std::string US_ChrMakeObjDisconnect(std::string token)
        {
            unordered_map<string, string> res{
                {CONSTANTS::LF_ACTION, CONSTANTS::ACT_DISCONNECT},
                {CONSTANTS::LF_TOKEN, token}

            };
            ChrSetChatroomDirection(res);
            return Service::SerializeUmap(res);
        };

        std::string US_ChrMakeSendMessage(std::string token, std::string message)
        {
            unordered_map<string, string> res{
                {CONSTANTS::LF_ACTION, CONSTANTS::ACT_SEND_MESSAGE},
                {CONSTANTS::LF_TOKEN, token},
                {CONSTANTS::LF_MESSAGE, message}};
            ChrSetChatroomDirection(res);
            return Service::SerializeUmap(res);
        };

    }
}

namespace ServiceChatroomServer
{
    namespace UserInterface
    {
        void ChrSetServerDirection(unordered_map<string, string> &action)
        {
            action[CONSTANTS::LF_DIRECTION] = CONSTANTS::RF_DIRECTION_SERVER;
        }

        std::string US_SrvMakeObjConnect(std::string name, std::string roomname) // sz4
        {
            unordered_map<string, string> res{
                {CONSTANTS::LF_ACTION, CONSTANTS::ACT_CONNECT},
                {CONSTANTS::LF_NAME, std::move(name)},
                {CONSTANTS::LF_ROOMNAME, std::move(roomname)}};
            ChrSetServerDirection(res);
            return Service::SerializeUmap(res);
        };

        std::string US_SrvMakeObjGetUsers(std::string name) // sz3
        {
            unordered_map<string, string> res{
                {CONSTANTS::LF_ACTION, CONSTANTS::ACT_GET_USERS},
                {CONSTANTS::LF_ROOMNAME, std::move(name)}};
            ChrSetServerDirection(res);
            return Service::SerializeUmap(res);
        };

        std::string US_SrvMakeObjLogin(std::string name)
        {
            unordered_map<string, string> res{};
            ChrSetServerDirection(res);
            return Service::SerializeUmap(res);
        };
        ///@brief Сериализованный объект для получения
        std::string US_SrvMakeObjCreateUser(std::string name)
        {
            unordered_map<string, string> res{};
            ChrSetServerDirection(res);
            return Service::SerializeUmap(res);
        };
        ///@brief Сериализованный объект для получения
        std::string US_SrvMakeObjCreateRoom(std::string name)
        {
            unordered_map<string, string> res{};
            ChrSetServerDirection(res);
            return Service::SerializeUmap(res);
        };
        ///@brief Сериализованный объект для получения
        std::string US_SrvMakeObjRoomList(std::string name)
        {
            unordered_map<string, string> res{};
            ChrSetServerDirection(res);
            return Service::SerializeUmap(res);
        };

    }

}

// SUCESS CHATROOM
namespace ServiceChatroomServer
{
    std::unordered_map<std::string, std::string> GetSuccess()
    {
        return {
            {CONSTANTS::LF_RESULT, CONSTANTS::RF_SUCCESS},
        };
    }

    // ОТВЕТ СЕРВЕРА НА ПОСЛАНИЕ СООБЩЕНИЯ
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

        return Service::SerializeUmap<std::string, std::string>(res);
    };
    std::string Srv_MakeSuccessCreateUser(std::string name)
    {
        std::unordered_map<std::string, std::string> res = GetSuccess();

        return Service::SerializeUmap<std::string, std::string>(res);
    };
    std::string Srv_MakeSuccessCreateRoom(std::string name)
    {
        std::unordered_map<std::string, std::string> res = GetSuccess();

        return Service::SerializeUmap<std::string, std::string>(res);
    };
    std::string Srv_MakeSuccessRoomList(std::string name)
    {
        std::unordered_map<std::string, std::string> res = GetSuccess();

        return Service::SerializeUmap<std::string, std::string>(res);
    };
    ;

}
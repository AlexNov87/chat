#pragma once
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/unordered_map.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/json.hpp>

#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <iostream>
#include <fstream>

#include <thread>
#include <optional>

#include "const.h"

namespace net = boost::asio;
using tcp = net::ip::tcp;

namespace Service
{
    enum class ACTION
    {
        /*Запросы к Чат-Комнате*/
        DISCONNECT,
        SEND_MESSAGE,

        /*Запросы к серверу*/
        LOGIN,
        GET_USERS,
        CREATE_ROOM,
        ROOM_LIST,
        CREATE_USER,
        CONNECT
    };

    struct Additional
    {
        static const std::unordered_map<std::string, ACTION> action_scernario;
        static const std::unordered_set<std::string> chatroom_actions;
        static const std::unordered_set<std::string> server_actions;
        static const std::unordered_set<std::string> request_directions;
    };

    enum class COMED_FROM
    {
        COMED_FROM_SERVER,
        COMED_FROM_OLD_SOCKET
    };

    ///@brief Сериализатор Unordered_Map
    template <typename T1, typename T2>
    std::string SerializeUmap(std::unordered_map<T1, T2> object)
    {
        std::ostringstream strm;
        boost::archive::text_oarchive arch(strm);
        arch << object;
        strm << '\0';
        return strm.str();
    }

    ///@brief Десериализатор Unordered_Map
    template <typename T1, typename T2>
    std::unordered_map<T1, T2> DeserializeUmap(std::string serialized)
    {
        std::istringstream strm(std::move(serialized));
        boost::archive::text_iarchive arch(strm);
        std::unordered_map<T1, T2> umap;
        arch >> umap;
        return umap;
    }

    // Запускает контекст в многопоточном режиме
    void MtreadRunContext(net::io_context &ioc);

    class PassHasher
    {
        std::hash<std::string> hasher{};

    public:
        size_t MakePassHash(const std::string &password)
        {
            return (hasher(password) * 37 * 11) ^ 3737373737;
        }
    };

    std::unordered_map<std::string, std::string> GetTaskFromBuffer(net::streambuf &buffer);

    template <typename T, typename Foo>
    T DoubleGuardedExcept(Foo foo, std::string fooname)
    {
        try
        {
            return foo();
        }
        catch (const std::exception &ex)
        {
            std::cout << "Foo:" << fooname << "  " << ex.what() << '\n';
        }
        catch (...)
        {
            std::cout << "Foo:" << fooname << " UNKNOWN EXCEPTION\n";
        }
    }

    void TrimContainer(std::unordered_map<std::string, std::string> &action);
}

namespace ServiceChatroomServer
{
    std::string MakeAnswerError(std::string reason, std::string initiator);
    void WriteErrorToSocket(tcp::socket &socket, std::string reason, std::string initiator);
    std::optional<std::string> CHK_ActionDirectionIncorrect(const std::unordered_map<std::string, std::string> &action);
    std::optional<std::string> CHK_Chr_CheckErrorsChatRoom(const std::unordered_map<std::string, std::string> &action);
    
    std::string Chr_MakeSuccessSendMessage();

    std::string Srv_MakeSuccessGetUsers(std::string userlist);
    std::string Srv_MakeSuccessLogin(std::string token, std::string roomname);
    std::string Srv_MakeSuccessCreateUser(std::string name);
    std::string Srv_MakeSuccessCreateRoom(std::string name);
    std::string Srv_MakeSuccessRoomList(std::string roomlist);

}

namespace UserInterface
{
    ///@brief Сериализованный объект для отключения
    std::string US_ChrMakeObjDisconnect(std::string token);
    ///@brief Сериализованный объект для послания сообщения
    std::string US_ChrMakeSendMessage(std::string token, std::string message);
}

namespace UserInterface
{
    ///@brief Сериализованный объект для получения списка пользователей
    std::string US_SrvMakeObjGetUsers(std::string name);
    ///@brief Сериализованный объект для логина на сервере
    std::string US_SrvMakeObjLogin(std::string name, std::string password, std::string roomname);
    ///@brief Сериализованный объект для получения
    std::string US_SrvMakeObjCreateUser(std::string name, std::string password);
    ///@brief Сериализованный объект для создания комнаты
    std::string US_SrvMakeObjCreateRoom(std::string name);
    ///@brief Сериализованный объект для получения списка комнат
    std::string US_SrvMakeObjRoomList();
}

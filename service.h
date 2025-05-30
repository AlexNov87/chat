#pragma once
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/unordered_map.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/streambuf.hpp>

#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <iostream>

#include <thread>
#include <optional>

namespace net = boost::asio;
using tcp = net::ip::tcp;

struct CONSTANTS
{
    // Actions
    static const std::string ACT_CONNECT;
    static const std::string ACT_DISCONNECT;
    static const std::string ACT_GET_USERS;
    static const std::string ACT_SEND_MESSAGE;

    // Map Parameters
    static const std::string LF_ACTION;
    static const std::string LF_NAME;
    static const std::string LF_TOKEN;
    static const std::string LF_MESSAGE;
    static const std::string LF_REASON;
    static const std::string LF_RESULT;

    static const std::string RF_SUCCESS;
    static const std::string RF_ERROR;

    //
    static const std::string LF_INITIATOR;
    static const std::string RF_ERR_INITIATOR_SERVER;
    static const std::string RF_ERR_INITIATOR_CHATROOM;

    static const std::string RF_ERR_PERMISSION_DENIDED;

    ///@brief Размер объекта действия подключения
    static const size_t N_CONNECT;
    ///@brief Размер объекта действия отключения
    static const size_t N_DISCONNECT;
    ///@brief Размер объекта действия для получения списка пользователей
    static const size_t N_GET_USERS;
    ///@brief Размер объекта действия послания сообщения
    static const size_t N_SEND_MESSAGE;
    ///@brief Длина токена
    static const size_t N_TOKEN_LEN;
    ///@brief
    static const size_t N_MAX_MESSAGE_LEN;

private:
    CONSTANTS() {};
};

namespace Service
{
    enum class ACTION
    {
        /*Запросы к Чат-Комнате*/
        DISCONNECT,
        SEND_MESSAGE,
        GET_USERS,

        /*Запросы к серверу*/
        CREATE_ROOM,
        ROOM_LIST,
        CREATE_USER,
        CONNECT,
    };

    struct Additional
    {
        static const std::unordered_map<std::string, ACTION> action_scernario;
        static const std::unordered_set<std::string> chatroom_actions;
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

    // ВЗАИМОДЕЙСТВИЕ С ЧАТ-РУМОМ
    ///@brief Сериализованный объект для подключения
    std::string ChrMakeObjConnect(std::string name);
    ///@brief Сериализованный объект для отключения
    std::string ChrMakeObjDisconnect(std::string token);
    ///@brief Сериализованный объект для получения списка пользователей
    std::string ChrMakeObjGetUsers();
    ///@brief Сериализованный объект для послания сообщения
    std::string ChrMakeSendMessage(std::string token, std::string message);

    // Запускает контекст в многопоточном режиме
    void MtreadRunContext(net::io_context &ioc);

    std::string ChrMakeAnswerError(std::string reason, std::string initiator);
    void WriteErrorToSocket(tcp::socket &socket, std::string reason, std::string initiator);

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
}

namespace Service_Chatroom
{
    std::optional<std::string> CheckErrorsChatRoom(std::unordered_map<std::string, std::string> &action);
    std::string Chr_MakeSuccessAddUser(std::string token);
    std::string Chr_MakeSuccessGetUsers(std::string userlist);
    std::string Chr_MakeSuccessSendMessage();
}
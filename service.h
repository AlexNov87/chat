#pragma once
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/unordered_map.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/json.hpp>

#include <boost/asio/strand.hpp>
#include <boost/asio/post.hpp>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/read_until.hpp>

#include <unordered_map>
#include <condition_variable>
#include <mutex>
#include <memory>
#include <variant>

#include <iostream>
#include <fstream>
#include <sstream>
#include <syncstream>
#include <chrono>

#include <string>
#include <unordered_set>
#include <functional>
#include <thread>
#include <array>
#include <optional>
#include <deque>
#include "const.h"

namespace net = boost::asio;
using tcp = net::ip::tcp;
using task = std::unordered_map<std::string, std::string>;
using task_complex = std::vector<task>;
using shared_socket = std::shared_ptr<tcp::socket>;
using shared_task = std::shared_ptr<task>;
using strand = boost::asio::strand<boost::asio::io_context::executor_type>;
using shared_strand = std::shared_ptr<strand>;

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

    ///@brief Печать Unordered_Map
    template <typename T1, typename T2>
    void PrintUmap(std::unordered_map<T1, T2> object)
    {
        for (auto &&el : object)
        {
            std::osyncstream(std::cout) << el.first << "->" << el.second << '\n';
        };
        std::osyncstream(std::cout) << '\n';
    }

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

    ///@brief Запускает контекст в многопоточном режиме
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

    ///@brief Зашищает от исключений
    template <typename T, typename Foo>
    T DoubleGuardedExcept(Foo foo, std::string fooname, std::ostream &os = std::cout)
    {
        try
        {
            return foo();
        }
        catch (const std::exception &ex)
        {
            os << "Foo:" << fooname << "  " << ex.what() << '\n';
        }
        catch (...)
        {
            os << "Foo:" << fooname << " UNKNOWN EXCEPTION\n";
        }
        return foo();
    }
    ///@brief Читает из файла в string
    std::string ReadFromFstream(std::ifstream &ifs);
    ///@brief Проверяет жив ли сокет
    bool IsAliveSocket(tcp::socket &sock);
    bool IsAliveSocket(shared_socket sock);
    ///@brief Выключает сокет
    void ShutDownSocket(tcp::socket &sock);
    void ShutDownSocket(shared_socket sock);

    ///@brief Извлекает список полученыых обьектов из сокета
    std::vector<task> ExtractObjectsfromSocket(tcp::socket &socket);
    ///@brief Извлекает список полученыых обьектов из сокета в виде shared_ptr
    std::vector<shared_task> ExtractSharedObjectsfromSocket(tcp::socket &socket);

    ///@brief Извлекает список полученыых обьектов из буфера
    std::vector<task> ExtractObjectsfromBuffer(net::streambuf &buffer);
    ///@brief Извлекает список полученыых обьектов из буфера в виде shared_ptr
    std::vector<shared_task> ExtractSharedObjectsfromBuffer(net::streambuf &buffer);

    shared_strand MakeSharedStrand(net::io_context &ioc);
}

namespace ServiceChatroomServer
{
    std::optional<std::string> CHK_ServerLoadObject(const boost::json::value &obj);
    ///@brief Создать о
    std::string MakeAnswerError(std::string reason, std::string initiator);
    ///@brief  Написаить ошибку в сокет
    void WriteErrorToSocket(tcp::socket &socket, std::string reason, std::string initiator);
    void WriteErrorToSocket(shared_socket socket, std::string reason, std::string initiator);
    ///@brief Проверяет валидно ли поле "направление"
    std::optional<std::string> CHK_FieldDirectionIncorrect(const task &action);
    ///@brief Проверяет валиден ли запрос к чатруму
    std::optional<std::string> CHK_Chr_CheckErrorsChatRoom(const task &action);
    ///@brief Проверяет валиден ли запрос к серверу
    std::optional<std::string> CHK_Chr_CheckErrorsChatServer(const task &action);

    ///@brief Успешное послание сообщения
    std::string Chr_MakeSuccessSendMessage();
    ///@brief Успешное получение сообщений до конннекта
    std::string Chr_MakeSuccessLastMessages(std::string msglist);
    ///@brief Успешное получение списка юзеров
    std::string Srv_MakeSuccessGetUsers(std::string userlist);
    ///@brief Успешный вход в систему
    std::string Srv_MakeSuccessLogin(std::string token, std::string roomname);
    ///@brief Успешное создание пользователя
    std::string Srv_MakeSuccessCreateUser(std::string name);
    ///@brief Успешное создание комнаты
    std::string Srv_MakeSuccessCreateRoom(std::string name);
    ///@brief Успешное получение списка комнат
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

#pragma once
#include "tokenizer.h"
#include "service.h"

namespace net = boost::asio;
using tcp = net::ip::tcp;
using namespace std::string_literals;

class MainServer;

class Chatroom
{
    struct Chatuser
    {
        Chatuser(std::string name, net::io_context &ioc, tcp::socket socket) : name_(std::move(name)),
                                                                               strand_(net::make_strand(ioc)),
                                                                               socket_(std::move(socket)) {};
        std::string name_;
        boost::asio::strand<boost::asio::io_context::executor_type> strand_;
        tcp::socket socket_;
    };

    class ChatRoomSession : public std::enable_shared_from_this<ChatRoomSession>
    {
        Chatroom *chatroom_ = nullptr;

    public:
        ChatRoomSession(Chatroom *chat) : chatroom_(chat) {};

        void HandleExistingSocket(tcp::socket &socket, task task);
        void HandleTaskFromServer(task action, tcp::socket socket);

    private:
        void HandleAction(task action, tcp::socket &socket);
    };

public:
    Chatroom(net::io_context &ioc) : ioc_(ioc) {}

    void HandleIncomeSocket(tcp::socket socket, task action);

private:
    MainServer *mainserv_;
    friend class MainServer;
    friend class ChatRoomSession;
    net::io_context &ioc_;
    std::unordered_map<std::string, Chatuser> users_;

    std::condition_variable cond_;
    bool modyfiing_users = false;
    bool do_not_allow_modify_users = false;
    std::mutex mut_users;

private:
    template <typename Foo>
    void MakeLockedModUsers(Foo foo)
    {
        // Блокируем возможность рассылки по сокетам на время модификации списка
        std::unique_lock<std::mutex> ul(mut_users);
        modyfiing_users = true;
        cond_.wait(ul, [&]()
                   { return do_not_allow_modify_users == false; });
        foo();
        // закончили модификацию
        modyfiing_users = false;
        // оповещаем потоки
        cond_.notify_all();
    }
    bool HasToken(const std::string &token);
    void AwaitSocket(std::string &token);

    void AddUser(tcp::socket socket, std::string name, std::string token, std::string roomname);
    void SendMessages(std::string token, std::string message);
    void DeleteUser(std::string token);
    std::string RoomMembers();
};

class MainServer
{
    friend class Chatroom;
    friend class ServerSession;

    net::io_context &ioc_;
    tcp::acceptor acceptor_;
    tcp::endpoint endpoint_;

    bool is_first_run_ = true;
    std::unordered_map<std::string, std::shared_ptr<Chatroom>> rooms_;

    class ServerSession : public std::enable_shared_from_this<ServerSession>
    {
        MainServer *server_;

    public:
        ServerSession(MainServer *server) : server_(server) {}

        void HandleSession(std::shared_ptr<tcp::socket> socket);

        void HandleSessionFromExistSocket(task action, Chatroom::Chatuser &chatuser) {

        };
    };

public:
    void Listen();

    MainServer(net::io_context &ioc) : ioc_(ioc), acceptor_(net::make_strand(ioc_))
    {
        init();
    }

    void PrintRooms()
    {
        for (auto &&room : rooms_)
        {
            std::cout << room.first << " members:" << room.second->users_.size() << '\n';
        }
    }

private:
    void init();
};
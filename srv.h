#pragma once
#include "tokenizer.h"
#include "message_man.h"
#include "service.h"

class MainServer;

class Chatroom
{
    struct Chatuser
    {
        Chatuser(std::string name, net::io_context &ioc, shared_socket socket) : name_(std::move(name)),
                                                                                 strand_(Service::MakeSharedStrand(ioc)),
                                                                                 socket_(socket) {};
        std::string name_;
        shared_strand strand_;
        shared_socket socket_;
    };

    class ChatRoomSession : public std::enable_shared_from_this<ChatRoomSession>
    {
        Chatroom *chatroom_ = nullptr;

    public:
        ChatRoomSession(Chatroom *chat) : chatroom_(chat) {};

        void HandleExistingSocket(shared_socket socket, shared_task action);
        void HandleTaskFromServer(shared_socket socket, shared_task action);

    private:
        void HandleAction(shared_socket socket, shared_task action);
    };

public:
    Chatroom(net::io_context &ioc) : ioc_(ioc) {}

private:
    friend class MainServer;
    friend class ChatRoomSession;
    MainServer *mainserv_;
    net::io_context &ioc_;
    std::unordered_map<std::string, Chatuser> users_;

    std::condition_variable cond_;
    bool modyfiing_users = false;
    bool do_not_allow_modify_users = false;
    std::mutex mut_users_;
    MessageManager msg_man_;

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
    void AwaitSocket(const std::string &token);

    void AddUser(shared_socket socket, std::string name, std::string token, std::string roomname);
    void SendMessages(const std::string &token, const std::string &message);
    void DeleteUser(const std::string &token);
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

        void Execute(const task_complex &, shared_socket socket)
        {
        }

        //  void ExectuteReadySession(shared_socket socket, shared_task action,   )

    public:
        ServerSession(MainServer *server) : server_(server) {}
        void HandleSession(shared_socket socket);

        void HandleExistsSocket(shared_task action, Chatroom::Chatuser &chatuser) {

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
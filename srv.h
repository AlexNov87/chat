#pragma once
#include "tokenizer.h"
#include "message_man.h"
#include "service.h"

class MainServer;

class AbstractSession : public std::enable_shared_from_this<AbstractSession>
{
protected:
    virtual void ExecuteTask(shared_task action) = 0;
    AbstractSession(net::io_context &ioc, shared_strand strand, shared_socket &socket) : timer_{ioc}, strand_(strand), socket_(socket) {}

protected:
    shared_socket socket_;
    net::streambuf readbuf_;
    shared_strand strand_;
    std::atomic_bool condition = true;
    net::steady_timer timer_;

    virtual std::string GetStringResponceToSocket(shared_task action) = 0;

public:
    void HandleSession();
};

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
        std::unique_lock<std::mutex> ul(mut_users_);
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
    friend class AbstactSession;

    net::io_context &ioc_;
    tcp::acceptor acceptor_;
    tcp::endpoint endpoint_;

    bool is_first_run_ = true;
    std::unordered_map<std::string, std::shared_ptr<Chatroom>> rooms_;

    class ServerSession : public AbstractSession
    {
        MainServer *server_;

        void ExecuteTask(shared_task action)
        {
            ExectuteReadySession(action, socket_, strand_);
        }

        std::string ExectuteReadySession(shared_task action, shared_socket socket, shared_strand strand)
        {
            Service::ACTION act = Service::Additional::action_scernario.at(action->at(CONSTANTS::LF_ACTION));
            switch (act)
            {
            case Service::ACTION::CREATE_ROOM:

                
                return ServiceChatroomServer::Srv_MakeSuccessCreateRoom
                (std::move(action->at(CONSTANTS::LF_ROOMNAME)));
                break;
            case Service::ACTION::CREATE_USER:
                /* code */
                
                return ServiceChatroomServer::Srv_MakeSuccessCreateUser
                (std::move(action->at(CONSTANTS::LF_NAME)));
                break;
            case Service::ACTION::GET_USERS:
                
                
                return ServiceChatroomServer::Srv_MakeSuccessGetUsers("");
                /* code */
                break;
            case Service::ACTION::LOGIN:
                
                return ServiceChatroomServer::Srv_MakeSuccessLogin("", "");
                break;
            case Service::ACTION::ROOM_LIST:
                
                return ServiceChatroomServer::Srv_MakeSuccessRoomList("");
                break;
            }

            return ServiceChatroomServer::MakeAnswerError("UNRECOGNIZED ACTION", __func__);
        }

        std::string GetStringResponceToSocket(shared_task action) override
        {
            auto reason = ServiceChatroomServer::CHK_Chr_CheckErrorsChatServer(*action);
            if (reason)
            {
                return ServiceChatroomServer::MakeAnswerError(*reason, __func__);
            }
            return ExectuteReadySession(action, socket_, strand_);
        }

    public:
        ServerSession(MainServer *server, shared_socket socket, shared_strand strand)
            : AbstractSession(server->ioc_, strand, socket), server_(server) {};

        void HandleExistsSocket(shared_task action, Chatroom::Chatuser &chatuser)
        {
            return;
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
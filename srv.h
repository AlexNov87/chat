#pragma once
#include "tokenizer.h"
#include "message_man.h"
#include "service.h"

class MainServer;

class AbstractSession : public std::enable_shared_from_this<AbstractSession>
{
protected:
    AbstractSession(net::io_context &ioc, shared_strand strand, shared_socket &socket)
        : timer_{ioc}, strand_(strand), socket_(socket) {}
    shared_socket socket_;
    net::streambuf readbuf_;
    shared_strand strand_;
    std::atomic_bool condition = true;
    net::steady_timer timer_;
    virtual std::string GetStringResponceToSocket(shared_task action) = 0;

public:
    void HandleSession(bool need_check = true);
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

    class ChatRoomSession : public AbstractSession
    {
        Chatroom *chatroom_ = nullptr;

    public:
        ChatRoomSession(Chatroom *chat, shared_socket socket, shared_strand strand)
            : AbstractSession(chat->ioc_, strand, socket), chatroom_(chat) {};
        std::string GetStringResponceToSocket(shared_task action) override;

    private:
        std::string HandleAction(shared_task action);
    };

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

public:
    Chatroom(net::io_context &ioc) : ioc_(ioc) {}

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

    bool AddUser(shared_socket socket, std::string name, std::string token);
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
    Service::TokenGen tokezier_;

    std::unordered_map<std::string, std::shared_ptr<Chatroom>> rooms_;
    std::mutex mtx_;
    std::atomic_bool mod_users_ = false;
    std::condition_variable condition_;

    template<typename Foo>
    void AvoidModUsers(Foo foo){
         std::unique_lock<std::mutex> ul(mtx_);
     condition_.wait(ul, [this]
                             { return this->mod_users_ == false; });
    mod_users_ = true;
          foo();
    mod_users_ = false;
    condition_.notify_all();
    }
    
    bool IsAutorizatedUser(const std::string &name, const std::string &passhash)
    {
        return true;
    }
    
    void CreateRoom(std::string room);
    void AddUserToRoom(shared_socket socket, const std::string &name, const std::string token, const std::string &roomname);

    class ServerSession : public AbstractSession
    {
        MainServer *server_;

        void ExecuteTask(shared_task action)
        {
            ExectuteReadySession(action, socket_);
        }

        std::string ExectuteReadySession(shared_task action, shared_socket socket)
        {
            Service::ACTION act = Service::Additional::action_scernario.at(action->at(CONSTANTS::LF_ACTION));
            switch (act)
            {
            case Service::ACTION::CREATE_ROOM:
            {
                
                return ServiceChatroomServer::Srv_MakeSuccessCreateRoom(std::move(action->at(CONSTANTS::LF_ROOMNAME)));
            }
            break;

            case Service::ACTION::CREATE_USER:
            { /* code */

                return ServiceChatroomServer::Srv_MakeSuccessCreateUser(std::move(action->at(CONSTANTS::LF_NAME)));
            }
            break;
            case Service::ACTION::GET_USERS:

                return ServiceChatroomServer::Srv_MakeSuccessGetUsers("");
                /* code */
                break;
            case Service::ACTION::LOGIN:
                return LoginUser(action, socket);
                break;
            case Service::ACTION::ROOM_LIST:
            {
                return ServiceChatroomServer::Srv_MakeSuccessRoomList("");
            }
            break;
            }

            return ServiceChatroomServer::MakeAnswerError("UNRECOGNIZED ACTION", __func__);
        }

        std::string GetStringResponceToSocket(shared_task action) override;
        std::string LoginUser(shared_task action, shared_socket socket);

    public:
        ServerSession(MainServer *server, shared_socket socket, shared_strand strand)
            : AbstractSession(server->ioc_, strand, socket), server_(server) {};
    };

public:
    void Listen();
    MainServer(net::io_context &ioc);
    void PrintRooms();

private:
    void init();
};
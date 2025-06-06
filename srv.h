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
    std::deque<std::string> mess_queue_;
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
    booltype modyfiing_users = false;
    booltype do_not_allow_modify_users = false;
    std::mutex mut_users_;
    MessageManager msg_man_;

public:
    Chatroom(net::io_context &ioc) : ioc_(ioc) {}
private:    
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

    struct Syncro
    {
        std::mutex mtx_lock_mod_users_;
        std::mutex mtx_lock_mod_sql_;
        std::atomic_bool mod_users_ = false;
        std::atomic_bool mod_sql_ = false;
        std::condition_variable condition_;
    };

    class ServerSession : public AbstractSession
    {
        MainServer *server_;
        void ExecuteTask(shared_task action);
        std::string ExectuteReadySession(shared_task action, shared_socket socket);
        std::string GetStringResponceToSocket(shared_task action) override;

    public:
        ServerSession(MainServer *server, shared_socket socket, shared_strand strand)
            : AbstractSession(server->ioc_, strand, socket), server_(server) {};
    };

    net::io_context &ioc_;
    tcp::acceptor acceptor_;
    tcp::endpoint endpoint_;
    Service::TokenGen tokezier_;
    std::unordered_map<std::string, std::shared_ptr<Chatroom>> rooms_;
    Syncro sync_;

    // В работе....
    bool AlreadyUserRegistered(const std::string &name)
    {
        /* ЛОГИКА ЗАПРОСА К SQL ИЛИ ЗАРАНЕЕ ПРОГРУЖЕННОМУ ОБЪЕКТУ*/
        return false;
    };

    bool IsAutorizatedUser(const std::string &name, const std::string &passhash)
    {
        /*  ПРОВЕРКА ЛОГИКИ АВТОРИЗАЦИИ */

        return true;
    }

    std::string AddUserToSQL(const std::string &name, const std::string &passhash)
    {
        std::lock_guard<std::mutex> lg(sync_.mtx_lock_mod_sql_); //??????? Возможно будет не нужно
        try
        {
            // ЕСЛИ УЖЕ ЗАРЕГИСТРИРОВАН
            if (AlreadyUserRegistered(name))
            {
                return ServiceChatroomServer::MakeAnswerError("USER WITH THIS NICK ALREADY REGISTERED", __func__);
            };
            /*
            ЛОГИКА ЗАПРОСА К SQL И ДОБАВЛЕНИЕ В ОБЩИЙ СПИСОК??
            */
            return ServiceChatroomServer::Srv_MakeSuccessCreateUser(name);
        }
        catch (const std::exception &ex)
        {
            return ServiceChatroomServer::MakeAnswerError(ex.what(), __func__);
        }
    }
      //..................    
public:
    void Listen();
    MainServer(net::io_context &ioc);
    void PrintRooms();

private:
    void init();
    //Действия
    std::string GetRoomUsersList(const std::string &roomname);
    std::string GetRoomsList();
    std::string CreateRoom(std::string room);
    std::string LoginUser(shared_task action, shared_socket socket);
};
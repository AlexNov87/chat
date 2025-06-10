#pragma once
#include "tokenizer.h"
#include "message_man.h"
#include "service.h"

class MainServer;
class Chatroom;
struct Chatuser;

class AbstractSession : public std::enable_shared_from_this<AbstractSession>
{
protected:
    AbstractSession(shared_socket &socket)
        : socket_(socket)
    {
        ZyncPrint("ABSTRACTSESS: " + std::to_string(++exempslars) + " IS CONSTRUCTING");
    }
    shared_socket socket_;
    beast::flat_buffer readbuf_;
    request request_;
    std::atomic_bool condition = true;
    std::deque<std::string> mess_queue_;
    std::atomic_bool keep_sess_ = true;
    static std::atomic_int exempslars;
    virtual std::string GetStringResponceToSocket(shared_task action) = 0;

    virtual ~AbstractSession()
    {
        ZyncPrint("SESSION CLOSED.....");
    };

public:
    void HandleSession();
    void FreeSession()
    {
        keep_sess_ = false;
    }
};

class ServerSession : public AbstractSession
{
    MainServer *server_;
    static std::atomic_int exempslars_s;
    friend class Chatroom;
    friend class Chatuser;
    void ExecuteTask(shared_task action);
    std::string ExecuteReadySession(shared_task action, shared_socket socket);
    std::string GetStringResponceToSocket(shared_task action) override;

public:
    ServerSession(MainServer *server, shared_socket socket)
        : AbstractSession(socket), server_(server)
    {
        ZyncPrint("SERVERSESS: " + std::to_string(++exempslars_s) + " IS CONSTRUCTING");
    };
};

class ChatRoomSession : public AbstractSession
{
    Chatroom *chatroom_ = nullptr;
    friend class Chatuser;

public:
    ChatRoomSession(Chatroom *chat, shared_socket socket)
        : AbstractSession(socket), chatroom_(chat) {};
    std::string GetStringResponceToSocket(shared_task action) override;

private:
    std::string ExecuteReadySession(shared_task action);
};

struct Chatuser : public std::enable_shared_from_this<Chatuser>
{
    Chatuser(std::weak_ptr<Chatroom> room, std::string name, shared_socket socket, net::io_context &io) : room_(room), name_(std::move(name)),
                                                                                                          socket_(socket), ioc_(io)
    {
        strand_ = Service::MakeSharedStrand(ioc_);
    }
    std::weak_ptr<Chatroom> room_;
    std::string name_;
    shared_socket socket_;
    net::io_context &ioc_;
    shared_strand strand_;
    beast::flat_buffer buffer_;
    request request_;

    void Read()
    {
        auto buf = Service::MakeSharedFlatBuffer();
        auto handler = [self = shared_from_this(), buf](err ec, size_t bytes)
        {
            ZyncPrint("LAMPDA READ...............");
            if (ec)
            {
                ZyncPrint(ec.what());
                return;
            }

            ZyncPrint("Body", self->request_.body(), self->request_.body().empty());

            auto i = Service::ExtractSharedObjectsfromRequestOrResponce(self->request_);
            self->request_.clear();
            Service::PrintUmap(*i);
            auto rsp = Service::MakeResponce(11, true, http::status::ok, ServiceChatroomServer::MakeAnswerError("TEST", "TEST", "TEST"));
            http::write(*self->socket_, rsp);
            self->Read();
        };
        ZyncPrint("READINASYNC.............");
        net::post(*strand_, [self = shared_from_this(), handler, buf]
                  { http::async_read(*self->socket_, self->buffer_, self->request_, handler); });
    }

    void Run()
    {
        net::post(*strand_, [self = shared_from_this()]
                  { 
        auto responce = Service::MakeResponce(11, true , http::status::ok,
        ServiceChatroomServer::Srv_MakeSuccessLogin("TTTY", "ROOOOM", "-----------")); 
        http::async_write(*self->socket_, responce, [self](err ec, size_t bytes){
              net::post([self] { self->Read();});
        }); });
    }

    void IncomeMessage(response resp)
    {
        net::post(*strand_, [self = shared_from_this(), resp]
                  { http::write(*self->socket_, resp); self->Read(); });
    }

    std::string ExecuteReadySesion(shared_task action)
    {

        try{
        auto reason = ServiceChatroomServer::CHK_FieldExistsAndNotEmpty(*action, CONSTANTS::LF_ACTION);
        if (reason)
        {
            return ServiceChatroomServer::MakeAnswerError(*reason, __func__, CONSTANTS::UNKNOWN);
        }
        std::string act_to_send = action->at(CONSTANTS::LF_ACTION);

        if (action->at(CONSTANTS::LF_DIRECTION) == CONSTANTS::RF_DIRECTION_CHATROOM)
        {
            auto reason = ServiceChatroomServer::CHK_Chr_CheckErrorsChatRoom(*action);
            if (reason)
            {
                return ServiceChatroomServer::MakeAnswerError(*reason, __func__, act_to_send);
            }
            // socket использоваться не будет, передан только в качестве реализации интерфейса
            ChatRoomSession session(room_.lock().get(), socket_);
            return session.ExecuteReadySession(action);
        }
        else if (action->at(CONSTANTS::LF_DIRECTION) == CONSTANTS::RF_DIRECTION_SERVER)
        {

            auto reason = ServiceChatroomServer::CHK_Chr_CheckErrorsChatRoom(*action);
            if (reason)
            {
                return ServiceChatroomServer::MakeAnswerError(*reason, __func__, act_to_send);
            }
            // socket использоваться не будет, передан только в качестве реализации интерфейса
            ServerSession session(room_.lock().get()->mainserv_, socket_);
            return session.ExecuteReadySession(action, socket_);
        }
        }
        catch(const std::exception &ex){
            return ServiceChatroomServer::MakeAnswerError("Exception Chatuser::ReadySesion", __func__, CONSTANTS::UNKNOWN); 
        }
    };
    
};

class Chatroom : public std::enable_shared_from_this<Chatroom>
{

    friend class MainServer;
    friend class ChatRoomSession;
    friend struct Chatuser;
    net::io_context &ioc_;
    MainServer *mainserv_;

    std::unordered_map<std::string, std::shared_ptr<Chatuser>> users_;
    std::condition_variable cond_;
    booltype modyfiing_users = false;
    booltype do_not_allow_modify_users = false;
    std::mutex mut_users_;
    std::mutex mut_send_;
    MessageManager msg_man_;

public:
    Chatroom(net::io_context &ioc, MainServer *serv) : ioc_(ioc), mainserv_(serv) {}

private:
    bool HasToken(const std::string &token);
    bool AddUser(shared_socket socket, std::string name, std::string token);
    void SendMessages(const std::string &token, const std::string &message);
    void DeleteUser(std::string token);
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

    net::io_context &ioc_;
    tcp::acceptor acceptor_;
    tcp::endpoint endpoint_;
    Service::TokenGen tokezier_;
    std::unordered_map<std::string, std::shared_ptr<Chatroom>> rooms_;
    Syncro sync_;
    const std::string CHECKPTR = "CHECK_PTR......................";

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
                return ServiceChatroomServer::MakeAnswerError("USER WITH THIS NICK ALREADY REGISTERED", __func__, CONSTANTS::ACT_CREATE_USER);
            };
            /*
            ЛОГИКА ЗАПРОСА К SQL И ДОБАВЛЕНИЕ В ОБЩИЙ СПИСОК??
            */
            return ServiceChatroomServer::Srv_MakeSuccessCreateUser(name);
        }
        catch (const std::exception &ex)
        {
            return ServiceChatroomServer::MakeAnswerError(ex.what(), __func__, CONSTANTS::ACT_CREATE_USER);
        }
    }
    //..................
public:
    void Listen();
    MainServer(net::io_context &ioc);
    void init();
    void PrintRooms();

private:
    // Действия
    std::string GetRoomUsersList(const std::string &roomname);
    std::string GetRoomsList();
    std::string CreateRoom(std::string room);
    std::string LoginUser(shared_task action, shared_socket socket);
};
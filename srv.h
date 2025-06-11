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
    AbstractSession(shared_stream stream)
        : stream_(stream)
    {
        ZyncPrint("ABSTRACTSESS: " + std::to_string(++exempslars) + " IS CONSTRUCTING");
    }
    shared_stream stream_ = nullptr;
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
    std::string ExecuteReadySession(shared_task action, shared_stream stream);
    std::string GetStringResponceToSocket(shared_task action) override;

public:
    ServerSession(MainServer *server, shared_stream stream)
        : AbstractSession(stream), server_(server)
    {
        ZyncPrint("SERVERSESS: " + std::to_string(++exempslars_s) + " IS CONSTRUCTING");
    };
};

class ChatRoomSession : public AbstractSession
{
    Chatroom *chatroom_ = nullptr;
    friend class Chatuser;

public:
    ChatRoomSession(Chatroom *chat, shared_stream stream)
        : AbstractSession(stream), chatroom_(chat) {};
    std::string GetStringResponceToSocket(shared_task action) override;

private:
    std::string ExecuteReadySession(shared_task action);
};

struct Chatuser : public std::enable_shared_from_this<Chatuser>
{
    Chatuser(std::weak_ptr<Chatroom> room, std::string name,
             shared_stream stream, net::io_context &io)
        : room_(room), name_(std::move(name)),
          stream_(stream), ioc_(io)
    {
        strand_ = Service::MakeSharedStrand(ioc_);
    }
    std::weak_ptr<Chatroom> room_;
    std::string name_;
    shared_stream stream_;
    net::io_context &ioc_;
    shared_strand strand_;
    beast::flat_buffer buffer_;
    request request_;
    std::atomic_int count_ = 0;

    void
    Run()
    {
        // We need to be executing within a strand to perform async operations
        // on the I/O objects in this session. Although not strictly necessary
        // for single-threaded contexts, this example code is written to be
        // thread-safe by default.
        net::dispatch(stream_ -> get_executor(),
                      beast::bind_front_handler(
                          &Chatuser::do_read,
                          shared_from_this()));
    }

    void
    do_read()
    {
        // Make the request empty before reading,
        // otherwise the operation behavior is undefined.
        request_ = {};

        // Set the timeout.
        stream_->expires_after(std::chrono::seconds(30));

        // Read a request
        http::async_read(*stream_, buffer_, request_,
                         beast::bind_front_handler(
                             &Chatuser::on_read,
                             shared_from_this()));
    }

    void
    on_read(beast::error_code ec, std::size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);
        // This means they closed the connection
        if (ec == http::error::end_of_stream)
            return do_close();

        if (ec){
          return do_close();
        }

       auto action = Service::ExtractSharedObjectsfromRequestOrResponce(request_);
       request_.clear();
       Service::PrintUmap(*action);
       auto resp_body = ExecuteReadySesion(action);
       auto res = Service::MakeResponce(11, true, http::status::ok, std::move(resp_body));
       send_response(std::move(res));
    };

    void
    send_response(response msg)
    {
        bool keep_alive = msg.keep_alive();

        // Write the response
        http::async_write(
            *stream_, std::move(msg), 
             beast::bind_front_handler(&Chatuser::on_write, shared_from_this(), keep_alive));
    }

    void
    on_write(
        bool keep_alive,
        beast::error_code ec,
        std::size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);

        if (ec)
            //   return fail(ec, "write");

            if (!keep_alive)
            {
                // This means we should close the connection, usually because
                // the response indicated the "Connection: close" semantic.
                return do_close();
            }

        // Read another request
        do_read();
    }

    void
    do_close()
    {
        // Send a TCP shutdown
        beast::error_code ec;
        stream_->socket().shutdown(tcp::socket::shutdown_send, ec);

        // At this point the connection is closed gracefully
    }

    /*
    void Read();
    void Run();
      */
    void IncomeMessage(response resp);
   
    std::string ExecuteReadySesion(shared_task action);
   
};

class Chatroom : public std::enable_shared_from_this<Chatroom>
{

    friend class MainServer;
    friend class ChatRoomSession;
    friend struct Chatuser;
    MainServer *mainserv_;
    std::string name_;
    
    std::unordered_map<std::string, std::shared_ptr<Chatuser>> users_;
    std::mutex mtx_;
    MessageManager msg_man_;

public:
    Chatroom(MainServer *serv , const std::string& name) :  mainserv_(serv),  name_(name) {}

private:
    bool HasToken(const std::string &token);
    bool AddUser(shared_stream stream, std::string name, std::string token);
    void SendMessages(const std::string &token , const std::string &name, const std::string &message);
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
    std::string LoginUser(shared_task action, shared_stream stream);
};
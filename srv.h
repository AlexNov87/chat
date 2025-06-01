#pragma once
#include "tokenizer.h"
#include "service.h"

#include <boost/asio/io_context.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/post.hpp>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/read_until.hpp>

#include <unordered_map>
#include <condition_variable>
#include <mutex>
#include <sstream>
#include <memory>
#include <iostream>

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

        void HandleExistingSocket(tcp::socket &socket, std::unordered_map<std::string, std::string> task);
        void HandleTaskFromServer(std::unordered_map<std::string, std::string> action, tcp::socket socket);

    private:
        void HandleAction(std::unordered_map<std::string, std::string> action, tcp::socket &socket);
    };

public:
    Chatroom() {}

    void HandleIncomeSocket(tcp::socket socket, std::unordered_map<std::string, std::string> action)
    {
        ioc_.post([&]()
                  { std::make_shared<ChatRoomSession>(this)->HandleTaskFromServer(action, std::move(socket)); });
    }

private:
    
    MainServer* mainserv_ ;
    friend class MainServer;
    friend class ChatRoomSession;
    net::io_context ioc_;
    std::unordered_map<std::string, Chatuser> users_;

    std::condition_variable cond_;
    bool modyfiing_users = false;
    bool do_not_allow_modify_users = false;
    std::mutex mut_users;

private:
    bool HasToken(const std::string &token);
    void AwaitSocket(std::string& token);

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

    bool IsAliveSocket(tcp::socket &sock);
    void AddUser(tcp::socket socket, std::string name, std::string token, std::string roomname);
    void SendMessages(std::string token, std::string message);
    void DeleteUser(std::string token);
    std::string RoomMembers();
};

class MainServer
{
    friend class Chatroom;
    friend class ServerSession;
    net::io_context ioc_;
    net::strand<net::io_context::executor_type> strand_ = net::make_strand(ioc_);
    tcp::endpoint ep_;
    std::shared_ptr<tcp::acceptor> acceptor_;

    class ServerSession : public std::enable_shared_from_this<ServerSession>
    {
       
       MainServer* server_;

    public:
      ServerSession(MainServer* server) : server_(server) {}
      void  HandleSession(tcp::socket socket){
           

      };
      void  HandleSessionFromExistSocket(Chatroom::Chatuser& chatuser){

      };
    private:
     // std::optional<std::unordered_map<std::string, std::string>> MakeTaskOrSendError()   
    
    };

public:
    void Run()
    {
    }

private:

   void init() {
        std::string load_var = "loadserv.conf";
        

   };
};


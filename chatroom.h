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

class ChatRoomSession;
class Chatroom
{
    enum class COMED_FROM
    {
        COMED_FROM_SERVER,
        COMED_FROM_OLD_SOCKET
    };

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
        bool HasToken(const std::string &token);
        void HandleAction(std::unordered_map<std::string, std::string> action, tcp::socket &socket, Chatroom::COMED_FROM from);
    };

public:
    Chatroom() {}

    void HandleIncomeSocket(tcp::socket socket, std::unordered_map<std::string, std::string> action)
    {
        ioc_.post([&]()
                  { std::make_shared<ChatRoomSession>(this)->HandleTaskFromServer(action, std::move(socket)); });
    }

private:
    friend class ChatRoomSession;
    net::io_context ioc_;
    std::unordered_map<std::string, Chatuser> users_;

    std::condition_variable cond_;
    bool modyfiing_users = false;
    bool do_not_allow_modify_users = false;
    std::mutex mut_users;

private:
    bool HasToken(const std::string &token)
    {
        // блокируем возможность подключение и удаления юзеров
        std::unique_lock<std::mutex> ul(mut_users);
        do_not_allow_modify_users = true;
        cond_.wait(ul, [&]()
                   { return modyfiing_users == false; });

        bool has_token = users_.contains(token);

        do_not_allow_modify_users = false;
        // оповещаем потоки
        cond_.notify_all();
        return has_token;
    }

    void AwaitSocket(tcp::socket &socket)
    {
        
        try
        {
            boost::asio::streambuf buffer;
            boost::asio::async_read_until(socket, buffer, '\0',  
                                          [&](boost::system::error_code ec, std::size_t length)
                                          {
                                              if (!ec)
                                              {
                                                  auto task = Service::GetTaskFromBuffer(buffer);
                                                  std::make_shared<ChatRoomSession>(this)->HandleExistingSocket(socket, task);
                                              }
                                              else
                                              {
                                                 Service::WriteErrorToSocket(socket, ec.message(), "AwaitSocket");
                                              }
                                          });
        }
        catch (const boost::system::system_error &err)
        {

            if (err.code() == boost::asio::error::eof)
            {
                std::cout << "Remote side closed connection (EOF)" << std::endl;
                // Корректное завершение работы с сокетом
            }
            else
            {
                Service::WriteErrorToSocket(socket, "Read Error: " + std::string(err.what()), CONSTANTS::RF_ERR_INITIATOR_CHATROOM);
            }
        }
    }

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

    bool IsAliveSocket(tcp::socket &sock)
    {
        boost::system::error_code ec;
        sock.non_blocking(true, ec);
        if (ec)
        {
            return false;
        }
        return true;
    }

    void AddUser(tcp::socket socket, std::string name, std::string token)
    {
        auto lam = [&]()
        {
            users_.insert({token, Chatuser(std::move(name), ioc_, std::move(socket))});
        };
        // Блокируем возможность слать по сокетам на время модификации списка юзеров
        MakeLockedModUsers(lam);

        // Если сокет "Не жив - выходим"
        if (!IsAliveSocket(users_.at(token).socket_))
        {
            DeleteUser(token);
            return;
        }

        // шлем юзеру токен
        net::post(users_.at(token).strand_, [&]()
                  { users_.at(token).socket_.write_some(net::buffer("Токен юзера")); });

        AwaitSocket(users_.at(token).socket_);
    }

    void SendMessages(std::string token, std::string message)
    {
        // блокируем возможность подключение и удаления юзеров
        std::unique_lock<std::mutex> ul(mut_users);
        do_not_allow_modify_users = true;
        cond_.wait(ul, [&]()
                   { return modyfiing_users == false; });
        for (auto &&[token, chatuser] : users_)
        {
            if (!IsAliveSocket(chatuser.socket_))
            {
                continue;
            }
            net::post(chatuser.strand_, [&]()
                      { users_.at(token).socket_.write_some(net::buffer(message)); });
        };
        // заканчиваем рассылку
        do_not_allow_modify_users = false;
        // оповещаем потоки
        cond_.notify_all();

        AwaitSocket(users_.at(token).socket_);
    }

    void DeleteUser(std::string token)
    {
        auto lam = [&]()
        {
            users_.erase(token);
        };
        MakeLockedModUsers(lam);
    }

    void RoomMembers(tcp::socket &socket, COMED_FROM from)
    {
        // Блокируем возможность рассылки по сокетам на время модификации списка
        std::unique_lock<std::mutex> ul(mut_users);
        // Запрещаем добавлять - удалять юзеров
        do_not_allow_modify_users = true;
        std::ostringstream oss;
        oss << "USERS:\n";
        for (auto &&[token, chatuser] : users_)
        {
            oss << chatuser.name_ << '\n';
        }
        do_not_allow_modify_users = false;
        // оповещаем потоки
        cond_.notify_all();

        socket.write_some(net::buffer(oss.str()));

        if (from == COMED_FROM::COMED_FROM_OLD_SOCKET)
        {
            AwaitSocket(socket);
        }
    }
};

/*
void Listenx(){
       try {
        // Создаём серверный сокет, слушающий порт 8080
        Poco::Net::ServerSocket serverSocket(80);

        std::cout << "Server started, waiting for connections..." << std::endl;

        while (true) {
            // Принимаем входящее соединение (блокирующий вызов)
            Poco::Net::StreamSocket clientSocket = serverSocket.acceptConnection();
            std::cout << "Client connected from " << clientSocket.peerAddress().toString() << std::endl;

            // Здесь можно читать/писать данные через clientSocket
            // ...
        }
    }
    catch (Poco::Exception& ex) {
        std::cerr << "Server error: " << ex.displayText() << std::endl;
    }
}


#include <Poco/Net/HTTPClientSession.h>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/URI.h>
#include <Poco/Net/ServerSocket.h>
#include <Poco/Net/StreamSocket.h>
*/

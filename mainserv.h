#pragma once
#include "chatroom.h"



class MainServer
{
    friend class Chatroom;
    friend class ServerSession;
    net::io_context ioc_;
    net::strand<net::io_context::executor_type> strand_ = net::make_strand(ioc_);
    tcp::endpoint ep_;
    std::shared_ptr<tcp::acceptor> acceptor_;

    class ServerSession
    {
       ServerSession(MainServer& server) : server_(server) {}
       MainServer& server_;

    public:
      void  HandleSession(tcp::socket socket){};
      void  HandleSession(const std::string token, Chatroom::Chatuser& chatuser){

      };

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
#pragma once
#include "service.h"
#include "tokenizer.h"
#include <windows.h>

class TestCli : public std::enable_shared_from_this<TestCli>
{
public:
    TestCli(net::io_context &ioc) : ioc_(ioc)
    {
     socket_  = Service::MakeSharedSocket(ioc_);
     strand_ = Service::MakeSharedStrand(ioc_);
     InitSocket();
    }

    void Run() {
        std::jthread([self = shared_from_this()]{self->Read();});
         
        
        
         std::jthread([self = shared_from_this()]{
           self-> PrintAct();
            self->Choise();});
    };
private:
    net::io_context &ioc_;
    shared_socket socket_; 
    shared_strand strand_;  
    strand &post_ = *strand_;
    err ec;

    tcp::endpoint endpoint_{tcp::endpoint(net::ip::make_address("127.0.0.1"), 1601)};
    std::string token_;
    Service::TokenGen gen_;

    void Do(std::string act)
    {
        err ec;
        auto req = Service::MakeRequest(http::verb::get, 11, UserInterface::US_SrvMakeObjLogin("RRAT", "jijjiw", "YANDEX"));
        ZyncPrint("WRITE.............");
        http::write(*socket_, req, ec);
    };

    std::vector<std::pair<std::string, std::string>> bodies_{
        {"SEND MSG", UserInterface::US_ChrMakeSendMessage(gen_.GenerateHEXToken(), "22sssssssswdqeefvqwef222222222222222222222222222222")},
        {"DISCONNECT", UserInterface::US_ChrMakeObjDisconnect(gen_.GenerateHEXToken())},
        {"CREATEROOM", UserInterface::US_SrvMakeObjCreateRoom("YANDEX!")},
        {"CREATEUSER", UserInterface::US_SrvMakeObjCreateUser("RRAT", "hjsjklk;l")},
        {"GETUSERS", UserInterface::US_SrvMakeObjGetUsers("YANDEX")},
        {"LOGIN", UserInterface::US_SrvMakeObjLogin("RRAT", "jijjiw", "YANDEX")},
        {"ROOMLIST", UserInterface::US_SrvMakeObjRoomList()}};

    void Choise()
    {
        while (1)
        {
            if (GetAsyncKeyState('1') & 0x8000)
            {
                ZyncPrint("1 PRESSED");
                net::post(post_, [self = shared_from_this()]
                          { ZyncPrint("1 DOING");
                            self->Do(self->bodies_[0].second); });
            };
            if (GetAsyncKeyState('2') & 0x8000)
            {
                net::post(post_, [self = shared_from_this()]
                          { self->Do(self->bodies_[1].second); });
            };
            if (GetAsyncKeyState('3') & 0x8000)
            {
                net::post(post_, [self = shared_from_this()]
                          { self->Do(self->bodies_[2].second); });
            };
            if (GetAsyncKeyState('4') & 0x8000)
            {
                net::post(post_, [self = shared_from_this()]
                          { self->Do(self->bodies_[3].second); });
            };
            if (GetAsyncKeyState('5') & 0x8000)
            {
                net::post(post_, [self = shared_from_this()]
                          { self->Do(self->bodies_[4].second); });
            };
            if (GetAsyncKeyState('6') & 0x8000)
            {
                net::post(post_, [self = shared_from_this()]
                          { self->Do(self->bodies_[5].second); });
            };
            if (GetAsyncKeyState('7') & 0x8000)
            {
                net::post(post_, [self = shared_from_this()]
                          { self->Do(self->bodies_[6].second); });
            };
        }
    };

    void PrintAct()
    {
        for (int i = 0; i < bodies_.size(); ++i)
        {
            ZyncPrint(std::to_string(i + 1) + " --> " + bodies_[i].first);
        }
    }

    void InitSocket()
    {
        socket_->open(endpoint_.protocol(), ec);
        if (ec)
        {
            std::cerr << "Open error: " << ec.message() << std::endl;
        }

        socket_->set_option(boost::asio::socket_base::reuse_address(true), ec);
        if (ec)
        {
            std::cerr << "Set option error: " << ec.message() << std::endl;
        }

        socket_->connect(endpoint_, ec);
        if (ec)
        {
            std::cerr << "COnnect error: " << ec.message() << std::endl;
        }
    }

    void Read()
    {
        ZyncPrint("READIN.............");
        auto sb = Service::MakeSharedFlatBuffer();
         ZyncPrint("RESPONCE.............");
        std::shared_ptr<response> req = std::make_shared<response>();
         ZyncPrint("SHFT.............");
        auto self = shared_from_this();
         ZyncPrint("SHFTX.............");

        
        auto handler = [sb, req, self](err ec, size_t bytes)
        {
            ZyncPrint("LAMPDA READ...............");
            if (ec)
            {
                ZyncPrint(ec.what());
                system("pause");
                return;
            }

            // ZyncPrint("->" + Service::ExtractStrFromStreambuf(*sb, bytes) + "<-");
            auto i = Service::ExtractSharedObjectsfromRequestOrResponce(*req);
            Service::PrintUmap(*i);
            sb->consume(bytes);
            self->Read();
        };
        ZyncPrint("READINASYNC.............");
        http::async_read(*socket_, *sb, *req, handler);
    }
};
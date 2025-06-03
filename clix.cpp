#include "service.h"
#include "tokenizer.h"
#include <boost/asio.hpp>
#include <iostream>

namespace net = boost::asio;
using tcp = net::ip::tcp;

void test1()
{
    try
    {
        net::io_context ioc(16);
        tcp::socket socket{ioc};
        boost::system::error_code ec;
        auto endpoint = tcp::endpoint(net::ip::make_address("127.0.0.1"), 80);

        socket.open(endpoint.protocol(), ec);
        if (ec)
        {
            std::cerr << "Open error: " << ec.message() << std::endl;
        }

        socket.set_option(boost::asio::socket_base::reuse_address(true), ec);
        if (ec)
        {
            std::cerr << "Set option error: " << ec.message() << std::endl;
        }

        socket.connect(endpoint, ec);
        if (ec)
        {
            std::cerr << "COnnect error: " << ec.message() << std::endl;
        }

        task mp;
        std::optional<std::string> reas = std::nullopt;
        std::string str = "";

        Service::TokenGen gen;
        for (int i = 0; i < 5; ++i)
        {

            // // boost::asio::io_context ioc;
            str = UserInterface::US_ChrMakeSendMessage(gen.GenerateHEXToken(), "22sssssssswdqeefvqwef222222222222222222222222222222");
            socket.write_some(net::const_buffer(str.data(), str.size()));
            net::post(ioc, [&]()
                      { socket.write_some(net::const_buffer(str.data(), str.size())); });

            str = UserInterface::US_ChrMakeObjDisconnect(gen.GenerateHEXToken());
            net::post(ioc, [&]()
                      { socket.write_some(net::const_buffer(str.data(), str.size())); });

            str = UserInterface::US_SrvMakeObjCreateRoom("YANDEX");
            net::post(ioc, [&]()
                      { socket.write_some(net::const_buffer(str.data(), str.size())); });

            str = UserInterface::US_SrvMakeObjCreateUser("RRAT", "hjsjklk;l");
            net::post(ioc, [&]()
                      { socket.write_some(net::const_buffer(str.data(), str.size())); });

            str = UserInterface::US_SrvMakeObjGetUsers("RRAT");
            net::post(ioc, [&]()
                      { socket.write_some(net::const_buffer(str.data(), str.size())); });

            str = UserInterface::US_SrvMakeObjLogin("RRAT", "jijjiw", "kjjolpdpw");
            net::post(ioc, [&]()
                      { socket.write_some(net::const_buffer(str.data(), str.size())); });

            str = UserInterface::US_SrvMakeObjRoomList();
            net::post(ioc, [&]()
                      { socket.write_some(net::const_buffer(str.data(), str.size())); });
            Service::MtreadRunContext(ioc);
        }
    }
    catch (const std::exception &ex)
    {
        std::cout << ex.what();
        system("pause");
    }

    // ioc.run();
}

int main()
{

    test1();

    /// system("pause");
}
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
    
    std::ofstream ofs("log!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!.txt"); 


    net::io_context ioc(16);
    tcp::socket socket{net::make_strand(ioc)};
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

    // boost::asio::io_context ioc;
    str = UserInterface::US_ChrMakeSendMessage(gen.GenerateHEXToken(), "22sssssssswdqeefvqwef222222222222222222222222222222");
    ofs << "->" << str << "<-\n\n\n";
    net::write(socket, net::buffer(std::move(str))); ;
    ioc.run();

    str = UserInterface::US_ChrMakeObjDisconnect(gen.GenerateHEXToken());
    net::write(socket, net::buffer(std::move(str))); 
    ofs << "->" << str << "<-\n\n\n";
    ioc.run();
  
     
    str = UserInterface::US_SrvMakeObjCreateRoom("YANDEX");
     net::write(socket, net::buffer(std::move(str)));
     ofs << "->" << str << "<-\n\n\n";
     ioc.run();

    
    str = UserInterface::US_SrvMakeObjCreateUser("RRAT", "hjsjklk;l");
    net::write(socket, net::buffer(std::move(str))); 
     ofs << "->" << str << "<-\n\n\n";
     ioc.run();

 
     

    str = UserInterface::US_SrvMakeObjGetUsers("RRAT");
   net::write(socket, net::buffer(std::move(str)));
       ofs << "->" << str << "<-\n\n\n";
       ioc.run();

    str = UserInterface::US_SrvMakeObjLogin("RRAT", "jijjiw", "kjjolpdpw");
   net::write(socket, net::buffer(std::move(str)));
      ofs << "->" << str << "<-\n\n\n";
      ioc.run();

    str = UserInterface::US_SrvMakeObjRoomList();
     net::write(socket, net::buffer(std::move(str))); 
   ofs << "->" << str << "<-\n\n\n";
   ioc.run();

      // Service::MtreadRunContext(ioc);
     
  }
  catch (const std::exception &ex)
  {
    std::cout << ex.what();
    system("pause");
  }

}

int main()
{
 for(int i = 0; i < 100; ++ i){
    test1();
 }
  
  /// system("pause");
}

/*
net::post(ioc, [&]()
              { net::write(socket, net::buffer(std::move(str))); });
              

    str = UserInterface::US_ChrMakeObjDisconnect(gen.GenerateHEXToken());
    net::post(ioc, [&]()
              { net::write(socket, net::buffer(std::move(str))); });

    str = UserInterface::US_SrvMakeObjCreateRoom("YANDEX");
    net::post(ioc, [&]()
              { net::write(socket, net::buffer(std::move(str))); });

    str = UserInterface::US_SrvMakeObjCreateUser("RRAT", "hjsjklk;l");
    net::post(ioc, [&]()
              { net::write(socket, net::buffer(std::move(str))); });

    str = UserInterface::US_SrvMakeObjGetUsers("RRAT");
    net::post(ioc, [&]()
              { net::write(socket, net::buffer(std::move(str))); });

    str = UserInterface::US_SrvMakeObjLogin("RRAT", "jijjiw", "kjjolpdpw");
    net::post(ioc, [&]()
              { net::write(socket, net::buffer(std::move(str))); });

    str = UserInterface::US_SrvMakeObjRoomList();
    net::post(ioc, [&]()
              { net::write(socket, net::buffer(std::move(str))); });
*/
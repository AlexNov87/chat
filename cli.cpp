#include "service.h"
#include "tokenizer.h"
#include <boost/asio.hpp>
#include <iostream>

namespace net = boost::asio;
using tcp = net::ip::tcp;

std::shared_ptr<std::ofstream> ofs = std::make_shared<std::ofstream>("log!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!.txt");

net::io_context ioc(16);
shared_strand strand__ = Service::MakeSharedStrand(ioc);
shared_socket socket__ = Service::MakeSharedSocket(ioc);
boost::system::error_code ec;
auto endpoint = tcp::endpoint(net::ip::make_address("127.0.0.1"), 80);
std::mutex mtx;

void InitSocket(){
  socket__->open(endpoint.protocol(), ec);
    if (ec)
    {
      std::cerr << "Open error: " << ec.message() << std::endl;
    }

    socket__->set_option(boost::asio::socket_base::reuse_address(true), ec);
    if (ec)
    {
      std::cerr << "Set option error: " << ec.message() << std::endl;
    }

    socket__->connect(endpoint, ec);
    if (ec)
    {
      std::cerr << "COnnect error: " << ec.message() << std::endl;
    }
}

std::shared_ptr<std::vector<std::string>> GetStringValues()
{
  Service::TokenGen gen;
  std::vector<std::string> objs{
      UserInterface::US_ChrMakeSendMessage(gen.GenerateHEXToken(), "22sssssssswdqeefvqwef222222222222222222222222222222"),
      UserInterface::US_ChrMakeObjDisconnect(gen.GenerateHEXToken()),
      UserInterface::US_SrvMakeObjCreateRoom("YANDEX"),
      UserInterface::US_SrvMakeObjCreateUser("RRAT", "hjsjklk;l"),
      UserInterface::US_SrvMakeObjGetUsers("RRAT"),
      UserInterface::US_SrvMakeObjLogin("RRAT", "jijjiw", "kjjolpdpw"),
      UserInterface::US_SrvMakeObjRoomList()

  };
  return std::make_shared<std::vector<std::string>>(std::move(objs));
}
std::atomic_int cnt;
auto values = GetStringValues();


void Read()
{
  auto sb = Service::MakeSharedStreambuf();
  auto handler = [sb](err ec, size_t bytes)
  {
    if (ec)
    {
      ZyncPrint(ec.what());
      system("pause");
      return;
      
    }
    
    //ZyncPrint("->" + Service::ExtractStrFromStreambuf(*sb, bytes) + "<-");
    auto i = Service::ExtractObjectsfromBuffer(*sb,bytes);
    Service::PrintUmap(i, *ofs);
    sb->consume(bytes);
   
    
   
  };
  net::async_read_until(*socket__, *sb, CONSTANTS::SERIAL_SYM , handler);
}

void test1()
{

  try
  {
    auto buf = Service::MakeSharedStreambuf(); 
    
    for (auto &&str : *values)
    {
    
      net::async_write(*socket__, net::buffer(str), [](err ec, size_t bytes){
                  Read();
      });
     
      
    }

  }
  catch (const std::exception &ex)
  {
    std::cout << ex.what();
    system("pause");
  }
}


void 
test2(){
  std::vector<std::string> o{
     
     // UserInterface::US_ChrMakeSendMessage(gen.GenerateHEXToken(), "22sssssssswdqeefvqwef222222222222222222222222222222"),
     // UserInterface::US_ChrMakeObjDisconnect(gen.GenerateHEXToken()),
      UserInterface::US_SrvMakeObjCreateRoom("YANDEX"),
      UserInterface::US_SrvMakeObjCreateUser("RRAT", "hjsjklk;l"),
      UserInterface::US_SrvMakeObjGetUsers("YANDEX"),
    //  UserInterface::US_SrvMakeObjLogin("RRAT", "jijjiw", "kjjolpdpw"),
      UserInterface::US_SrvMakeObjRoomList()

  };
  
  for(int q = 0; q< 2; ++q){
   for(int i = 0; i< o.size(); ++i){
      net::write(*socket__, net::buffer(o[i]));
   }
  }

}


int main()
{
   InitSocket();  // сначала подключаемся
   Read();       // затем запускаем чтение
   
     for(int q = 0; q< 2;){
   test1();
   }
  
  
  //запуск ioc
  Service::MtreadRunContext(ioc);
}
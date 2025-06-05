#include "service.h"
#include "tokenizer.h"
#include <boost/asio.hpp>
#include <iostream>

namespace net = boost::asio;
using tcp = net::ip::tcp;

std::shared_ptr<std::ofstream> ofs = std::make_shared<std::ofstream>("log!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!.txt");
net::io_context ioc(16);

shared_strand strand__ = Service::MakeSharedStrand(ioc);
shared_socket socket__ = Service::MakeSharedSocket(ioc);
boost::system::error_code ec;
auto endpoint = tcp::endpoint(net::ip::make_address("127.0.0.1"), 80);

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

void Read(std::shared_ptr<net::streambuf> sb, shared_socket socket)
{

  auto handler = [sb, socket](err ec, size_t bytes)
  {
    if (ec)
    {
      ZyncPrint(ec.what());
      system("pause");
      return;
    }

    auto actions = Service::ExtractObjectsfromBuffer(*sb);
    for(auto&& i : actions){
       Service::PrintUmap(i);
    }

    sb->consume(bytes);
    Read(sb, socket);

  };
  net::async_read_until(*socket__, *sb, '\0', handler);
}

void test1()
{

  try
  {
    auto values = GetStringValues();
    for (auto &&str : *values)
    {
      net::async_write(*socket__, net::buffer(str), [](err ec, size_t bytes) {

      });
    }

  }
  catch (const std::exception &ex)
  {
    std::cout << ex.what();
    system("pause");
  }
}

int main()
{
  
  InitSocket();
  auto sb = Service::MakeSharedStreambuf();
  Read(sb, socket__);
  
  for (int i = 0; i < 1; ++i)
  {
     net::post(ioc,[]{test1();});
  }
  Service::MtreadRunContext(ioc);
}

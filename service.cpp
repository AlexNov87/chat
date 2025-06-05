#include "service.h"
using namespace std;

namespace Service
{

    const std::unordered_map<std::string, ACTION> Additional::action_scernario{
        {CONSTANTS::ACT_CREATE_ROOM, ACTION::CREATE_ROOM},
        {CONSTANTS::ACT_CREATE_USER, ACTION::CREATE_USER},
        {CONSTANTS::ACT_DISCONNECT, ACTION::DISCONNECT},
        {CONSTANTS::ACT_GET_USERS, ACTION::GET_USERS},
        {CONSTANTS::ACT_LOGIN, ACTION::LOGIN},
        {CONSTANTS::ACT_ROOM_LIST, ACTION::ROOM_LIST},
        {CONSTANTS::ACT_SEND_MESSAGE, ACTION::SEND_MESSAGE}};

    const std::unordered_set<std::string> Service::Additional::chatroom_actions{
        CONSTANTS::ACT_DISCONNECT, CONSTANTS::ACT_SEND_MESSAGE};

    const std::unordered_set<std::string> Service::Additional::server_actions{
        CONSTANTS::ACT_CREATE_ROOM, CONSTANTS::ACT_CREATE_USER,
        CONSTANTS::ACT_GET_USERS, CONSTANTS::ACT_LOGIN, CONSTANTS::ACT_ROOM_LIST};

    const std::unordered_set<std::string> Service::Additional::request_directions{
        CONSTANTS::RF_DIRECTION_SERVER, CONSTANTS::RF_DIRECTION_CHATROOM};

        std::mutex Service::Additional::mtx;

    void MtreadRunContext(net::io_context &ioc)
    {
        std::vector<jthread> run;
        for (int i = 0; i < std::thread::hardware_concurrency(); ++i)
        {
            run.push_back(jthread([&ioc]
                                  { ioc.run(); }));
        }
        ioc.run();
    }

    std::string ReadFromFstream(std::ifstream &ifs)
    {
        if (!ifs)
        {
            std::cerr << "Bad Filestream\n";
            return CONSTANTS::RF_ERROR;
        }
        std::stringstream strm;

        while (ifs)
        {
            std::string tmp;
            std::getline(ifs, tmp);
            strm << tmp;
        }
        return strm.str();
    };

    shared_strand MakeSharedStrand(net::io_context &ioc)
    {
        return std::make_shared<strand>(net::make_strand(ioc));
    }
    std::shared_ptr<MutableBufferHolder> MakeSharedMutableGuffer()
    {
       return std::shared_ptr<Service::MutableBufferHolder>();
    };
    std::shared_ptr<net::streambuf> MakeSharedStreambuf()
    {
        return std::make_shared<net::streambuf>();
    };

}

namespace Service
{

    bool IsAliveSocket(tcp::socket &sock)
    {
        if (!sock.is_open())
        {
            return false;
        }
        boost::system::error_code ec;
        char data;
        size_t len = sock.receive(boost::asio::buffer(&data, 1), boost::asio::socket_base::message_peek, ec);

        if (ec)
        {

            if (ec == boost::asio::error::would_block || boost::asio::error::try_again)
            {
                return true;
            }
            if (ec == boost::asio::error::eof)
            {
                return false;
            }
            else
            {
                return false;
            }
        }
        // Данные доступны для чтения — соединение живо
        return len > 0;
    }

    bool IsAliveSocket(shared_socket sock)
    {
        return IsAliveSocket(*sock);
    };

    void ShutDownSocket(tcp::socket &sock)
    {
        boost::system::error_code ec;
        sock.cancel(ec);
        if (ec)
        {
            std::cout << ec.what() << std::endl;
        }
        sock.shutdown(sock.shutdown_both, ec);
        if (ec)
        {
            std::cout << ec.what() << std::endl;
        }
        sock.close();
        if (ec)
        {
            std::cout << ec.what() << std::endl;
        }
    };
    void ShutDownSocket(shared_socket sock)
    {
        ShutDownSocket(*sock);
    };
}

namespace Service
{
    std::string ExtractStrFromStreambuf(net::streambuf &buffer, size_t extract)
    {
        const char *data = boost::asio::buffer_cast<const char *>(buffer.data());
        return std::string(data, extract);
    }

    

    task ExtractObjectsfromBuffer(net::streambuf &buffer, size_t extract)
    {
        std::string str(ExtractStrFromStreambuf(buffer, extract));
        return DeserializeUmap<std::string,std::string>(str);
    };
    shared_task ExtractSharedObjectsfromBuffer(net::streambuf &buffer, size_t extract)
    {
        std::string str(ExtractStrFromStreambuf(buffer, extract));
        return std::make_shared<task>(DeserializeUmap<std::string,std::string>(str));
    };

}

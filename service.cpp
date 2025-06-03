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

     shared_strand MakeSharedStrand(net::io_context& ioc){
        return std::make_shared<strand>(net::make_strand(ioc));
    }
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
    std::string ExtractStrFromStreambuf(net::streambuf &buffer)
    {
        const char *data = boost::asio::buffer_cast<const char *>(buffer.data());
        std::size_t size = buffer.size();
        return std::string(data, buffer.size());
    }

    std::optional<std::string> GetTaskFromSocket(tcp::socket &socket)
    {
        boost::system::error_code ec;
        if (!Service::IsAliveSocket(socket))
        {
            // std::cerr << "THE SOCKET IS NOT ALIVE FOO\n";
            return std::nullopt;
        }
        net::streambuf buffer;
        auto sym_read = net::read(socket, buffer, ec);
        if (sym_read == 0)
        {
            return std::nullopt;
        }
        if (ec && ec != net::error::eof)
        {
            // std::cout << "RETURNING GLOBAL EWRROR\n";
            std::cout << ec.what() << '\n';
            return std::nullopt;
        }
        return ExtractStrFromStreambuf(buffer);
    }

    std::vector<task> ExtractObjectsFromStream(std::stringstream &strm)
    {

        std::vector<task> tasks;
        while (strm)
        {
            std::string tmp;
            std::getline(strm, tmp, '\0');
            if (tmp.empty())
            {
                break;
            }
            auto task = Service::DeserializeUmap<std::string, std::string>(std::move(tmp));
            tasks.push_back(std::move(task));
        };
        return tasks;
    }

    std::vector<shared_task> ExtractSharedObjectsFromStream(std::stringstream &strm)
    {
        std::vector<shared_task> tasks;
        while (strm)
        {
            std::string tmp;
            std::getline(strm, tmp, '\0');
            if (tmp.empty())
            {
                break;
            }
            auto action = std::make_shared<task>(Service::DeserializeUmap<std::string, std::string>(std::move(tmp)));
            tasks.push_back(std::move(action));
        };
        return tasks;
    }

    std::vector<task> ExtractObjectsfromBuffer(net::streambuf &buffer)
    {
        std::stringstream strm(ExtractStrFromStreambuf(buffer));
        return ExtractObjectsFromStream(strm);
    };
    std::vector<shared_task> ExtractSharedObjectsfromBuffer(net::streambuf &buffer)
    {
        std::stringstream strm(ExtractStrFromStreambuf(buffer));
        return ExtractSharedObjectsFromStream(strm);
    };

    std::vector<task> ExtractObjectsfromSocket(tcp::socket &socket)
    {
        std::stringstream oss;
        while (auto action = Service::GetTaskFromSocket(socket))
        {
            oss << *action;
        };
        return ExtractObjectsFromStream(oss);
    };

    std::vector<shared_task> ExtractSharedObjectsfromSocket(tcp::socket &socket)
    {
        std::stringstream oss;
        while (auto action = Service::GetTaskFromSocket(socket))
        {
            oss << *action;
        };
        return ExtractSharedObjectsFromStream(oss);
    };

}

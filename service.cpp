#include "service.h"
using namespace std;

namespace Service
{

    void MtreadRunContext(net::io_context &ioc)
    {
        for (int i = 0; i < std::thread::hardware_concurrency(); ++i)
        {
            jthread jth([&ioc]
                        { ioc.run(); });
        }
    }

    std::unordered_map<std::string, std::string> GetTaskFromBuffer(net::streambuf &buffer)
    {
        const char *data = boost::asio::buffer_cast<const char *>(buffer.data());
        std::size_t size = buffer.size();
        std::string ln(data, size - 1);
        std::unordered_map<std::string, std::string> task =
            Service::DeserializeUmap<std::string, std::string>(ln);
        TrimContainer(task);
        return task;
    }

    void TrimContainer(std::unordered_map<std::string, std::string> &action)
    {
        for (auto &pair : action)
        {
            boost::algorithm::trim(pair.second);
        }
    }

    const std::unordered_map<std::string, ACTION> Additional::action_scernario{
        {CONSTANTS::ACT_CONNECT, ACTION::CONNECT},
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
        CONSTANTS::ACT_GET_USERS, CONSTANTS::ACT_CONNECT};

    const std::unordered_set<std::string> Service::Additional::request_directions{
        CONSTANTS::RF_DIRECTION_SERVER, CONSTANTS::RF_DIRECTION_CHATROOM};
}






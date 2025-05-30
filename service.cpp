#include "service.h"
using namespace std;

const std::string CONSTANTS::ACT_CONNECT = "CONNECT";
const std::string CONSTANTS::ACT_DISCONNECT = "DISCONNECT";
const std::string CONSTANTS::ACT_GET_USERS = "GET_USERS";
const std::string CONSTANTS::ACT_SEND_MESSAGE = "SEND_MESSAGE";

// Map Parameters
const std::string CONSTANTS::LF_ACTION = "ACTION";
const std::string CONSTANTS::LF_NAME = "NAME";
const std::string CONSTANTS::LF_TOKEN = "TOKEN";
const std::string CONSTANTS::LF_MESSAGE = "MESSAGE";
const std::string CONSTANTS::LF_REASON = "REASON";
const std::string CONSTANTS::LF_RESULT = "RESULT";
const std::string CONSTANTS::LF_INITIATOR = "INITIATOR";

const std::string CONSTANTS::RF_SUCCESS = "SUCCESS";
const std::string CONSTANTS::RF_ERROR = "ERROR";

//
const std::string CONSTANTS::RF_ERR_INITIATOR_SERVER = "INITIATOR_SERVER";
const std::string CONSTANTS::RF_ERR_INITIATOR_CHATROOM = "INITIATOR_CLENT";
const std::string CONSTANTS::RF_ERR_PERMISSION_DENIDED = "YOUR TOKEN INCORRECT. PERMISSION DENIDED";


const size_t CONSTANTS::N_CONNECT = 3 - 1;
const size_t CONSTANTS::N_DISCONNECT = 3 - 1;
const size_t CONSTANTS::N_GET_USERS = 2 - 1;
const size_t CONSTANTS::N_SEND_MESSAGE = 4 - 1;
const size_t CONSTANTS::N_TOKEN_LEN = 32;
const size_t CONSTANTS::N_MAX_MESSAGE_LEN = 512;

namespace Service
{

    std::string ChrMakeObjConnect(std::string name)
    {
        unordered_map<string, string> res{
            {CONSTANTS::LF_ACTION, CONSTANTS::ACT_CONNECT},
            {CONSTANTS::LF_NAME, std::move(name)}};
        return SerializeUmap(res);
    };
    std::string ChrMakeObjDisconnect(std::string token)
    {
        unordered_map<string, string> res{
            {CONSTANTS::LF_ACTION, CONSTANTS::ACT_DISCONNECT},
            {CONSTANTS::LF_TOKEN, token}

        };
        return SerializeUmap(res);
    };
    std::string ChrMakeObjGetUsers()
    {
        unordered_map<string, string> res{
            {CONSTANTS::LF_ACTION, CONSTANTS::ACT_GET_USERS},
        };
        return SerializeUmap(res);
    };
    std::string ChrMakeSendMessage(std::string token, std::string message)
    {
        unordered_map<string, string> res{
            {CONSTANTS::LF_ACTION, CONSTANTS::ACT_SEND_MESSAGE},
            {CONSTANTS::LF_TOKEN, token},
            {CONSTANTS::LF_MESSAGE, message}};
        return SerializeUmap(res);
    };

    std::string ChrMakeAnswerError(std::string reason, string initiator)
    {
        unordered_map<string, string> res{
            {CONSTANTS::LF_RESULT, CONSTANTS::RF_ERROR},
            {CONSTANTS::LF_REASON, std::move(reason)},
            {CONSTANTS::LF_INITIATOR, std::move(initiator)}};
        return Service::SerializeUmap(res);
    };

    void MtreadRunContext(net::io_context &ioc)
    {
        for (int i = 0; i < std::thread::hardware_concurrency(); ++i)
        {
            jthread jth([&ioc]
                        { ioc.run(); });
        }
    }

    void WriteErrorToSocket(tcp::socket &socket, std::string reason,std::string initiator){
        
         Service::DoubleGuardedExcept<void>(
          [&](){
         socket.write_some(net::buffer(ChrMakeAnswerError(reason,initiator)));
         }, "WriteErrorToSocket" );
    };

     std::unordered_map<std::string, std::string> GetTaskFromBuffer(net::streambuf& buffer)
    {
        const char *data = boost::asio::buffer_cast<const char *>(buffer.data());
        std::size_t size = buffer.size();
        std::string ln(data, size - 1);
        std::unordered_map<std::string, std::string> task =
            Service::DeserializeUmap<std::string, std::string>(ln);

        return task;
    }

    const std::unordered_map<std::string, ACTION> Additional::action_scernario{
        {CONSTANTS::ACT_CONNECT, ACTION::CONNECT},
        {CONSTANTS::ACT_DISCONNECT, ACTION::DISCONNECT},
        {CONSTANTS::ACT_SEND_MESSAGE, ACTION::SEND_MESSAGE},
        {CONSTANTS::ACT_GET_USERS, ACTION::GET_USERS}};

    const std::unordered_set<std::string> Service::Additional::chatroom_actions{
        CONSTANTS::ACT_DISCONNECT, CONSTANTS::ACT_SEND_MESSAGE, CONSTANTS::ACT_GET_USERS};
}

namespace Service_Chatroom
{

    ///@brief Проверяет валидность действия контейнера
    std::optional<std::string> ActionReasonIncorrect(const std::unordered_map<std::string, std::string> &action);
    ///@brief Проверяет валидность контейнера действия подключения
    std::optional<std::string> ActionConnectIncorrect(std::unordered_map<std::string, std::string> &action);
    ///@brief Проверяет валидность контейнера действия послания сообщения
    std::optional<std::string> ActionSendMessageIncorrect(std::unordered_map<std::string, std::string> &action);
    ///@brief Проверяет валидность контейнера действия отключения
    std::optional<std::string> ActionDisconnectIncorrect(const std::unordered_map<std::string, std::string> &action);
    ///@brief Проверяет размер контейнера
    std::optional<std::string> SizeActionIncorrect(const std::unordered_map<std::string, std::string> &action, size_t size);
    ///@brief Проверяет валидность токена
    std::optional<std::string> TokenIncorrect(const std::unordered_map<std::string, std::string> &action);
    ///@brief Проверяет валидность токена и размера
    std::optional<std::string> ActionSizeAndTokenIncorrect(const std::unordered_map<std::string, std::string> &action, size_t size);
 
    std::optional<std::string> CheckErrorsChatRoom(std::unordered_map<std::string, std::string> &action)
    {
        auto err = ActionReasonIncorrect(action);
        if (err)
        {
            return *err;
        }

        auto testcase = Service::Additional::action_scernario.at(action.at(CONSTANTS::LF_ACTION));
        switch (testcase)
        {
        case Service::ACTION::DISCONNECT:
        {
            err = ActionDisconnectIncorrect(action);
        }
        break;

        case Service::ACTION::SEND_MESSAGE:
        {
            err = ActionSendMessageIncorrect(action);
        }
        break;

        case Service::ACTION::GET_USERS:
        {
            err = SizeActionIncorrect(action, CONSTANTS::N_GET_USERS);
        }
        break;
        }
        if (err)
        {
            return *err;
        }

        return std::nullopt;
    };

    std::optional<std::string> SizeActionIncorrect(const std::unordered_map<std::string, std::string> &action, size_t size)
    {
        if (action.size() != size)
        {
            return "SIZE OF " + action.at(CONSTANTS::LF_ACTION) + " IS INCORRECT EXPECTED: " + std::to_string(size);
        }
        return std::nullopt;
    }

    std::optional<std::string> TokenIncorrect(const std::unordered_map<std::string, std::string> &action)
    {
        if (!action.contains(CONSTANTS::LF_TOKEN))
        {
            return "NO TOKEN FIELD";
        }

        if (!action.at(CONSTANTS::LF_TOKEN).size() != CONSTANTS::N_TOKEN_LEN)
        {
            return "SIZE OF TOKEN: " + std::to_string(action.at(CONSTANTS::LF_TOKEN).size()) + " IS INCORRECT";
        }
        return std::nullopt;
    }


    std::optional<std::string> ActionSizeAndTokenIncorrect(const std::unordered_map<std::string, std::string> &action, size_t size){

        auto size_reason = SizeActionIncorrect(action, size);
        if (size_reason)
        {
            return *size_reason;
        }
        
        auto token_error = TokenIncorrect(action);
        if (token_error)
        {
            return *token_error;
        }
        return std::nullopt;
    };

    std::optional<std::string> ActionReasonIncorrect(const std::unordered_map<std::string, std::string> &action)
    {
        if (!action.contains(CONSTANTS::LF_ACTION))
        {
            return "NO ACTION FIELD";
        }
        if (action.at(CONSTANTS::LF_ACTION).empty())
        {
            return "EMPTY ACTION";
        }
        if (!Service::Additional::chatroom_actions.contains(action.at(CONSTANTS::LF_ACTION)))
        {
            return "ACTION: " + action.at(CONSTANTS::LF_ACTION) + " IS UNRECOGNIZED";
        }
        return std::nullopt;
    }

    std::optional<std::string> ActionConnectIncorrect(std::unordered_map<std::string, std::string> &action)
    {
        auto reason = ActionSizeAndTokenIncorrect(action, CONSTANTS::N_CONNECT);
        if (reason)
        {
            return *reason;
        }
        
        boost::algorithm::trim(action.at(CONSTANTS::LF_NAME));
        if (action.at(CONSTANTS::LF_NAME).empty())
        {
            return "NAME CAN NOT BE EMPTY";
        }
        return std::nullopt;
    }

    std::optional<std::string> ActionSendMessageIncorrect(std::unordered_map<std::string, std::string> &action)
    {
        auto reason = ActionSizeAndTokenIncorrect(action, CONSTANTS::N_SEND_MESSAGE);
        if (reason)
        {
            return *reason;
        }

        if (!action.contains(CONSTANTS::LF_MESSAGE))
        {
            return "NO MESSAGE FIELD";
        }

        boost::algorithm::trim(action.at(CONSTANTS::LF_MESSAGE));
        if (action.at(CONSTANTS::LF_MESSAGE).empty())
        {
            return "NO MESSAGE";
        }

        if (action.at(CONSTANTS::LF_MESSAGE).size() > CONSTANTS::N_MAX_MESSAGE_LEN)
        {
            return "TOO LARGE MESSAGE";
        }
        return std::nullopt;
    }

    std::optional<std::string> ActionDisconnectIncorrect(const std::unordered_map<std::string, std::string> &action)
    {
        
        auto reason = ActionSizeAndTokenIncorrect(action, CONSTANTS::N_DISCONNECT);
        if (reason)
        {
            return *reason;
        }
        return std::nullopt;
    }
}


namespace Service_Chatroom
{

    std::string Chr_MakeSuccessAddUser(std::string token);
    std::string Chr_MakeSuccessGetUsers(std::string userlist);
    std::string Chr_MakeSuccessSendMessage();
}
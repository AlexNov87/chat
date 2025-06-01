#include "service.h"

//ПРОВЕРКА ОБЩАЯ
namespace ServiceChatroomServer
{
    
    std::optional<std::string> CHK_ActionBaseIncorrect(const std::unordered_map<std::string, std::string> &action)
    {
        auto direction_err = CHK_ActionDirectionIncorrect(action);
        if (direction_err)
        {
            return *direction_err;
        }
        if (!action.contains(CONSTANTS::LF_ACTION))
        {
            return "NO ACTION FIELD";
        }
        if (action.at(CONSTANTS::LF_ACTION).empty())
        {
            return "EMPTY ACTION";
        }
        if (!Service::Additional::chatroom_actions.contains(action.at(CONSTANTS::LF_ACTION)) && !Service::Additional::server_actions.contains(action.at(CONSTANTS::LF_ACTION)))
        {
            return "ACTION: " + action.at(CONSTANTS::LF_ACTION) + " IS UNRECOGNIZED";
        }
        return std::nullopt;
    }
    
    std::optional<std::string> CHK_ActionDirectionIncorrect(const std::unordered_map<std::string, std::string> &action)
    {
        if (!action.contains(CONSTANTS::LF_DIRECTION))
        {
            return "NO DIRECTION FIELD";
        }

        if (!Service::Additional::request_directions.contains(action.at(CONSTANTS::LF_DIRECTION)))
        {
            return "THE DIRECTION IS NOT RECOGNIZED";
        }

        return std::nullopt;
    }

    ///@brief Проверяет размер контейнера
    std::optional<std::string> CHK_SizeActionIncorrect(const std::unordered_map<std::string, std::string> &action, size_t size)
    {
        if (action.size() != size)
        {
            return "SIZE OF " + action.at(CONSTANTS::LF_ACTION) + " IS INCORRECT EXPECTED: " + std::to_string(size);
        }
        return std::nullopt;
    }
    ///@brief Проверяет валидность токена
    std::optional<std::string> CHK_TokenIncorrect(const std::unordered_map<std::string, std::string> &action)
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
    ///@brief Проверяет валидность токена и размера
    std::optional<std::string> CHK_ActionSizeAndTokenIncorrect(const std::unordered_map<std::string, std::string> &action, size_t size)
    {

        auto size_reason = CHK_SizeActionIncorrect(action, size);
        if (size_reason)
        {
            return *size_reason;
        }

        auto token_error = CHK_TokenIncorrect(action);
        if (token_error)
        {
            return *token_error;
        }
        return std::nullopt;
    };

    

    

}

//ПРОВЕРКА ЗАПРОСА К ЧАТРУМУ
namespace ServiceChatroomServer
{

    ///@brief Проверяет валидность контейнера действия послания сообщения
    std::optional<std::string> Chr_ActionSendMessageIncorrect(const std::unordered_map<std::string, std::string> &action)
    {
        auto reason = CHK_ActionSizeAndTokenIncorrect(action, CONSTANTS::N_SEND_MESSAGE);
        if (reason)
        {
            return *reason;
        }

        if (!action.contains(CONSTANTS::LF_MESSAGE))
        {
            return "NO MESSAGE FIELD";
        }
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

    ///@brief Проверяет валидность контейнера действия отключения
    std::optional<std::string> Chr_ActionDisconnectIncorrect(const std::unordered_map<std::string, std::string> &action)
    {
        auto reason = CHK_ActionSizeAndTokenIncorrect(action, CONSTANTS::N_DISCONNECT);
        if (reason)
        {
            return *reason;
        }
        return std::nullopt;
    }

    std::optional<std::string> CHK_Chr_CheckErrorsChatRoom(const std::unordered_map<std::string, std::string> &action)
    {

        auto err = CHK_ActionBaseIncorrect(action);
        if (err)
        {
            return *err;
        }

        auto testcase = Service::Additional::action_scernario.at(action.at(CONSTANTS::LF_ACTION));
        switch (testcase)
        {
        case Service::ACTION::DISCONNECT:
        {
            err = Chr_ActionDisconnectIncorrect(action);
        }
        break;

        case Service::ACTION::SEND_MESSAGE:
        {
            err = Chr_ActionSendMessageIncorrect(action);
        }
        break;
        }

        if (err)
        {
            return *err;
        }

        return std::nullopt;
    };
}

namespace ServiceChatroomServer
{

    std::optional<std::string> Srv_ActionLoginIncorrect(const std::unordered_map<std::string, std::string> &action)
    {
        auto reason = CHK_ActionSizeAndTokenIncorrect(action, CONSTANTS::N_LOGIN);
        if (reason)
        {
            return *reason;
        }

        if (!action.contains(CONSTANTS::LF_NAME))
        {
            return "NO NAME FIELD";
        }

        if (action.at(CONSTANTS::LF_NAME).empty())
        {
            return "NAME CAN NOT BE EMPTY";
        }
        
        if (!action.contains(CONSTANTS::LF_ROOMNAME))
        {
            return "NO ROOMNAME FIELD";
        }

        if (action.at(CONSTANTS::LF_ROOMNAME).empty())
        {
            return "ROOMNAME CAN NOT BE EMPTY";
        }
        
        return std::nullopt;
    }

}

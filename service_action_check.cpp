#include "service.h"

namespace ServiceChatroomServer
{

    std::optional<std::string> CHK_ServerLoadObject(const boost::json::value &obj)
    {
        if (!obj.is_object())
        {
            return "OBJECT TYPE IS INCORRECT, JSON::OBJECT";
        }
        auto &js = obj.as_object();

        if (!js.contains(CONSTANTS::IP))
        {
            return "IP FIELD IS MISSING";
        }
        if (!js.at(CONSTANTS::IP).is_string())
        {
            return "IP MUST BE STRING";
        }

        if (!js.contains(CONSTANTS::PORT))
        {
            return "PORT FIELD IS MISSING";
        }
        if (!js.at(CONSTANTS::PORT).is_int64())
        {
            return "PORT MUST BE INT";
        }
        if (js.at(CONSTANTS::PORT).as_int64() < 0 || js.at(CONSTANTS::PORT).as_int64() > 65535)
        {
            return std::to_string(js.at(CONSTANTS::PORT).as_int64()) + " IS INCORRECT PORT VALUE";
        }

        if (!js.contains(CONSTANTS::CHATROOMS))
        {
            return std::nullopt;
        }

        auto &chrooms = js.at(CONSTANTS::CHATROOMS);
        if (!chrooms.is_array())
        {
            return "CHATROOM IS NOT ARRAY";
        }

        for (auto &&chr : chrooms.as_array())
        {
            if (!chr.is_string())
            {
                return "CHECK ROOMS NAMES";
            }
        }
        return std::nullopt;
    };
}
// ПРОВЕРКА ОБЩАЯ
namespace ServiceChatroomServer
{
    std::optional<std::string> CHK_FieldActionIncorrect(const task &action)
    {
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
    
    ///@brief Проверяет валидность направления
    std::optional<std::string> CHK_FieldDirectionIncorrect(const task &action)
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

    ///@brief Проверяет валидность токена
    std::optional<std::string> CHK_FieldTokenIncorrect(const task &action)
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


    ///@brief Проверяет размер контейнера
    std::optional<std::string> CHK_SizeOfContainerActionIncorrect(const task &action, size_t size)
    {
        if (action.size() != size)
        {
            return "SIZE OF " + action.at(CONSTANTS::LF_ACTION) + " IS INCORRECT EXPECTED: " + std::to_string(size);
        }
        return std::nullopt;
    }
    
    ///@brief Проверяет валидность токена и размера
    std::optional<std::string> CHK_ActionSizeAndTokenIncorrect(const task &action, size_t size)
    {

        auto size_reason = CHK_SizeOfContainerActionIncorrect(action, size);
        if (size_reason)
        {
            return *size_reason;
        }

        auto token_error = CHK_FieldTokenIncorrect(action);
        if (token_error)
        {
            return *token_error;
        }
        return std::nullopt;
    };

}

// ПРОВЕРКА ЗАПРОСА К ЧАТРУМУ
namespace ServiceChatroomServer
{

    ///@brief Проверяет валидность контейнера действия послания сообщения
    std::optional<std::string> Chr_ActionSendMessageIncorrect(const task &action)
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
    std::optional<std::string> Chr_ActionDisconnectIncorrect(const task &action)
    {
        auto reason = CHK_ActionSizeAndTokenIncorrect(action, CONSTANTS::N_DISCONNECT);
        if (reason)
        {
            return *reason;
        }
        return std::nullopt;
    }

    std::optional<std::string> CHK_Chr_CheckErrorsChatRoom(const task &action)
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

    std::optional<std::string> CHK_Srv_IsAddressedToServer(const task &action){
            if(Service::Additional::server_actions.contains(action.at(CONSTANTS::LF_ACTION))){
                return "ACTION " + action.at(CONSTANTS::LF_ACTION) + " CAN NOT BE ADDRESSED TO SERVER";
            }
    }

    std::optional<std::string> CHK_Srv_BaseToServerCheckIncorrect(const task &action){
        
        if(action.at(CONSTANTS::LF_DIRECTION) != CONSTANTS::RF_DIRECTION_SERVER){
            return "DIRECTION TO SERVER" + action.at(CONSTANTS::LF_DIRECTION) +" IS INCORRECT";
        }

        // ПРОВЕРКА ПОЛЯ ДЕЙСТВИЯ
        auto reason = CHK_ActionBaseIncorrect(action);
        if (reason)
        {
            return *reason;
        }
        //ПРОВЕРКА К СЕРВЕРУ ЛИ ОТНОСИТСЯ ДЕЙСТВИЕ 
        reason = CHK_Srv_IsAddressedToServer(action);
        if (reason)
        {
            return *reason;
        }

    }
    
    // Проверка Логина
    std::optional<std::string> CHK_Srv_ActionLoginIncorrect(const task &action)
    {
         
        // ПРОВЕРКА ПОЛЕЙ ТОКЕНА И РАЗМЕРА КОНТЕЙНЕРА
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

    std::optional<std::string> CHK_Srv_ActionGetUsersIncorrect(const task &action)
    {
        
    }

    std::optional<std::string> CHK_Srv_ActionCreateUserIncorrect(const task &action)
    {
      
        
        
        return std::nullopt;
    }








    std::optional<std::string> CHK_Chr_CheckErrorsChatServer(const task &action)
    {
        auto reason = CHK_Srv_BaseToServerCheckIncorrect(action);
        if (reason)
        {
            return *reason;
        } 
        
        
        
        
        
        
        
        return std::nullopt;
    };

}

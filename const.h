#pragma once
#include <string>
struct CONSTANTS
{
    // Actions
    static const std::string ACT_DISCONNECT;
    static const std::string ACT_GET_USERS;
    static const std::string ACT_SEND_MESSAGE;
    static const std::string ACT_LOGIN;
    static const std::string ACT_CREATE_ROOM;
    static const std::string ACT_ROOM_LIST;
    static const std::string ACT_CREATE_USER;
    static const std::string ACT_EARLIER_MESS;

    // Map Parameters
    static const std::string LF_ACTION;
    static const std::string LF_NAME;
    static const std::string LF_TOKEN;
    static const std::string LF_MESSAGE;
    static const std::string LF_REASON;
    static const std::string LF_RESULT;
    static const std::string LF_USERS;
    static const std::string LF_ROOMNAME;
    static const std::string LF_LOGIN;
    static const std::string LF_ROOMLIST;
    static const std::string LF_PASSWORD;

    static const std::string RF_SUCCESS;
    static const std::string RF_ERROR;

    //
    static const std::string LF_INITIATOR;
    static const std::string RF_ERR_INITIATOR_SERVER;
    static const std::string RF_ERR_INITIATOR_CHATROOM;
    static const std::string RF_ERR_PERMISSION_DENIDED;

    static const std::string LF_DIRECTION;
    static const std::string RF_DIRECTION_SERVER;
    static const std::string RF_DIRECTION_CHATROOM;

    static const std::string IP ; 
    static const std::string PORT; 
    static const std::string CHATROOMS; 

    ///@brief Размер объекта действия отключения
    static const size_t N_DISCONNECT;
    ///@brief Размер объекта действия послания сообщения
    static const size_t N_SEND_MESSAGE;

    
    ///@brief Размер объекта действия для получения списка пользователей
    static const size_t N_GET_USERS;
    ///@brief Размер объекта действия логина
    static const size_t N_LOGIN;
    ///@brief Размер объекта действия создания комнаты
    static const size_t N_CREATE_ROOM;
    ///@brief Размер объекта действия список комнат
    static const size_t N_ROOM_LIST;
    ///@brief Размер объекта действия =регистрация юзера
    static const size_t N_CREATE_USER;

    ///@brief Длина токена
    static const size_t N_TOKEN_LEN;
    ///@brief
    static const size_t N_MAX_MESSAGE_LEN;

private:
    CONSTANTS() {};
};
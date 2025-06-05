#include "srv.h"

std::string MainServer::ServerSession::LoginUser(shared_task action, shared_socket socket)
{
    try{

    // ЕСЛИ АВТОРИЗОВАН
    const auto &name = action->at(CONSTANTS::LF_NAME);
    const auto &pass = action->at(CONSTANTS::LF_PASSWORD);
    if (!server_->IsAutorizatedUser(name, pass))
    {
        return ServiceChatroomServer::MakeAnswerError("YOU ARE NOT AUTHORIZED", __func__);
    }
    // Генерируем токен
    std::string token = this->server_->tokezier_.GenerateHEXToken();

    // Блокируем возможность модифицировать комнаты
    std::atomic_bool &mod_users = server_->mod_users_;
    std::unique_lock<std::mutex> ul(server_->mtx_);
    server_->condition_.wait(ul, [&mod_users]
                             { return mod_users == false; });
    mod_users = true;

    // ЕСЛИ ЕСТЬ ТАКАЯ КОМНАТА
    std::string &roomname = action->at(CONSTANTS::LF_ROOMNAME);
    if (!server_->rooms_.contains(roomname))
    {
        // СНИМАЕМ БЛОКИРОВКУ
        mod_users = false;
        server_->condition_.notify_all();
        return ServiceChatroomServer::MakeAnswerError("NO ROOM: " + roomname, __func__);
    };
    auto room = server_->rooms_.at(roomname);
    // УДАЛОСЬ ЛИ ДОБАВТЬ
    bool added = room->AddUser(socket, name, token);
    // СНИМАЕМ БЛОКИРОВКУ
    mod_users = false;
    server_->condition_.notify_all();
    if (added)
    {
        // ОТВЕЧАЕМ УСПЕХОМ
        return ServiceChatroomServer::Srv_MakeSuccessLogin(std::move(token), std::move(roomname), room->msg_man_.LastMessages());
    }
    return ServiceChatroomServer::MakeAnswerError("FAILED TO ADD USER", __func__);
    }
    catch(const std::exception&ex){
        return ServiceChatroomServer::MakeAnswerError( ex.what() , __func__);
    }
}

std::string MainServer::ServerSession::GetStringResponceToSocket(shared_task action)
{
    auto reason = ServiceChatroomServer::CHK_Chr_CheckErrorsChatServer(*action);
    if (reason)
    {
        return ServiceChatroomServer::MakeAnswerError(*reason, __func__);
    }
    return ExectuteReadySession(action, socket_);
}

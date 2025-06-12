#include "srv.h"


bool Chatroom::HasToken(const std::string &token)
{
    std::lock_guard<std::mutex> lg(mtx_);
     return users_.contains(token);
}

bool Chatroom::AddUser(shared_stream stream, std::string name,
                       std::string token)
{
    bool success = false;
    {
        std::lock_guard<std::mutex> lg(mtx_);
        auto self = weak_from_this();
        auto it = users_.insert({token, std::make_shared<Chatuser>(self, std::move(name), stream, mainserv_->ioc_)});
        success = it.second;
    }
    if (success)
    {
        users_.at(token)->Run();
        // ЛОГИРУЕМ СИТЕМНОЕ СООБЩЕНИЕ
        msg_man_.ServiceMessage(users_.at(token)->name_ + " IS CONNECTED");
        return success;
    }
    return success;   
}

void Chatroom::SendMessages(const std::string &token, const std::string &name, const std::string &message)
{
    
      //Создаем тело ответа
      auto str = ServiceChatroomServer::Chr_MakeSuccessUserMessage(name , message);
      //Создаем сам ответ
      auto responce = Service::MakeResponce(11, true, http::status::ok, std::move(str)); 
   
std::lock_guard<std::mutex> lg(mtx_);     
    std::cout << users_.at(token)->name_ << " : " << message << '\n';
    // Рассылка
    for (auto &&[token, chatuser] : users_)
    {
        // Если сокет недоступен
        if (!Service::IsAliveSocket(chatuser->stream_->socket()))
        {
            continue;
        }
        //Постим каждому юзеру задачу
        chatuser->IncomeMessage(responce);
    };
}

void Chatroom::DeleteUser(std::string token)
{
    std::lock_guard<std::mutex> lg(mtx_); 
    users_.erase(token);
}

std::string Chatroom::RoomMembers()
{
    std::ostringstream oss;
    oss << "[ ";
    size_t nowpos = 0;
    
    std::unique_lock<std::mutex> ul(mtx_);
    {
        for (auto &&[token, chatuser] : users_)
        {
            oss << '"' << chatuser->name_ << '"';
            ++nowpos;
            if (nowpos >= users_.size() - 1)
            {
                break;
            }
            oss << " , ";
        }
    }
    ul.unlock();
    oss << " ]";
    return oss.str();
}

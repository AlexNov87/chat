#include "srv.h"


bool Chatroom::HasToken(const std::string &token)
{
    // блокируем возможность подключение и удаления юзеров
  //  std::unique_lock<std::mutex> ul(mut_users_);
  //  do_not_allow_modify_users = true;
 //   cond_.wait(ul, [&]()
   //            { return modyfiing_users == false; });

    bool has_token = users_.contains(token);

  //  do_not_allow_modify_users = false;
    // оповещаем потоки
  //  cond_.notify_all();
    return has_token;
}

bool Chatroom::AddUser(shared_socket socket, std::string name,
                       std::string token)
{
    bool success = false;
    {
        /* Обязательно область видимости!!! Service::GuardLock
               пока не выпоннит задачу, требующую блокировки
            */
        // Дождется пока добавляются или удаляются юзеры, запретит удалять или добавлять.
       #ifdef sw 
        Service::GuardLockAnotherAwait(do_not_allow_modify_users, mut_users_, cond_, modyfiing_users).Lock();
       #endif
        auto self = weak_from_this();
        auto it = users_.insert({token, std::make_shared<Chatuser>(self, std::move(name), socket, mainserv_->ioc_)});
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

void Chatroom::SendMessages(const std::string &token, const std::string &message)
{
      //Создаем тело ответа
      auto str = ServiceChatroomServer::Chr_MakeSuccessUserMessage(users_.at(token)->name_  , message);
      //Создаем сам ответ
      auto responce = Service::MakeResponce(11, true, http::status::ok, std::move(str)); 

    // Дождется пока добавляются или удаляются юзеры, запретит удалять или добавлять.
  //  Service::GuardLockAnotherAwait(do_not_allow_modify_users, mut_users_, cond_, modyfiing_users).Lock();
    // Нет токена - выйдет
    std::cout << users_.at(token)->name_ << " : " << message << '\n';
   
    // Рассылка
    for (auto &&[token, chatuser] : users_)
    {
        // Если сокет недоступен
        if (!Service::IsAliveSocket(chatuser->socket_))
        {
            continue;
        }
        //Постим каждому юзеру задачу
        chatuser->IncomeMessage(responce);
    };
}

void Chatroom::DeleteUser(std::string token)
{
    // Дождется пока постятся рассылки сообщений и запретит временно рассылку
  //  Service::GuardLockAnotherAwait(modyfiing_users, mut_users_, cond_, do_not_allow_modify_users).Lock();
    users_.erase(token);
}

std::string Chatroom::RoomMembers()
{
    std::ostringstream oss;
    oss << "[ ";
    size_t nowpos = 0;
    {
        /* Обязательно область видимости!!! Service::GuardLock
              пока не выпоннит задачу, требующую блокировки
           */
        // Ждет пока добавляются или удаляются юзеры , потом запрещает добалять или удалять
    //    Service::GuardLockConditional(do_not_allow_modify_users, mut_users_, cond_).Lock();
        for (auto &&[token, chatuser] : users_)
        {
            oss << '"' << chatuser->name_ << '"';
            ++nowpos;
            if (nowpos == users_.size() - 1)
            {
                break;
            }
            oss << " , ";
        }
    }
    oss << " ]";
    return oss.str();
}

/*
 // Блокируем возможность рассылки по сокетам на время модификации списка
    std::unique_lock<std::mutex> ul(mut_users_);
    modyfiing_users = true;
    cond_.wait(ul, [&]()
               { return do_not_allow_modify_users == false; });
    foo();
    // закончили модификацию
    modyfiing_users = false;
    // оповещаем потоки
    cond_.notify_all();

*/
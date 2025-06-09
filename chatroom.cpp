#include "srv.h"

bool Chatroom::HasToken(const std::string &token)
{
    // блокируем возможность подключение и удаления юзеров
    std::unique_lock<std::mutex> ul(mut_users_);
    do_not_allow_modify_users = true;
    cond_.wait(ul, [&]()
               { return modyfiing_users == false; });

    bool has_token = users_.contains(token);

    do_not_allow_modify_users = false;
    // оповещаем потоки
    cond_.notify_all();
    return has_token;
}

void Chatroom::AddUser(shared_socket socket, std::string name, 
std::string token)
{
    ZyncPrint("ADDING USER");
    {
        /* Обязательно область видимости!!! Service::GuardLock
               пока не выпоннит задачу, требующую блокировки
            */
        // Дождется пока добавляются или удаляются юзеры, запретит удалять или добавлять.
     //  Service::GuardLockAnotherAwait(do_not_allow_modify_users, mut_users_, cond_, modyfiing_users).Lock();
       ZyncPrint("INSERTING");
        ZyncPrint("-------------->" + mainserv_->CHECKPTR);
      
      try{
        ZyncPrint(this->mainserv_->CHECKPTR);
        auto self = weak_from_this();
     ZyncPrint("CREATING");
     auto us = std::make_shared<Chatuser>(self, std::move(name), socket , mainserv_->ioc_);
      if(!us){
        ZyncPrint("NOOO");
      }
      // users_[token] = 
        users_.at(token)->Run();
      }
      catch(const std::exception& ex){
        ZyncPrint(ex.what());
      }
     
     
    }

        ZyncPrint("RUNNING");
       
    // ЛОГИРУЕМ СИТЕМНОЕ СООБЩЕНИЕ
     msg_man_.ServiceMessage(users_.at(token)->name_ + " IS CONNECTED");
    //  ZyncPrint("ADDPOST.....................B");

    // auto lam = [self = shared_from_this(), token, socket]
    // {
    //     auto rsp = Service::MakeResponce(11, true, http::status::ok, ServiceChatroomServer::Srv_MakeSuccessLogin(token, "RMNAME", "NONE"));
    //     http::async_write(*socket, rsp, [self, token](err ec, size_t bytes){
    //       //  self->AwaitSocket(token);
    //     });
        
    // };
   
    //     for(int i = 0; i< 5; ++i){
    //     net::post(*users_.at(token).strand_, lam);
    // }
}

void Chatroom::SendMessages(const std::string &token, const std::string &message)
{

    // Дождется пока добавляются или удаляются юзеры, запретит удалять или добавлять.
    Service::GuardLockAnotherAwait(do_not_allow_modify_users, mut_users_, cond_, modyfiing_users).Lock();
    // Нет токена - выйдет
    if (!HasToken(token))
    {
        return;
    }
    std::cout << users_.at(token)->name_ << " : " << message << '\n';

    // Рассылка
    for (auto &&[token, chatuser] : users_)
    {
        // Если сокет недоступен
        if (!Service::IsAliveSocket(chatuser->socket_))
        {
            continue;
        }
        // // Постим в последовательный исполнитель юзера асинхронную запись в его сокет
        // net::post(*chatuser.strand_, [&, socket = users_.at(token).socket_, token, buf = net::buffer(message)]()
        //           { net::async_write(*socket, std::move(buf), [&, token, socket](err ec, size_t bytes)
        //                              {
        //                      //Переводим сокет юзера в режим ожидания
        //                     }); });
    };
}

void Chatroom::DeleteUser(std::string token)
{
    // Дождется пока постятся рассылки сообщений и запретит временно рассылку
    Service::GuardLockAnotherAwait(modyfiing_users, mut_users_, cond_, do_not_allow_modify_users).Lock();
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
        Service::GuardLockConditional(do_not_allow_modify_users, mut_users_, cond_).Lock();
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
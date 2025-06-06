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

void Chatroom::AwaitSocket(const std::string &token)
{
    try
    {
        auto socket = users_.at(token).socket_;
        auto strand = users_.at(token).strand_;
        auto buffer = Service::MakeSharedStreambuf();
        auto lam = [&, socket, strand, buffer](err ec, std::size_t bytes)
        {
            if (!ec)
            {
                auto action = Service::ExtractSharedObjectsfromBuffer(*buffer, bytes);
                // ПРОВЕРЯЕМ НАПРАВЛЕНИЕ ЗАДАЧИ (есть ли ошибка в поле "направление задачи")
                auto direction_err = ServiceChatroomServer::CHK_FieldDirectionIncorrect(*action);
                // ПРОВЕРЯЕМ НАПРАВЛЕНИЕ
                if (direction_err)
                {
                    // Постим в последовательный исполнитель юзера асинхронную запись в его сокет
                   net::post(*strand, [&, socket , token, error = *direction_err ](){ 
                    
                    net::async_write(*socket, net::buffer(ServiceChatroomServer::MakeAnswerError(std::move(error), __func__)), [&, token, socket](err ec, size_t bytes){
                             //Переводим сокет юзера в режим ожидания
                             AwaitSocket(token); 
                             }); 
                   });
                }
                // ЕСЛИ ЗАДАЧА ОТНОСИТСЯ К ЧАТ-РУМУ
                if (action->at(CONSTANTS::LF_DIRECTION) == CONSTANTS::RF_DIRECTION_CHATROOM)
                {
                    auto chrsess = std::make_shared<ChatRoomSession>(this, socket, strand);
                }
                else
                {
                    // ЕСЛИ ЗАДАЧА ОТНОСИТСЯ К CЕРВЕРУ
                    auto servsessptr = std::make_shared<MainServer::ServerSession>(this->mainserv_, socket, strand);
                    servsessptr->HandleSession();
                }
            }
            else
            {
                //Постим запись в сокет ошибку
                net::post(*users_.at(token).strand_, [&, token, msg = ec.message(), socket]
                          {
                              net::async_write(*socket,
                                               net::buffer(ServiceChatroomServer::MakeAnswerError(msg, " __func__")), [&, token](err ec, size_t bytes)
                                               { 
                                              if(!ec){
                                              //Переводим в асинхронное слушание
                                              AwaitSocket(token);
                                              }});
                          });
            }
        };
        net::async_read(*socket, *buffer, lam);
    }
    catch (const std::exception &ex)
    {
       ZyncPrint(ex.what());
    }
}

bool Chatroom::AddUser(shared_socket socket, std::string name, std::string token)
{
    {
        /* Обязательно область видимости!!! Service::GuardLock
               пока не выпоннит задачу, требующую блокировки
            */
        // Дождется пока добавляются или удаляются юзеры, запретит удалять или добавлять.
        Service::GuardLockAnotherAwait(do_not_allow_modify_users, mut_users_, cond_, modyfiing_users).Lock();
        users_.insert({token, Chatuser(std::move(name), ioc_, socket)});
    }
    // ЛОГИРУЕМ СИТЕМНОЕ СООБЩЕНИЕ
    msg_man_.ServiceMessage(users_.at(token).name_ + " IS CONNECTED");

    // Постим в последовательный исполгитель юзера асинххронное прослушивание его сокета
    net::post(*users_.at(token).strand_, [&, token]
              { AwaitSocket(token); });
    return true;
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
    std::cout << users_.at(token).name_ << " : " << message << '\n';

    // Рассылка
    for (auto &&[token, chatuser] : users_)
    {
        // Если сокет недоступен
        if (!Service::IsAliveSocket(chatuser.socket_))
        {
            continue;
        }
        // Постим в последовательный исполнитель юзера асинхронную запись в его сокет
        net::post(*chatuser.strand_, [&, socket = users_.at(token).socket_, token, buf = net::buffer(message)]()
                  { net::async_write(*socket, std::move(buf), [&, token, socket](err ec, size_t bytes)
                                     {
                             //Переводим сокет юзера в режим ожидания
                             AwaitSocket(token); }); });
    };
}

void Chatroom::DeleteUser(const std::string &token)
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
        Service::GuardLockConditional(do_not_allow_modify_users, mut_users_, cond_);
        for (auto &&[token, chatuser] : users_)
        {
            oss << '"' << chatuser.name_ << '"';
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
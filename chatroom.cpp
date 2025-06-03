#include"srv.h"
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

    void Chatroom::AwaitSocket(const std::string& token)
    {
        auto socket = users_.at(token).socket_;
        try
        {
            boost::asio::streambuf buffer;
            auto lam =  [&,socket](boost::system::error_code ec, std::size_t length)
                                          {
                                              if (!ec)
                                              {
                                                auto actions = Service::ExtractSharedObjectsfromBuffer(buffer);
                                                  
                                                  
                                                  for (auto action : actions){
                                                
                                                  //ПРОВЕРЯЕМ НАПРАВЛЕНИЕ ЗАДАЧИ (есть ли ошибка в поле "направление задачи")
                                                  auto direction_err = ServiceChatroomServer::CHK_FieldDirectionIncorrect(*action);
                                                  if(direction_err){
                                                    ServiceChatroomServer::WriteErrorToSocket(socket, *direction_err, "AwaitSocket");
                                                    AwaitSocket(token);
                                                  }
                                                    //ЕСЛИ ЗАДАЧА ОТНОСИТСЯ К ЧАТ-РУМУ 
                                                    if(action->at(CONSTANTS::LF_DIRECTION) == CONSTANTS::RF_DIRECTION_CHATROOM) {
                                                       auto chrsess = std::make_shared<ChatRoomSession>(this);
                                                       chrsess->HandleExistingSocket(socket, action);
                                                    }
                                                    //ЕСЛИ ЗАДАЧА ОТНОСИТСЯ К CЕРВЕРУ
                                                    auto servsessptr = std::make_shared<MainServer::ServerSession>(this->mainserv_);
                                                    servsessptr->HandleExistsSocket(action ,this->users_.at(token));  
                                                  }
                                              }
                                              else
                                              {
                                                 ServiceChatroomServer::WriteErrorToSocket(socket, ec.message(), "AwaitSocket");
                                              }
                                              AwaitSocket(token);
                                          };  
            
                                         net::async_read(*socket, buffer,  lam);

        }
        catch (const boost::system::system_error &err)
        {

            if (err.code() == boost::asio::error::eof)
            {
                std::cout << "Remote side closed connection (EOF)" << std::endl;
                // Корректное завершение работы с сокетом
            }
            else
            {
                ServiceChatroomServer::WriteErrorToSocket
                (socket, "Read Error: " + std::string(err.what()), CONSTANTS::RF_ERR_INITIATOR_CHATROOM);
            }
        }
    }

    

    void Chatroom::AddUser(shared_socket socket, std::string name, std::string token, std::string roomname)
    {
        auto lam = [&]()
        {
            users_.insert({token, Chatuser(std::move(name), ioc_, socket)});
        };
        // Блокируем возможность слать по сокетам на время модификации списка юзеров
        MakeLockedModUsers(lam);

        // Если сокет "Не жив - выходим"
        if (!Service::IsAliveSocket(users_.at(token).socket_))
        {
            DeleteUser(token);
            return;
        }
        
        std::string responce = ServiceChatroomServer::
        Srv_MakeSuccessLogin(token, std::move(roomname)); 
        net::post( *(users_.at(token).strand_), [&, resp = std::move(responce)]()
                  { users_.at(token).socket_->write_some(net::buffer(resp));});

        std::string lastmess = msg_man_.LastMessages();
        net::post( *(users_.at(token).strand_), [&, resp = std::move(lastmess)]()
                  { users_.at(token).socket_->write_some(net::buffer(resp));});

        msg_man_.ServiceMessage(users_.at(token).name_ + " IS CONNECTED" );
        AwaitSocket(token);
    }

    void Chatroom::SendMessages(const std::string& token, const std::string& message)
    {
        // блокируем возможность подключение и удаления юзеров
        std::unique_lock<std::mutex> ul(mut_users_);
        do_not_allow_modify_users = true;
        cond_.wait(ul, [&]()
                   { return modyfiing_users == false; });
        
        if(!HasToken(token)){return;}
        std::cout << users_.at(token).name_ <<  " : " << message << '\n' ; 
        
        for (auto &&[token, chatuser] : users_)
        {
            if (!Service::IsAliveSocket(chatuser.socket_))
            {
                continue;
            }
            net::post(*chatuser.strand_, [&, buf = net::buffer(message) ]()
                      { users_.at(token).socket_->write_some(buf); });
            
        };
        // заканчиваем рассылку
        do_not_allow_modify_users = false;
        // оповещаем потоки
        cond_.notify_all();
        AwaitSocket(token);
    }

    void Chatroom::DeleteUser(const std::string& token)
    {
        auto lam = [&]()
        {
            users_.erase(token);
        };
        MakeLockedModUsers(lam);
    }

    std::string Chatroom::RoomMembers()
    {
        // Блокируем возможность рассылки по сокетам на время модификации списка
        std::unique_lock<std::mutex> ul(mut_users_);
        // Запрещаем добавлять - удалять юзеров
        do_not_allow_modify_users = true;
        std::ostringstream oss;
        oss << "[\n";
        size_t nowpos = 0;
        for (auto &&[token, chatuser] : users_)
        {
            oss << '"' << chatuser.name_ << '"'; 
            ++nowpos;
            if(nowpos == users_.size() - 1){break;} 
           oss << ", \n";
        }
        oss << "]\n";
        do_not_allow_modify_users = false;
        // оповещаем потоки
        cond_.notify_all();
        return oss.str(); 
    }

   
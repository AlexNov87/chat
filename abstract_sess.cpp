#include "srv.h"
std::atomic_int PrototypeSession::exempslars = 0;

void AbstractSession::StartExecuteAction(shared_task action)
{
    try
    {
        std::string responce_body;
        // Проверка есть ли действие и не пустое ли оно
        auto reason = ServiceChatroomServer::CHK_FieldExistsAndNotEmpty(*action, CONSTANTS::LF_ACTION);
        if (reason)
        {
            // Если нет - ошибка
            responce_body = ServiceChatroomServer::MakeAnswerError(*reason, "StartExecuteAction()Exc1", CONSTANTS::UNKNOWN);
            ZyncPrint("StartExecuteAction()Exc1");
        }
        else
        {
            // Если все ок - исполняем сессию
            responce_body = GetStringResponceToSocket(action);
        }
        if (responce_body.empty())
        {
            // При логине пользователя вернется "" значит закрываем сесссию
            return;
        }

        Write(std::move(responce_body));
    } // try
    catch (const std::exception &ex)
    {
        // ловим всевозможные исключения
        std::string responce_body = ServiceChatroomServer::MakeAnswerError(ex.what(), "StartExecuteAction()Exc2", CONSTANTS::UNKNOWN);
        ZyncPrint("StartExecuteAction()Exc2");
        Write(std::move(responce_body));
    }
};

void AbstractSession::StartAfterReadHandle()
{
    /// ИЗВЛЕКАЕМ ЗНАЧЕНИЕ
    try
    {
        shared_task action = Service::ExtractSharedObjectsfromRequestOrResponce(request_);
        if (!action)
        {
            Write(ServiceChatroomServer::MakeAnswerError("Action is nullptr", "StartAfterReadHandle1()", CONSTANTS::UNKNOWN));
            return;
        }
        StartExecuteAction(action);
    }
    // При исключении десереализации архива
    catch (const std::exception &ex)
    {
        std::string responce_body = ServiceChatroomServer::MakeAnswerError(ex.what(), "StartAfterReadHandle2()", CONSTANTS::UNKNOWN);
        ZyncPrint("StartAfterReadHandle2()");
        Write(std::move(responce_body));
    }
};

void PrototypeSession::Read()
{
    request_ = {};
    //  ПРОВЕРЯЕМ ЖИВ ЛИ СОКЕТ
    if (!Service::IsAliveSocket(stream_->socket()))
    {
        ZyncPrint("SOCKET IS DAMAGED");
        Service::ShutDownSocket(stream_->socket());
        return;
    }
    if (!stream_)
    {
        ZyncPrint("STREAM SESSION IS DAMAGED");
        return;
    }
   
    // Начинаем асинхронноен чтение
    http::async_read(*stream_, readbuf_, request_, 
    beast::bind_front_handler(&PrototypeSession::OnRead, shared_from_this())); // async read until
};

void PrototypeSession::OnRead(err ec, size_t bytes)
{
    if (!ec)
    {   // Обрабатываем прочитанные данные
        StartAfterReadHandle();
    }
};

void PrototypeSession::Run()
{
    net::dispatch(stream_->get_executor(),
                  beast::bind_front_handler(
                      &PrototypeSession::Read,
                      shared_from_this()));
};

void PrototypeSession::Write(std::string responce_body, http::status status)
{

    try
    {
        response rsp(Service::MakeResponce(11, true, http::status::ok, std::move(responce_body)));
        ZyncPrint(rsp.body());
        Service::PrintUmap(Service::DeserializeUmap<std::string, std::string>(rsp.body()));

        // ПИШЕМ В СОКЕТ
        http::async_write(*stream_, std::move(rsp), 
        beast::bind_front_handler(&AbstractSession::OnWrite,shared_from_this(), rsp.keep_alive())); // async writ
    }
    catch (const std::exception &ex)
    {
        ZyncPrint("WriteToSocketEXCERPTION");
    }
};

void PrototypeSession::OnWrite(bool keep_alive, beast::error_code ec, std::size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);

        if (ec)

            if (!keep_alive)
            {
                   stream_->socket().close();
            }
        // Read another request
         ZyncPrint("WriteComplete");
         request_ = {};
        Read();
    }

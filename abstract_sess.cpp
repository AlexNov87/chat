#include "srv.h"
std::atomic_int AbstractSession::exempslars = 0;
////////////////////////////////
void AbstractSession::Read()
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
                     beast::bind_front_handler(&AbstractSession::OnRead, shared_from_this())); // async read until
};

void AbstractSession::OnRead(err ec, size_t bytes)
{
    if (!ec)
    { // Обрабатываем прочитанные данные
        StartAfterReadHandle();
    }
};

void AbstractSession::Run()
{
   if(!stream_) {
      ZyncPrint("THE STREAM IS DAMAGED");
      return;
   }
    net::dispatch(stream_->get_executor(),
                  beast::bind_front_handler(
                      &AbstractSession::Read,
                      shared_from_this()));
};

void AbstractSession::Write(std::string responce_body, http::status status)
{
    try
    {
        response rsp(Service::MakeResponce(
            11, true,
            status, std::move(responce_body)));
        ZyncPrint(rsp.body());
        Service::PrintUmap(Service::DeserializeUmap<std::string, std::string>(rsp.body()));

        // ПИШЕМ В СОКЕТ
        http::async_write(*stream_, std::move(rsp),
                          beast::bind_front_handler(&AbstractSession::OnWrite, shared_from_this(), rsp.keep_alive())); // async writ
    }
    catch (const std::exception &ex)
    {
        ZyncPrint("WriteToSocketEXCERPTION", ex.what());
    }
};

void AbstractSession::OnWrite(bool keep_alive, beast::error_code ec, std::size_t bytes_transferred)
{
    boost::ignore_unused(bytes_transferred);

    if (ec)

        if (!keep_alive)
        {
            stream_->socket().close();
        }
    // Read another request
    ZyncPrint("WriteComplete.............");
    request_ = {};
    Read();
}

void AbstractSession::Сlose()
{
    beast::error_code ec;
    stream_->socket().shutdown(tcp::socket::shutdown_send, ec);
}

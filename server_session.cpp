#include"srv.h"
void MainServer::ServerSession::HandleSession(shared_socket socket)
        {
            if (!Service::IsAliveSocket(*socket))
            {
                Service::ShutDownSocket(*socket);
                return;
            }

            std::atomic_bool condition = true;
            net::steady_timer timer(server_->ioc_);
            auto self = this->shared_from_this();
            timer.async_wait([socket, self, &condition](const boost::system::error_code &ec)
                             {
                 if(condition){return;}
                 std::cout << "Timer expired!" << std::endl;
                 Service::ShutDownSocket(*socket); });
            timer.expires_after(std::chrono::milliseconds(3));
            auto tasks = Service::DoubleGuardedExcept<std::vector<task>>([socket]()
                                                                         { return Service::ExtractObjectsfromSocket(*socket); }, "HandleSession");
            condition = false;
        };
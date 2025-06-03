#pragma once
#include<mutex>
#include<deque>
#include<string>
#include<iostream>
#include<sstream>

class MessageManager
{
public:
    void AddUserMessage(const std::string &message, std::string &autor)
    {
        std::lock_guard<std::mutex> lg(mtx_msg_);
        std::cout << message << std::endl;
        ++messages_;
        std::stringstream strm;
        message_queue_.push_back(autor + "-->" + message + '\n');
        ClearDeque();
    }

    template <typename T>
    void ServiceMessage(T message)
    {
        std::lock_guard<std::mutex> lg(mtx_service_);
        std::cout << message << std::endl;
    }

    std::string LastMessages()
    {
        std::lock_guard<std::mutex> lg(mtx_msg_);
        size_t loops = std::min(static_cast<int>(message_queue_.size()), 30);
        std::stringstream strm;
        for(auto&& msg : message_queue_){
            strm << msg;
        }
        return strm.str();
    };

private:
    std::deque<std::string> message_queue_;
    std::mutex mtx_msg_;
    std::mutex mtx_service_;
    size_t messages_;

private:
    void ClearDeque()
    {
        while (message_queue_.size() > 30)
        {
            message_queue_.pop_front();
        }
    }
};
#include"guardlock.h"
namespace Service
{
 NoConditionGuardLock::NoConditionGuardLock(booltype &lc, std::mutex &mtx) : locker_(lc), ul_(mtx) {};
         void NoConditionGuardLock::UnlockImmediately()
        {
            CheckThatWasLocked();
            if (!was_im_unlocked_)
            {
                Unlock();
                ul_.unlock();
                was_im_unlocked_ = true;
            }
            else
            {
                throw std::logic_error("ONCE YOU UNLOCKED THIS OPERATION");
            }
        };
       
        NoConditionGuardLock::~NoConditionGuardLock()
        {
            if (!was_im_unlocked_)
            {
                Unlock();
            }
        }

        void NoConditionGuardLock::Lock()
        {
            // локер закрыт
            locker_ = true;
            was_locked_ = true;
        };

  
        void NoConditionGuardLock::Unlock()
        {
            CheckThatWasLocked();
            // локер открыт(значение false что он открыт)
            locker_ = false;
        };

        void NoConditionGuardLock::CheckThatWasLocked()
        {
            if (!was_locked_)
            {
                throw std::logic_error("THE OBJECT WASN`T LOCKED");
            }
        }
}


namespace Service
{
    GuardLockConditional::GuardLockConditional(std::atomic_bool &lc,
                                               std::mutex &mtx, std::condition_variable &condition)
        : NoConditionGuardLock(lc, mtx), condition_(condition) {}

    GuardLockConditional::~GuardLockConditional()
    {
        if (!was_im_unlocked_)
        {
            Unlock();
        }
    }

    void GuardLockConditional::UnlockImmediately()
    {
        if (!was_im_unlocked_)
        {
            Unlock();
            ul_.unlock();
            was_im_unlocked_ = true;
        }
        else
        {
            throw std::logic_error("ONCE YOU UNLOCKED THIS OPERATION");
        }
    }

    void GuardLockConditional::Lock()
    {
        condition_.wait(ul_, [&]
                        { return locker_ == false; });
        locker_ = true;
        was_locked_ = true;
    }

    void GuardLockConditional::Unlock()
    {
        if (!was_im_unlocked_)
        {
            CheckThatWasLocked();
            locker_ = false;
            condition_.notify_all();
        }
    }
 }

namespace Service
{   ///////////////////////////////////////////////////////////
    void GuardLockAnotherAwait::Lock()
    {
        // Ждем выполнение внешнего условия
        condition_.wait(ul_, [&]()
                        { return another_await_ == false; });
        // блокируем чтобы никто не стал выполнять внешнее условие
        another_await_ = true;
        condition_.wait(ul_, [&]()
                        { return locker_ == false; });
        // блокируем чтобы никто не стал выполнять внешнее условие
        locker_ = true;
        // Блокируем локер, (взялся за выполнение функции)
        locker_ = true;
        was_locked_ = true;
    };

    void GuardLockAnotherAwait::UnlockImmediately()
    {
        if (!was_im_unlocked_)
        {
            CheckThatWasLocked();
            Unlock();
            ul_.unlock();
            was_im_unlocked_ = true;
        }
        else
        {
            throw std::logic_error("ONCE YOU UNLOCKED THIS OPERATION");
        }
    };

    void GuardLockAnotherAwait::Unlock()
    {
        if (!was_im_unlocked_)
        {
            another_await_ = false;
            locker_ = false;
            condition_.notify_all();
        }
    };

    GuardLockAnotherAwait::GuardLockAnotherAwait(booltype &lc, std::mutex &mtx,
                                                 std::condition_variable &condition, booltype &an_await)
        : GuardLockConditional(lc, mtx, condition), another_await_(an_await) {};
    
    GuardLockAnotherAwait::~GuardLockAnotherAwait()
    {
        Unlock();
    };

}
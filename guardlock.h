#pragma once
#include<condition_variable>
#include<mutex>
using booltype = std::atomic_bool;
namespace Service
{
 class NoConditionGuardLock
    {
    public:
        NoConditionGuardLock(booltype &lc, std::mutex &mtx);
        virtual void UnlockImmediately();
        virtual ~NoConditionGuardLock();
        virtual void Lock();
    protected:
        virtual void Unlock();
        virtual void CheckThatWasLocked();

        booltype &locker_;
        std::unique_lock<std::mutex> ul_;
        bool was_im_unlocked_ = false;
        bool was_locked_ = false;
    };

    class GuardLockConditional : public NoConditionGuardLock
    {
    public:
        GuardLockConditional(booltype &lc, std::mutex &mtx, std::condition_variable &condition);
        ~GuardLockConditional() override;
        void UnlockImmediately() override;
       // void CheckThatWasLocked() override;
        void Lock() override;

    protected:
        void Unlock() override;
        std::condition_variable &condition_;
    };

    class GuardLockAnotherAwait : public GuardLockConditional
    {
    public:
        GuardLockAnotherAwait(booltype &lc, std::mutex &mtx,
                              std::condition_variable &condition, booltype &an_await);
        ~GuardLockAnotherAwait();
        void Lock() override;
        void UnlockImmediately() override;
       // void CheckThatWasLocked() override;

    protected:
         void Unlock() override;
         booltype &another_await_;
    };

}
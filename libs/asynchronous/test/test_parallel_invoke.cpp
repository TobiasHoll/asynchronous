

// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2013
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#include <vector>
#include <set>
#include <future>

#include <boost/asynchronous/scheduler/single_thread_scheduler.hpp>
#include <boost/asynchronous/queue/lockfree_queue.hpp>
#include <boost/asynchronous/scheduler_shared_proxy.hpp>
#include <boost/asynchronous/scheduler/threadpool_scheduler.hpp>

#include <boost/asynchronous/servant_proxy.hpp>
#include <boost/asynchronous/post.hpp>
#include <boost/asynchronous/trackable_servant.hpp>
#include <boost/asynchronous/algorithm/parallel_invoke.hpp>

#include "test_common.hpp"

#include <boost/test/unit_test.hpp>
using namespace boost::asynchronous::test;

namespace
{
// main thread id
boost::thread::id main_thread_id;
bool servant_dtor=false;
typedef boost::asynchronous::any_loggable servant_job;

struct my_exception : public boost::asynchronous::asynchronous_exception
{
    virtual const char* what() const throw()
    {
        return "my_exception";
    }
};

struct Servant : boost::asynchronous::trackable_servant<>
{
    typedef int simple_ctor;
    Servant(boost::asynchronous::any_weak_scheduler<> scheduler)
        : boost::asynchronous::trackable_servant<>(scheduler,
                                               boost::asynchronous::make_shared_scheduler_proxy<
                                                   boost::asynchronous::threadpool_scheduler<
                                                           boost::asynchronous::lockfree_queue<>>>(6))
    {
    }
    ~Servant()
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant dtor not posted.");
        servant_dtor = true;
    }

    std::future<void> start_async_work()
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant start_async_work not posted.");
        // we need a promise to inform caller when we're done
        std::shared_ptr<std::promise<void> > aPromise(new std::promise<void>);
        std::future<void> fu = aPromise->get_future();
        boost::asynchronous::any_shared_scheduler_proxy<> tp =get_worker();
        std::vector<boost::thread::id> ids = tp.thread_ids();
        // start long tasks
        post_callback(
           [ids](){
                    BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant work not posted.");

                    BOOST_CHECK_MESSAGE(contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task executed in the wrong thread");
                    return boost::asynchronous::parallel_invoke<boost::asynchronous::any_callable>(
                                boost::asynchronous::to_continuation_task([](){ASYNCHRONOUS_THROW (my_exception());}),
                                boost::asynchronous::to_continuation_task([](){return 42.0;}));
                    },// work
           [aPromise,ids,this](boost::asynchronous::expected<std::tuple<boost::asynchronous::expected<void>,
                                                                       boost::asynchronous::expected<double>>> res){
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant callback in main thread.");
                        BOOST_CHECK_MESSAGE(!contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task callback executed in the wrong thread(pool)");
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                        try
                        {
                            auto t = res.get();
                            BOOST_CHECK_MESSAGE((std::get<0>(t)).has_exception(),"first lambda did not throw an exception. It should have");
                            BOOST_CHECK_MESSAGE(!(std::get<1>(t)).has_exception(),"second lambda threw an exception. It should not have");
                            BOOST_CHECK_MESSAGE((std::get<1>(t)).get() == 42.0,"second lambda returned wrong result");
                        }
                        catch(...)
                        {
                            BOOST_FAIL( "unexpected exception in callback" );
                        }
                        // check correct exception
                        try
                        {
                            auto t = res.get();
                            (std::get<0>(t)).get();
                        }
                        catch ( my_exception& e)
                        {
                            BOOST_CHECK_MESSAGE(std::string(e.what_) == "my_exception","no what data");
                            BOOST_CHECK_MESSAGE(!std::string(e.file_).empty(),"no file data");
                            BOOST_CHECK_MESSAGE(e.line_ != -1,"no line data");
                        }
                        catch(...)
                        {
                            BOOST_FAIL( "unexpected exception in callback" );
                        }
                        aPromise->set_value();
           }// callback functor.
        );
        return fu;
    }
    std::future<void> start_async_work_timeout()
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant start_async_work not posted.");
        // we need a promise to inform caller when we're done
        std::shared_ptr<std::promise<void> > aPromise(new std::promise<void>);
        std::future<void> fu = aPromise->get_future();
        boost::asynchronous::any_shared_scheduler_proxy<> tp =get_worker();
        std::vector<boost::thread::id> ids = tp.thread_ids();
        // start long tasks
        post_callback(
           [ids](){
                    BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant work not posted.");

                    BOOST_CHECK_MESSAGE(contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task executed in the wrong thread");
                    return boost::asynchronous::parallel_invoke_timeout<boost::asynchronous::any_callable>(
                                std::chrono::milliseconds(1000),
                                boost::asynchronous::to_continuation_task(
                                    [](){boost::this_thread::sleep(boost::posix_time::milliseconds(500));}),
                                boost::asynchronous::to_continuation_task(
                                    [](){boost::this_thread::sleep(boost::posix_time::milliseconds(2000));return 42.0;}));
                    },// work
           [aPromise,ids,this](boost::asynchronous::expected<std::tuple<boost::asynchronous::expected<void>,
                                                                       boost::asynchronous::expected<double>>> res){
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant callback in main thread.");
                        BOOST_CHECK_MESSAGE(!contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task callback executed in the wrong thread(pool)");
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                        try
                        {
                            auto t = res.get();
                            BOOST_CHECK_MESSAGE(!(std::get<0>(t)).has_exception(),"first lambda threw an exception.");
                            BOOST_CHECK_MESSAGE((std::get<0>(t)).has_value(),"first task should be finished");
                            BOOST_CHECK_MESSAGE(!(std::get<1>(t)).has_exception(),"second lambda threw an exception. It should not have");
                        }
                        catch(std::exception& )
                        {
                            BOOST_FAIL( "unexpected exception in callback" );
                        }
                        aPromise->set_value();
           }// callback functor.
        );
        return fu;
    }
};

class ServantProxy : public boost::asynchronous::servant_proxy<ServantProxy,Servant>
{
public:
    template <class Scheduler>
    ServantProxy(Scheduler s):
        boost::asynchronous::servant_proxy<ServantProxy,Servant>(s)
    {}
    BOOST_ASYNC_FUTURE_MEMBER(start_async_work)
    BOOST_ASYNC_FUTURE_MEMBER(start_async_work_timeout)
};

struct ServantLog : boost::asynchronous::trackable_servant<servant_job,servant_job>
{
    typedef int simple_ctor;
    ServantLog(boost::asynchronous::any_weak_scheduler<servant_job> scheduler)
        : boost::asynchronous::trackable_servant<servant_job,servant_job>(scheduler,
                                               boost::asynchronous::make_shared_scheduler_proxy<
                                                   boost::asynchronous::threadpool_scheduler<
                                                           boost::asynchronous::lockfree_queue<servant_job>>>(6))
    {
    }
    ~ServantLog()
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant dtor not posted.");
        servant_dtor = true;
    }

    std::future<void> start_async_work()
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant start_async_work not posted.");
        // we need a promise to inform caller when we're done
        std::shared_ptr<std::promise<void> > aPromise(new std::promise<void>);
        std::future<void> fu = aPromise->get_future();
        boost::asynchronous::any_shared_scheduler_proxy<servant_job> tp =get_worker();
        std::vector<boost::thread::id> ids = tp.thread_ids();
        // start long tasks
        post_callback(
           [ids](){
                    BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant work not posted.");

                    BOOST_CHECK_MESSAGE(contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task executed in the wrong thread");
                    return boost::asynchronous::parallel_invoke<servant_job>(
                                boost::asynchronous::to_continuation_task([](){ASYNCHRONOUS_THROW (my_exception());}),
                                boost::asynchronous::to_continuation_task([](){return 42.0;}));
                    },// work
           [aPromise,ids,this](boost::asynchronous::expected<std::tuple<boost::asynchronous::expected<void>,
                                                                       boost::asynchronous::expected<double>>> res){
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant callback in main thread.");
                        BOOST_CHECK_MESSAGE(!contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task callback executed in the wrong thread(pool)");
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                        try
                        {
                            auto t = res.get();
                            BOOST_CHECK_MESSAGE((std::get<0>(t)).has_exception(),"first lambda did not throw an exception. It should have");
                            BOOST_CHECK_MESSAGE(!(std::get<1>(t)).has_exception(),"second lambda threw an exception. It should not have");
                            BOOST_CHECK_MESSAGE((std::get<1>(t)).get() == 42.0,"second lambda returned wrong result");
                        }
                        catch(...)
                        {
                            BOOST_FAIL( "unexpected exception in callback" );
                        }
                        // check correct exception
                        try
                        {
                            auto t = res.get();
                            (std::get<0>(t)).get();
                        }
                        catch ( my_exception& e)
                        {
                            BOOST_CHECK_MESSAGE(std::string(e.what_) == "my_exception","no what data");
                            BOOST_CHECK_MESSAGE(!std::string(e.file_).empty(),"no file data");
                            BOOST_CHECK_MESSAGE(e.line_ != -1,"no line data");
                        }
                        catch(...)
                        {
                            BOOST_FAIL( "unexpected exception in callback" );
                        }
                        aPromise->set_value();
           }// callback functor.
        ,"start_async_work");
        return fu;
    }
    std::future<void> start_async_work_timeout()
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant start_async_work not posted.");
        // we need a promise to inform caller when we're done
        std::shared_ptr<std::promise<void> > aPromise(new std::promise<void>);
        std::future<void> fu = aPromise->get_future();
        boost::asynchronous::any_shared_scheduler_proxy<servant_job> tp =get_worker();
        std::vector<boost::thread::id> ids = tp.thread_ids();
        // start long tasks
        post_callback(
           [ids](){
                    BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant work not posted.");

                    BOOST_CHECK_MESSAGE(contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task executed in the wrong thread");
                    return boost::asynchronous::parallel_invoke_timeout<servant_job>(
                                std::chrono::milliseconds(1000),
                                boost::asynchronous::to_continuation_task(
                                    [](){boost::this_thread::sleep(boost::posix_time::milliseconds(500));},"task1"),
                                boost::asynchronous::to_continuation_task(
                                    [](){boost::this_thread::sleep(boost::posix_time::milliseconds(2000));return 42.0;},"task2"));
                    },// work
           [aPromise,ids,this](boost::asynchronous::expected<std::tuple<boost::asynchronous::expected<void>,
                                                                       boost::asynchronous::expected<double>>> res){
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant callback in main thread.");
                        BOOST_CHECK_MESSAGE(!contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"task callback executed in the wrong thread(pool)");
                        BOOST_CHECK_MESSAGE(!res.has_exception(),"servant work threw an exception.");
                        try
                        {
                            auto t = res.get();
                            BOOST_CHECK_MESSAGE(!(std::get<0>(t)).has_exception(),"first lambda threw an exception.");
                            BOOST_CHECK_MESSAGE((std::get<0>(t)).has_value(),"first task should be finished");
                            BOOST_CHECK_MESSAGE(!(std::get<1>(t)).has_exception(),"second lambda threw an exception. It should not have");
                        }
                        catch(std::exception& )
                        {
                            BOOST_FAIL( "unexpected exception in callback" );
                        }
                        aPromise->set_value();
           }// callback functor.
        ,"start_async_work_timeout");
        return fu;
    }
};

class ServantLogProxy : public boost::asynchronous::servant_proxy<ServantLogProxy,ServantLog,servant_job>
{
public:
    template <class Scheduler>
    ServantLogProxy(Scheduler s):
        boost::asynchronous::servant_proxy<ServantLogProxy,ServantLog,servant_job>(s)
    {}
    BOOST_ASYNC_FUTURE_MEMBER_LOG(start_async_work,"start_async_work")
    BOOST_ASYNC_FUTURE_MEMBER_LOG(start_async_work_timeout,"start_async_work_timeout")
};

}

BOOST_AUTO_TEST_CASE( test_parallel_invoke )
{
    servant_dtor=false;
    {
        auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
                                                                            boost::asynchronous::lockfree_queue<>>>();

        main_thread_id = boost::this_thread::get_id();
        ServantProxy proxy(scheduler);
        auto fuv = proxy.start_async_work();
        try
        {
            auto resfuv = fuv.get();
            resfuv.get();
        }
        catch(...)
        {
            BOOST_FAIL( "unexpected exception" );
        }
    }
    BOOST_CHECK_MESSAGE(servant_dtor,"servant dtor not called.");
}

BOOST_AUTO_TEST_CASE( test_parallel_invoke_timeout )
{
    servant_dtor=false;
    {
        auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
                                                                            boost::asynchronous::lockfree_queue<>>>();

        main_thread_id = boost::this_thread::get_id();
        ServantProxy proxy(scheduler);
        auto fuv = proxy.start_async_work_timeout();
        try
        {
            auto resfuv = fuv.get();
            resfuv.get();
        }
        catch(...)
        {
            BOOST_FAIL( "unexpected exception" );
        }
    }
    BOOST_CHECK_MESSAGE(servant_dtor,"servant dtor not called.");
}

BOOST_AUTO_TEST_CASE( test_parallel_invoke_log )
{
    servant_dtor=false;
    {
        auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
                                                                            boost::asynchronous::lockfree_queue<servant_job>>>();

        main_thread_id = boost::this_thread::get_id();
        ServantLogProxy proxy(scheduler);
        auto fuv = proxy.start_async_work();
        try
        {
            auto resfuv = fuv.get();
            resfuv.get();
        }
        catch(...)
        {
            BOOST_FAIL( "unexpected exception" );
        }
    }
    BOOST_CHECK_MESSAGE(servant_dtor,"servant dtor not called.");
}

BOOST_AUTO_TEST_CASE( test_parallel_invoke_timeout_log )
{
    servant_dtor=false;
    {
        auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
                                                                            boost::asynchronous::lockfree_queue<servant_job>>>();

        main_thread_id = boost::this_thread::get_id();
        ServantLogProxy proxy(scheduler);
        auto fuv = proxy.start_async_work_timeout();
        try
        {
            auto resfuv = fuv.get();
            resfuv.get();
        }
        catch(...)
        {
            BOOST_FAIL( "unexpected exception" );
        }
    }
    BOOST_CHECK_MESSAGE(servant_dtor,"servant dtor not called.");
}



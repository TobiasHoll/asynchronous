#include <iostream>
#include <boost/asynchronous/scheduler/single_thread_scheduler.hpp>
#include <boost/asynchronous/extensions/asio/asio_scheduler.hpp>
#include <boost/asynchronous/queue/lockfree_queue.hpp>
#include <boost/asynchronous/scheduler_shared_proxy.hpp>
#include <boost/asynchronous/servant_proxy.hpp>
#include <boost/asynchronous/trackable_servant.hpp>
#include <boost/asynchronous/extensions/asio/asio_deadline_timer.hpp>
#include "test_common.hpp"

#include <boost/test/unit_test.hpp>

using namespace std;
using namespace boost::asynchronous::test;

namespace
{
// main thread id
boost::thread::id main_thread_id;
unsigned timer_expired_count=0;
unsigned timer_cancelled_count=0;

struct Servant : boost::asynchronous::trackable_servant<>
{
    Servant(boost::asynchronous::any_weak_scheduler<> scheduler) 
        : boost::asynchronous::trackable_servant<>(scheduler,
                                                   // as timer servant we use an asio-based scheduler with 1 thread
                                                   boost::asynchronous::make_shared_scheduler_proxy<
                                                       boost::asynchronous::asio_scheduler<>>(1))
        , m_timer(get_worker(),boost::posix_time::milliseconds(500))
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant ctor not posted.");
        m_threadid = boost::this_thread::get_id();
    }
    ~Servant()
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant dtor not posted.");
    } 
    void timer_expired(std::shared_ptr<std::promise<void> > p)
    {
        boost::asynchronous::any_shared_scheduler<> s = get_scheduler().lock();
        std::vector<boost::thread::id> ids = s.thread_ids();
        BOOST_CHECK_MESSAGE(contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"timer_expired running in wrong thread.");
        boost::thread::id threadid = m_threadid;
        
        typename std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();

        async_wait(m_timer,
                   [p,start,threadid](const ::boost::system::error_code& err){
                       BOOST_CHECK_MESSAGE(threadid==boost::this_thread::get_id(),"timer callback in wrong thread.");
                       BOOST_CHECK_MESSAGE(!err,"timer not expired.");
                       typename std::chrono::high_resolution_clock::time_point stop = std::chrono::high_resolution_clock::now();
                       auto d =  std::chrono::nanoseconds(stop - start).count();
                       BOOST_CHECK_MESSAGE(d/1000000 >= 400,"timer was too long.");
                       BOOST_CHECK_MESSAGE(d/1000000 >= 490,"timer was too short.");
                       ++timer_expired_count;
                       p->set_value();
                   });
    }
    void timer_cancelled(std::shared_ptr<std::promise<void> > p)
    {
        m_timer =  boost::asynchronous::asio_deadline_timer_proxy(get_worker(),boost::posix_time::milliseconds(50000));
        boost::asynchronous::any_shared_scheduler<> s = get_scheduler().lock();
        std::vector<boost::thread::id> ids = s.thread_ids();
        BOOST_CHECK_MESSAGE(contains_id(ids.begin(),ids.end(),boost::this_thread::get_id()),"timer_cancelled running in wrong thread.");
        boost::thread::id threadid = m_threadid;
        
        typename std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
        
        async_wait(m_timer,
                   [p,start,threadid](const ::boost::system::error_code& err){
                       BOOST_CHECK_MESSAGE(threadid==boost::this_thread::get_id(),"timer callback in wrong thread.");
                       BOOST_CHECK_MESSAGE(err,"timer not expired.");
                       typename std::chrono::high_resolution_clock::time_point stop = std::chrono::high_resolution_clock::now();
                       auto d =  std::chrono::nanoseconds(stop - start).count();
                       BOOST_CHECK_MESSAGE(d/1000000 < 49999,"timer was too long.");
                       ++timer_cancelled_count;
                       p->set_value();
        });
        // cancel timer
        m_timer.cancel();
    }
private:
    boost::asynchronous::asio_deadline_timer_proxy m_timer;
    boost::thread::id m_threadid;
};

class ServantProxy : public boost::asynchronous::servant_proxy<ServantProxy,Servant>
{
public:
    template <class Scheduler>
    ServantProxy(Scheduler s):
        boost::asynchronous::servant_proxy<ServantProxy,Servant>(s)
    {}
    BOOST_ASYNC_FUTURE_MEMBER(timer_expired)
    BOOST_ASYNC_FUTURE_MEMBER(timer_cancelled)
};

}

BOOST_AUTO_TEST_CASE( test_asio_timer_cancelled )
{        
    auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
                                                                        boost::asynchronous::lockfree_queue<>>>();
    
    main_thread_id = boost::this_thread::get_id();   
    ServantProxy proxy(scheduler);
    std::shared_ptr<std::promise<void> > p(new std::promise<void>);
    auto fu = p->get_future();
    auto fuv = proxy.timer_cancelled(p);
    fu.get();
    BOOST_CHECK_MESSAGE(timer_cancelled_count==1,"timer callback not called.");
}


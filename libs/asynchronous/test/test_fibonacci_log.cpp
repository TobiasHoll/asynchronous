#include <iostream>
#include <tuple>
#include <utility>
#include <map>
#include <future>

#include <boost/lexical_cast.hpp>

#include <boost/asynchronous/scheduler/single_thread_scheduler.hpp>
#include <boost/asynchronous/queue/lockfree_queue.hpp>
#include <boost/asynchronous/scheduler_shared_proxy.hpp>
#include <boost/asynchronous/scheduler/multiqueue_threadpool_scheduler.hpp>
#include <boost/asynchronous/scheduler/detail/any_continuation.hpp>

#include <boost/asynchronous/servant_proxy.hpp>
#include <boost/asynchronous/post.hpp>
#include <boost/asynchronous/trackable_servant.hpp>
#include <boost/asynchronous/continuation_task.hpp>

#include "test_common.hpp"
#include <boost/test/unit_test.hpp>
using namespace boost::asynchronous::test;
namespace
{
typedef boost::asynchronous::any_loggable servant_job;
typedef std::map<std::string,std::list<boost::asynchronous::diagnostic_item> > diag_type;

// main thread id
boost::thread::id main_thread_id;
std::vector<boost::thread::id> tpids;

long serial_fib( long n ) {
    if( n<2 )
        return n;
    else
        return serial_fib(n-1)+serial_fib(n-2);
}

struct fib_task : public boost::asynchronous::continuation_task<long>
{
    // task name is its fibonacci number
    fib_task(long n,long cutoff)
        : boost::asynchronous::continuation_task<long>(boost::lexical_cast<std::string>(n))
        , n_(n),cutoff_(cutoff){}

    void operator()()const
    {
        if (!contains_id(tpids.begin(),tpids.end(),boost::this_thread::get_id()))
        {
            BOOST_FAIL("fib_task runs in wrong thread");
        }
        boost::asynchronous::continuation_result<long> task_res = this_task_result();
        if (n_<cutoff_)
        {
            task_res.set_value(serial_fib(n_));
        }
        else
        {
            boost::asynchronous::create_callback_continuation_job<servant_job>(
                        [task_res](std::tuple<boost::asynchronous::expected<long>,boost::asynchronous::expected<long> > res)
                        {
                            long r = std::get<0>(res).get() + std::get<1>(res).get();
                            task_res.set_value(r);
                        },
                        fib_task(n_-1,cutoff_),
                        fib_task(n_-2,cutoff_));
        }
    }
    long n_;
    long cutoff_;
};

struct Servant : boost::asynchronous::trackable_servant<servant_job,servant_job>
{
    // optional, ctor is simple enough not to be posted
    typedef int simple_ctor;
    Servant(boost::asynchronous::any_weak_scheduler<servant_job> scheduler)
        : boost::asynchronous::trackable_servant<servant_job,servant_job>(scheduler,
                                               // threadpool with 4 threads and a simple lockfree_queue
                                               boost::asynchronous::make_shared_scheduler_proxy<
                                                   boost::asynchronous::multiqueue_threadpool_scheduler<
                                                           boost::asynchronous::lockfree_queue<servant_job>>>(4))
        // for testing purpose
        , m_promise(new std::promise<long>)
    {
    }
    // called when task done, in our thread
    void on_callback(long res)
    {
        // inform test caller
        m_promise->set_value(res);
    }
    // call to this is posted and executes in our (safe) single-thread scheduler
    std::future<long> calc_fibonacci(long n,long cutoff)
    {
        BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant calc_fibonacci not posted.");
        // for testing purpose
        auto fu = m_promise->get_future();
        boost::asynchronous::any_shared_scheduler_proxy<servant_job> tp =get_worker();
        tpids = tp.thread_ids();
        // start long tasks in threadpool (first lambda) and callback in our thread
        post_callback(
                [n,cutoff]()
                {
                    BOOST_CHECK_MESSAGE(contains_id(tpids.begin(),tpids.end(),boost::this_thread::get_id()),"task executed in the wrong thread");
                    return boost::asynchronous::top_level_callback_continuation_job<long,servant_job>(fib_task(n,cutoff));
                 }// work
               ,
               // the lambda calls Servant, just to show that all is safe, Servant is alive if this is called
               [this](boost::asynchronous::expected<long> res){
                            BOOST_CHECK_MESSAGE(!contains_id(tpids.begin(),tpids.end(),boost::this_thread::get_id()),"fibonacci callback executed in the wrong thread(pool)");
                            BOOST_CHECK_MESSAGE(main_thread_id!=boost::this_thread::get_id(),"servant callback in main thread.");
                            BOOST_CHECK_MESSAGE(res.has_value(),"callback has a blocking future.");
                            this->on_callback(res.get());
               },// callback functor.
               "calc_fibonacci"
        );
        return fu;
    }
    // threadpool diagnostics
    diag_type get_diagnostics() const
    {
        return get_worker().get_diagnostics().totals();
    }
private:
// for testing
std::shared_ptr<std::promise<long> > m_promise;
};
class ServantProxy : public boost::asynchronous::servant_proxy<ServantProxy,Servant,servant_job>
{
public:
    template <class Scheduler>
    ServantProxy(Scheduler s):
        boost::asynchronous::servant_proxy<ServantProxy,Servant,servant_job>(s)
    {}
    // caller will get a future
#ifndef _MSC_VER
    BOOST_ASYNC_FUTURE_MEMBER_LOG(calc_fibonacci,"proxy::calc_fibonacci")
    BOOST_ASYNC_FUTURE_MEMBER_LOG(get_diagnostics,"proxy::get_diagnostics")
#else
    BOOST_ASYNC_FUTURE_MEMBER_LOG_2(calc_fibonacci, "proxy::calc_fibonacci")
    BOOST_ASYNC_FUTURE_MEMBER_LOG_2(get_diagnostics, "proxy::get_diagnostics")
#endif
};
}

BOOST_AUTO_TEST_CASE( test_fibonacci_log_30_18 )
{
    long fibo_val = 30;
    long cutoff = 18;
    main_thread_id = boost::this_thread::get_id();

    long sres = serial_fib(fibo_val);
    {
        // a single-threaded world, where Servant will live.
        auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<
                                boost::asynchronous::single_thread_scheduler<
                                     boost::asynchronous::lockfree_queue<servant_job>>>();
        {
            ServantProxy proxy(scheduler);
            auto fu = proxy.calc_fibonacci(fibo_val,cutoff);
            auto resfu = fu.get();
            long res = resfu.get();
            BOOST_CHECK_MESSAGE(sres == res,"fibonacci parallel and serial version found different results");
            // check if we found all tasks and just these ones (30..16 must be found)
            auto fu_diag = proxy.get_diagnostics();
            diag_type diag = fu_diag.get();
            std::map<int,bool> tasks;
            for (int i=16; i<31;++i)
            {
                tasks[i]=false;
            }

            for (auto mit = diag.begin(); mit != diag.end() ; ++mit)
            {
                if ((*mit).first == "calc_fibonacci")
                {
                    continue;
                }
                int name = boost::lexical_cast<int>((*mit).first);
                tasks[name]=true;
                BOOST_CHECK_MESSAGE(name <= 30 && name > 15 ,"unexpected fibonacci task");
                for (auto jit = (*mit).second.begin(); jit != (*mit).second.end();++jit)
                {
                    BOOST_CHECK_MESSAGE(!(*jit).is_interrupted(),"no task should have been interrupted.");
                    BOOST_CHECK_MESSAGE(!(*jit).is_failed(),"no task should have failed.");
                }
            }
            //no more possible to test task name called with optimization that some tasks are called immediately instead of being posted
        }
    }
}



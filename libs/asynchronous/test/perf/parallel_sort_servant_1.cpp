#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cmath>


#include <algorithm>
#include <iostream>
#include <vector>

#include <boost/smart_ptr/shared_array.hpp>
#include <boost/asynchronous/queue/lockfree_queue.hpp>
#include <boost/asynchronous/servant_proxy.hpp>
#include <boost/asynchronous/scheduler/multiqueue_threadpool_scheduler.hpp>
#include <boost/asynchronous/scheduler/single_thread_scheduler.hpp>
#include <boost/asynchronous/queue/any_queue_container.hpp>
#include <boost/asynchronous/scheduler_shared_proxy.hpp>
#include <boost/asynchronous/trackable_servant.hpp>
#include <boost/asynchronous/algorithm/parallel_sort.hpp>

#include <boost/lexical_cast.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/utility/enable_if.hpp>

using namespace std;
#define LOOP 1

//#define NELEM 200000000
//#define SORTED_TYPE uint32_t

//#define NELEM 10000000
//#define SORTED_TYPE std::string

#define NELEM 200000000
#define SORTED_TYPE double

typename boost::chrono::high_resolution_clock::time_point servant_time;
double servant_intern=0.0;
long tpsize = 12;
long tasks = 48;

template <class T, class U>
typename boost::disable_if<boost::is_same<T,U>,U >::type
test_cast(T const& t)
{
    return boost::lexical_cast<U>(t);
}
template <class T, class U>
typename boost::enable_if<boost::is_same<T,U>,U >::type
test_cast(T const& t)
{
    return t;
}

struct Servant : boost::asynchronous::trackable_servant<>
{
    typedef int simple_ctor;
    typedef int requires_weak_scheduler;
    Servant(boost::asynchronous::any_weak_scheduler<> scheduler)
        : boost::asynchronous::trackable_servant<>(scheduler,
                                               boost::asynchronous::create_shared_scheduler_proxy(
                                                   new boost::asynchronous::multiqueue_threadpool_scheduler<
                                                           boost::asynchronous::lockfree_queue<>,
                                                           boost::asynchronous::default_find_position< boost::asynchronous::sequential_push_policy>,
                                                           boost::asynchronous::no_cpu_load_saving
                                                       >(tpsize,tasks)))
        , m_promise(new boost::promise<void>)
    {
    }
    ~Servant(){}

    // called when task done, in our thread
    void on_callback()
    {
        servant_intern += (boost::chrono::nanoseconds(boost::chrono::high_resolution_clock::now() - servant_time).count() / 1000000);
        m_promise->set_value();
    }

    boost::shared_future<void> do_sort(SORTED_TYPE a[], size_t n)
    {
        boost::shared_future<void> fu = m_promise->get_future();
        long tasksize = NELEM / tasks;
        servant_time = boost::chrono::high_resolution_clock::now();
        post_callback(
               [a,n,tasksize](){
                        return boost::asynchronous::parallel_sort(a,a+n,std::less<SORTED_TYPE>(),tasksize,"",0);
                      },// work
               // the lambda calls Servant, just to show that all is safe, Servant is alive if this is called
               [this](boost::asynchronous::expected<void> /*res*/){
                            this->on_callback();
               }// callback functor.
               ,"",0,0
        );
        return fu;
    }
    boost::shared_future<void> do_spreadsort(SORTED_TYPE a[], size_t n)
    {
        boost::shared_future<void> fu = m_promise->get_future();
        long tasksize = NELEM / tasks;
        servant_time = boost::chrono::high_resolution_clock::now();
        post_callback(
               [a,n,tasksize](){
                        return boost::asynchronous::parallel_spreadsort(a,a+n,std::less<SORTED_TYPE>(),tasksize,"",0);
                      },// work
               // the lambda calls Servant, just to show that all is safe, Servant is alive if this is called
               [this](boost::asynchronous::expected<void> /*res*/){
                            this->on_callback();
               }// callback functor.
               ,"",0,0
        );
        return fu;
    }
private:
// for testing
boost::shared_ptr<boost::promise<void> > m_promise;
};
class ServantProxy : public boost::asynchronous::servant_proxy<ServantProxy,Servant>
{
public:
    template <class Scheduler>
    ServantProxy(Scheduler s):
        boost::asynchronous::servant_proxy<ServantProxy,Servant>(s)
    {}
    BOOST_ASYNC_FUTURE_MEMBER(do_sort,0)
    BOOST_ASYNC_FUTURE_MEMBER(do_spreadsort,0)
};
void ParallelAsyncPostCb(SORTED_TYPE a[], size_t n)
{
    auto scheduler = boost::asynchronous::create_shared_scheduler_proxy(
                            new boost::asynchronous::single_thread_scheduler<boost::asynchronous::lockfree_queue<>,
                                                                             boost::asynchronous::default_save_cpu_load<10,80000,1000>>(tpsize));
    {
        ServantProxy proxy(scheduler);

        boost::shared_future<boost::shared_future<void> > fu = proxy.do_sort(a,n);
        boost::shared_future<void> resfu = fu.get();
        resfu.get();
    }
}
void ParallelAsyncPostCbSpreadsort(SORTED_TYPE a[], size_t n)
{
    auto scheduler = boost::asynchronous::create_shared_scheduler_proxy(
                            new boost::asynchronous::single_thread_scheduler<boost::asynchronous::lockfree_queue<>,
                                                                             boost::asynchronous::default_save_cpu_load<10,80000,1000>>(tpsize));
    {
        ServantProxy proxy(scheduler);

        boost::shared_future<boost::shared_future<void> > fu = proxy.do_spreadsort(a,n);
        boost::shared_future<void> resfu = fu.get();
        resfu.get();
    }
}
    


void test_sorted_elements(void(*pf)(SORTED_TYPE [], size_t ))
{
    boost::shared_array<SORTED_TYPE> a (new SORTED_TYPE[NELEM]);
    for ( uint32_t i = 0 ; i < NELEM ; ++i)
    {
        *(a.get()+i) = test_cast<uint32_t,SORTED_TYPE>( i+NELEM) ;
    }
    (*pf)(a.get(),NELEM);
}
void test_random_elements_many_repeated(void(*pf)(SORTED_TYPE [], size_t ))
{
    boost::shared_array<SORTED_TYPE> a (new SORTED_TYPE[NELEM]);
    for ( uint32_t i = 0 ; i < NELEM ; ++i)
    {
        *(a.get()+i) = test_cast<uint32_t,SORTED_TYPE>(rand() % 10000) ;
    }
    (*pf)(a.get(),NELEM);
}
void test_random_elements_few_repeated(void(*pf)(SORTED_TYPE [], size_t ))
{
    boost::shared_array<SORTED_TYPE> a (new SORTED_TYPE[NELEM]);
    for ( uint32_t i = 0 ; i < NELEM ; ++i)
    {
        *(a.get()+i) = test_cast<uint32_t,SORTED_TYPE>(rand());
    }
    (*pf)(a.get(),NELEM);
}
void test_random_elements_quite_repeated(void(*pf)(SORTED_TYPE [], size_t ))
{
    boost::shared_array<SORTED_TYPE> a (new SORTED_TYPE[NELEM]);
    for ( uint32_t i = 0 ; i < NELEM ; ++i)
    {
        *(a.get()+i) = test_cast<uint32_t,SORTED_TYPE>(rand() % (NELEM/2)) ;
    }
    (*pf)(a.get(),NELEM);
}
void test_reversed_sorted_elements(void(*pf)(SORTED_TYPE [], size_t ))
{
    boost::shared_array<SORTED_TYPE> a (new SORTED_TYPE[NELEM]);
    for ( uint32_t i = 0 ; i < NELEM ; ++i)
    {
        *(a.get()+i) = test_cast<uint32_t,SORTED_TYPE>((NELEM<<1) -i) ;
    }
    (*pf)(a.get(),NELEM);
}
void test_equal_elements(void(*pf)(SORTED_TYPE [], size_t ))
{
    boost::shared_array<SORTED_TYPE> a (new SORTED_TYPE[NELEM]);
    for ( uint32_t i = 0 ; i < NELEM ; ++i)
    {
        *(a.get()+i) = test_cast<uint32_t,SORTED_TYPE>(NELEM) ;
    }
    (*pf)(a.get(),NELEM);
}
int main( int argc, const char *argv[] ) 
{           
    tpsize = (argc>1) ? strtol(argv[1],0,0) : boost::thread::hardware_concurrency();
    tasks = (argc>2) ? strtol(argv[2],0,0) : 500;
    std::cout << "tpsize=" << tpsize << std::endl;
    std::cout << "tasks=" << tasks << std::endl;   
    
    servant_intern=0.0;
    for (int i=0;i<LOOP;++i)
    {     
        test_random_elements_many_repeated(ParallelAsyncPostCb);
    }
    printf ("%50s: time = %.1f msec\n","test_random_elements_many_repeated", servant_intern);
    
    servant_intern=0.0;
    for (int i=0;i<LOOP;++i)
    {     
        test_random_elements_few_repeated(ParallelAsyncPostCb);
    }
    printf ("%50s: time = %.1f msec\n","test_random_elements_few_repeated", servant_intern);
    
    servant_intern=0.0;
    for (int i=0;i<LOOP;++i)
    {     
        test_random_elements_quite_repeated(ParallelAsyncPostCb);
    }
    printf ("%50s: time = %.1f msec\n","test_random_elements_quite_repeated", servant_intern);
    
    servant_intern=0.0;
    for (int i=0;i<LOOP;++i)
    {     
        test_sorted_elements(ParallelAsyncPostCb);
    }
    printf ("%50s: time = %.1f msec\n","test_sorted_elements", servant_intern);
    
    servant_intern=0.0;
    for (int i=0;i<LOOP;++i)
    {     
        test_reversed_sorted_elements(ParallelAsyncPostCb);
    }
    printf ("%50s: time = %.1f msec\n","test_reversed_sorted_elements", servant_intern);
    
    servant_intern=0.0;
    for (int i=0;i<LOOP;++i)
    {     
        test_equal_elements(ParallelAsyncPostCb);
    }
    printf ("%50s: time = %.1f msec\n","test_equal_elements", servant_intern);
    
    std::cout << std::endl;
    
    // boost spreadsort
    servant_intern=0.0;
    for (int i=0;i<LOOP;++i)
    {     
        test_random_elements_many_repeated(ParallelAsyncPostCbSpreadsort);
    }
    printf ("%50s: time = %.1f msec\n","Spreadsort: test_random_elements_many_repeated", servant_intern);
    
    servant_intern=0.0;
    for (int i=0;i<LOOP;++i)
    {     
        test_random_elements_few_repeated(ParallelAsyncPostCbSpreadsort);
    }
    printf ("%50s: time = %.1f msec\n","Spreadsort: test_random_elements_few_repeated", servant_intern);
    
    servant_intern=0.0;
    for (int i=0;i<LOOP;++i)
    {     
        test_random_elements_quite_repeated(ParallelAsyncPostCbSpreadsort);
    }
    printf ("%50s: time = %.1f msec\n","Spreadsort: test_random_elements_quite_repeated", servant_intern);
    
    servant_intern=0.0;
    for (int i=0;i<LOOP;++i)
    {     
        test_sorted_elements(ParallelAsyncPostCbSpreadsort);
    }
    printf ("%50s: time = %.1f msec\n","Spreadsort: test_sorted_elements", servant_intern);
    
    servant_intern=0.0;
    for (int i=0;i<LOOP;++i)
    {     
        test_reversed_sorted_elements(ParallelAsyncPostCbSpreadsort);
    }
    printf ("%50s: time = %.1f msec\n","Spreadsort: test_reversed_sorted_elements", servant_intern);
    
    servant_intern=0.0;
    for (int i=0;i<LOOP;++i)
    {     
        test_equal_elements(ParallelAsyncPostCbSpreadsort);
    }
    printf ("%50s: time = %.1f msec\n","Spreadsort: test_equal_elements", servant_intern);
    return 0;
}

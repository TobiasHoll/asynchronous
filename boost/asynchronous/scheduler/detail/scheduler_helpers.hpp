// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2013
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNC_SCHEDULER_SCHEDULER_HELPERS_HPP
#define BOOST_ASYNC_SCHEDULER_SCHEDULER_HELPERS_HPP

#include<string>
#include <functional>

#include <memory>
#include <boost/thread/thread.hpp>
#include <boost/asynchronous/scheduler/detail/exceptions.hpp>
#include <boost/asynchronous/scheduler/detail/processor_bind.hpp>

#ifdef BOOST_ASYNCHRONOUS_PRCTL_SUPPORT
#include <sys/prctl.h>
#endif

namespace boost { namespace asynchronous { namespace detail
{
// terminates processing of a given scheduler
template <class Diag, class ThreadType>
struct default_termination_task: public Diag
{
    default_termination_task(std::function<void()> fct = std::function<void()>())
        :m_fct(std::move(fct)){}
    default_termination_task(default_termination_task&&)=default;
    default_termination_task(default_termination_task const&)=default;
    default_termination_task& operator= (default_termination_task&&)=default;
    default_termination_task& operator= (default_termination_task const&)=default;

    void operator()()const
    {
        if (m_fct)
            m_fct();
        throw boost::asynchronous::detail::shutdown_exception();
    }
    std::function<void()> m_fct;
};

template<class ThreadType>
struct thread_join_helper
{
    template <class T>
    static void join(T* t)
    {
        t->join();
    }
};
template<>
struct thread_join_helper<boost::thread_group>
{
    template <class T>
    static void join(T* t)
    {
        t->join_all();        
    }
};

template <class ThreadType>
struct worker_wrap
{
    worker_wrap(std::shared_ptr<ThreadType> g):m_group(g){}
    void join()
    {
        thread_join_helper<ThreadType>::join(m_group.get());
    }
    std::shared_ptr<ThreadType> m_group;
};
template <class Diag>
struct set_name_task: public Diag
{
    set_name_task( std::string const& n): m_name(n){}
    set_name_task(set_name_task&& rhs)noexcept
        : m_name(std::move(rhs.m_name))
    {}
    set_name_task(set_name_task const& rhs)noexcept
        : Diag(),m_name(rhs.m_name)
    {}
    set_name_task& operator= (set_name_task&& rhs)noexcept
    {
        std::swap(m_name,rhs.m_name);
        return *this;
    }
    set_name_task& operator= (set_name_task const& rhs)noexcept
    {
        m_name = rhs.m_name;
        return *this;
    }
    void operator()()const
    {
#ifdef BOOST_ASYNCHRONOUS_PRCTL_SUPPORT
        prctl(PR_SET_NAME, m_name.c_str(), 0, 0, 0);
#endif
    }
    std::string m_name;
};

}}}
#endif // BOOST_ASYNC_SCHEDULER_SCHEDULER_HELPERS_HPP

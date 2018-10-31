// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2013
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNCHRONOUS_PARALLEL_INVOKE_HPP
#define BOOST_ASYNCHRONOUS_PARALLEL_INVOKE_HPP

#include <algorithm>
#include <vector>
#include <tuple>

#include <boost/serialization/vector.hpp>

#include <boost/asynchronous/detail/any_interruptible.hpp>
#include <boost/asynchronous/callable_any.hpp>
#include <boost/asynchronous/detail/continuation_impl.hpp>
#include <boost/asynchronous/continuation_task.hpp>
#include <boost/asynchronous/post.hpp>
#include <boost/asynchronous/scheduler/serializable_task.hpp>
#include <boost/asynchronous/detail/metafunctions.hpp>


namespace boost { namespace asynchronous
{
namespace detail
{
template <class ReturnType, class Job,typename... Args>
struct parallel_invoke_helper: public boost::asynchronous::continuation_task<ReturnType>
{
    parallel_invoke_helper(ReturnType expected_tuple,Args... args)
        : boost::asynchronous::continuation_task<ReturnType>("parallel_invoke")
        , m_expected_tuple(std::move(expected_tuple))
        , m_tuple(std::make_tuple(std::move(args)...))
    {
    }
    void operator()()
    {
        boost::asynchronous::continuation_result<ReturnType> task_res = this->this_task_result();
        try
        {
            boost::asynchronous::create_callback_continuation_job<Job>(
                        // called when subtasks are done, set our result
                        [task_res](ReturnType res) mutable
                        {
                            task_res.set_value(std::move(res));
                        },
                        std::move(m_expected_tuple),
                        // recursive tasks
                        std::move(m_tuple));
        }
        catch(...)
        {
            task_res.set_exception(std::current_exception());
        }
    }
    template <class Archive>
    void serialize(Archive & /*ar*/, const unsigned int /*version*/)
    {
    }
    ReturnType m_expected_tuple;
    std::tuple<Args...> m_tuple;
};
template <class ReturnType, class Job,class Duration,typename... Args>
struct parallel_invoke_helper_timeout: public boost::asynchronous::continuation_task<ReturnType>
{
    parallel_invoke_helper_timeout(Duration const& d,ReturnType expected_tuple,Args... args)
        : boost::asynchronous::continuation_task<ReturnType>("parallel_invoke_timeout")
        , m_expected_tuple(std::move(expected_tuple))
        , m_tuple(std::make_tuple(std::move(args)...))
        , m_duration(d)
    {
    }
    void operator()()
    {
        boost::asynchronous::continuation_result<ReturnType> task_res = this->this_task_result();
        try
        {
            boost::asynchronous::create_callback_continuation_job_timeout<Job>(
                        // called when subtasks are done, set our result
                        [task_res](ReturnType res) mutable
                        {
                            task_res.set_value(std::move(res));
                        },
                        m_duration,
                        std::move(m_expected_tuple),
                        // recursive tasks
                        std::move(m_tuple));
        }
        catch(...)
        {
            task_res.set_exception(std::current_exception());
        }
    }
    template <class Archive>
    void serialize(Archive & /*ar*/, const unsigned int /*version*/)
    {
    }
    ReturnType m_expected_tuple;
    std::tuple<Args...> m_tuple;
    Duration m_duration;
};
}
template <class Return, class Func>
struct continuation_task_wrapper : public boost::asynchronous::continuation_task<Return>
{
    continuation_task_wrapper(Func&& f,const std::string& task_name)
        : boost::asynchronous::continuation_task<Return>(task_name)
        , func_(std::forward<Func>(f)){}
    void operator()()
    {
        boost::asynchronous::continuation_result<Return> task_res = this->this_task_result();
        try
        {
            task_res.set_value(std::move(func_()));
        }
        catch(...)
        {
            task_res.set_exception(std::current_exception());
        }
    }
    template <class Archive>
    void serialize(Archive & ar, const unsigned int /*version*/)
    {
        ar & func_;
    }
    Func func_;
};
template <class Func>
struct continuation_task_wrapper<void,Func> : public boost::asynchronous::continuation_task<void>
{
    continuation_task_wrapper(Func&& f,const std::string& task_name)
        : boost::asynchronous::continuation_task<void>(task_name)
        , func_(std::forward<Func>(f)){}
    void operator()()
    {
        boost::asynchronous::continuation_result<void> task_res = this->this_task_result();
        try
        {
            func_();
            task_res.set_value();
        }
        catch(...)
        {
            task_res.set_exception(std::current_exception());
        }
    }
    template <class Archive>
    void serialize(Archive & ar, const unsigned int /*version*/)
    {
        ar & func_;
    }
    Func func_;
};
template <class T>
auto to_continuation_task(T task,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
           const std::string& task_name)
#else
           const std::string& task_name="")
#endif
-> boost::asynchronous::continuation_task_wrapper<decltype(task()),T>
{
    return boost::asynchronous::continuation_task_wrapper<decltype(task()),T>(std::move(task),task_name);
}

template <class Job, typename... Args>
auto parallel_invoke(Args... args)
    -> boost::asynchronous::detail::callback_continuation<decltype(boost::asynchronous::detail::make_expected_tuple(args...)),Job>
{
    typedef decltype(boost::asynchronous::detail::make_expected_tuple(args...)) ReturnType;

    return boost::asynchronous::top_level_callback_continuation_job<ReturnType,Job>
            (boost::asynchronous::detail::parallel_invoke_helper<ReturnType,Job,Args...>
               (boost::asynchronous::detail::make_expected_tuple(args...),std::move(args)...));
}
// version with timeout
template <class Job, class Duration, typename... Args>
auto parallel_invoke_timeout(Duration const& d, Args... args)
    -> boost::asynchronous::detail::callback_continuation<decltype(boost::asynchronous::detail::make_expected_tuple(args...)),Job>
{
    typedef decltype(boost::asynchronous::detail::make_expected_tuple(args...)) ReturnType;

    return boost::asynchronous::top_level_callback_continuation_job<ReturnType,Job>
            (boost::asynchronous::detail::parallel_invoke_helper_timeout<ReturnType,Job,Duration,Args...>
               (d,boost::asynchronous::detail::make_expected_tuple(args...),std::move(args)...));
}
}}

#endif //BOOST_ASYNCHRONOUS_PARALLEL_INVOKE_HPP

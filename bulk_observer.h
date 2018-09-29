#pragma once
#include <vector>
#include <string>
#include <algorithm>
#include <iostream>
#include <memory>
#include <cstddef>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <queue>
#include <utility>
#include <thread>
#include "metrics.h"
#include "bulk.h"

//#define DEBUG

class Observable;
class Observer : public std::enable_shared_from_this<Observer>
{
private:
  std::vector<std::weak_ptr<Observable>> _observables;

public:
  virtual void update(BulkStorage &source, std::size_t) = 0;
  void subscribe_on_observable(const std::weak_ptr<Observable> &);
  void unsubscribe_on_observable(const std::weak_ptr<Observable> &);
  virtual ~Observer(){};

protected:
  virtual void printOstream(std::ostream &out, const std::vector<std::string> &vecStr)
  {
    if (!vecStr.empty())
    {
      out << "bulk: ";
      for (const auto &cmd : vecStr)
        out << cmd << (&cmd != &vecStr.back() ? ", " : "");
      out << std::endl;
    }
  }
};

class ToConsolePrint : public Observer
{
private:
  std::condition_variable cv_queue;
  std::mutex console_mutex;
  std::atomic<bool> finished{false};
  std::queue<std::pair<std::size_t, std::vector<std::string>>> data_queue;
  std::vector<std::thread> console_threads;
  std::ostream &_out;
  std::string thread_base_name{"log"};

public:
  ToConsolePrint(std::ostream &out, std::size_t threads_count = 1) : Observer(), _out{out},
                                                                     finished{false} { start(threads_count); }
  void update(BulkStorage &, std::size_t) override;
  static std::shared_ptr<ToConsolePrint> create(std::ostream &out, const std::weak_ptr<Observable> &_obs, std::size_t threads_count = 1)
  {
    std::shared_ptr _tmpToConsolePrint = std::make_shared<ToConsolePrint>(out, threads_count);
    auto tmpObservable = _obs.lock();
    if (tmpObservable)
    {
      _tmpToConsolePrint->subscribe_on_observable(tmpObservable);
      tmpObservable.reset();
    }
    return _tmpToConsolePrint;
  }
  static std::shared_ptr<ToConsolePrint> create(std::ostream &out, std::size_t threads_count = 1)
  {
    return std::make_shared<ToConsolePrint>(out, threads_count);
  }

  void start(std::size_t threads_count = 1)
  {
    if (!threads_count)
      threads_count = 1;
    for (auto i = 0; i < threads_count; ++i)
    {
      std::thread createdThread = std::thread(&ToConsolePrint::printOut, this);
      MetricsCount::Instance().regThread(createdThread.get_id(), thread_base_name);
      console_threads.emplace_back(std::move(createdThread));
    }
#ifdef DEBUG
    std::cout << "Console_log thread created" << console_threads.size() << std::endl;
#endif
  }

  void stop()
  {
    finished = true;
    cv_queue.notify_all();
    if (console_threads.empty())
    {
      return;
    }
    for (auto &thread : console_threads)
      if (thread.joinable())
        thread.join();

#ifdef DEBUG
    std::cout << "Console_log threads stopped" << std::endl;
#endif
    console_threads.clear();
  }
  void printOut();

  virtual ~ToConsolePrint()
  {
    stop();
  }
};

class ToFilePrint : public Observer
{
private:
  std::condition_variable cv_queue;
  std::mutex file_mutex;
  std::atomic<bool> finished;
  std::queue<std::pair<std::size_t, std::vector<std::string>>> data_queue;
  std::vector<std::thread> file_threads;
  std::string thread_base_name{"file"};

public:
  ToFilePrint(std::size_t threads_count = 1) : Observer() { start(threads_count); }
  void update(BulkStorage &, std::size_t) override;

  static std::shared_ptr<ToFilePrint> create(std::size_t threads_count = 1)
  {
    return std::make_shared<ToFilePrint>(threads_count);
  }

  static std::shared_ptr<ToFilePrint> create(const std::weak_ptr<Observable> &_observable, std::size_t threads_count = 1)
  {
    std::shared_ptr _tmpToFilePrint = std::make_shared<ToFilePrint>(threads_count);
    auto tmpObservable = _observable.lock();
    if (tmpObservable)
    {
      _tmpToFilePrint->subscribe_on_observable(tmpObservable);
      tmpObservable.reset();
    }
    return _tmpToFilePrint;
  }

  void start(std::size_t threads_count = 1)
  {
    if (!threads_count)
      threads_count = 1;
    for (auto i = 0; i < threads_count; ++i)
    {
      std::thread createdThread = std::thread(&ToFilePrint::printOut, this);
      MetricsCount::Instance().regThread(createdThread.get_id(), thread_base_name + std::to_string(i + 1));
      file_threads.emplace_back(std::move(createdThread));
    }

#ifdef DEBUG
    std::cout << "File_log thread created" << file_threads.size() << std::endl;
#endif
  }

  void stop()
  {
    finished = true;
    cv_queue.notify_all();
    if (file_threads.empty())
    {
      return;
    }
    for (auto &thread : file_threads)
      if (thread.joinable())
        thread.join();
#ifdef DEBUG
    std::cout << "File_log threads stopped" << file_threads.size() << std::endl;
#endif
    file_threads.clear();
  }
  void printOut();

  virtual ~ToFilePrint()
  {
    stop();
  }
};
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
#include "bulk.h"

class Observable;
class Observer : public std::enable_shared_from_this<Observer>
{
private:
  std::vector<std::weak_ptr<Observable>> _observables;

public:
  virtual void printOut(BulkStorage &source, std::size_t) = 0;
  void subscribe_on_observable(const std::weak_ptr<Observable> &);
  void unsubscribe_on_observable(const std::weak_ptr<Observable> &);
  virtual ~Observer(){};

protected:
  virtual void printOstream(std::ostream &out, BulkStorage &source, std::size_t id)
  {
    if (source.get_commands(id).size())
    {
      out << "bulk: ";
      for (const auto &cmd : source.get_commands(id))
        out << cmd << (&cmd != &source.get_commands(id).back() ? ", " : "");
      out << std::endl;
    }
  }
};

class ToConsolePrint : public Observer
{
private:
  std::condition_variable cv_queue;
  std::mutex console_mutex;

public:
  ToConsolePrint(std::ostream &out) : Observer(), _out{out} {}
  void printOut(BulkStorage &, std::size_t) override;
  static std::shared_ptr<ToConsolePrint> create(std::ostream &out, const std::weak_ptr<Observable> &_obs)
  {
    std::shared_ptr _tmpToConsolePrint = std::make_shared<ToConsolePrint>(out);
    auto tmpObservable = _obs.lock();
    if (tmpObservable)
    {
      _tmpToConsolePrint->subscribe_on_observable(tmpObservable);
      tmpObservable.reset();
    }
    return _tmpToConsolePrint;
  }
  static std::shared_ptr<ToConsolePrint> create(std::ostream &out)
  {
    return std::make_shared<ToConsolePrint>(out);
  }

private:
  std::ostream &_out;
};

class ToFilePrint : public Observer
{
private:
  std::condition_variable cv_queue;
  std::mutex file_mutex;

public:
  ToFilePrint() : Observer() {}
  void printOut(BulkStorage &, std::size_t) override;

  static std::shared_ptr<ToFilePrint> create()
  {
    return std::make_shared<ToFilePrint>();
  }

  static std::shared_ptr<ToFilePrint> create(const std::weak_ptr<Observable> &_observable)
  {
    std::shared_ptr _tmpToFilePrint = std::make_shared<ToFilePrint>();
    auto tmpObservable = _observable.lock();
    if (tmpObservable)
    {
      _tmpToFilePrint->subscribe_on_observable(tmpObservable);
      tmpObservable.reset();
    }
    return _tmpToFilePrint;
  }
};
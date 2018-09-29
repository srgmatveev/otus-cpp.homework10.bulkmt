#include <iostream>
#include <memory>
#include <cstddef>
#include <fstream>
#include "bulk_storage.h"
#include "bulk.h"
#include "bulk_observer.h"
#include "metrics.h"

void ToConsolePrint::update(BulkStorage &source, std::size_t id)
{
    std::unique_lock<std::mutex> lock{console_mutex};
    data_queue.emplace(std::pair{source.get_timestamp(id), source.get_commands(id)});
    cv_queue.notify_one();
}

void ToConsolePrint::printOut()
{
    while (!finished || !data_queue.empty())
    {
        std::unique_lock<std::mutex> locker{console_mutex};

        while (data_queue.empty() && !finished)
            cv_queue.wait(locker);
        if (!data_queue.empty())
        {
            auto cmd_pair = data_queue.front();
            data_queue.pop();
            MetricsCount::Instance().blocksIncr(std::this_thread::get_id());
            MetricsCount::Instance().cmdsIncr(std::this_thread::get_id(), cmd_pair.second.size());
            printOstream(_out, cmd_pair.second);
        }
    }
}
void ToFilePrint::update(BulkStorage &source, std::size_t id)
{
    std::lock_guard<std::mutex> lock_fs(file_mutex);
    data_queue.emplace(std::pair{source.get_timestamp(id), source.get_commands(id)});
    cv_queue.notify_one();
}

void ToFilePrint::printOut()
{
    while (!finished || !data_queue.empty())
    {
        std::unique_lock<std::mutex> lock_fs{file_mutex};
        while (data_queue.empty() && !finished)
            cv_queue.wait(lock_fs);
        if (!data_queue.empty())
        {
            auto cmd_pair = data_queue.front();
            data_queue.pop();
            std::ostringstream oss;
            oss << "bulk";
            oss << cmd_pair.first;
            oss << ".log";
            std::string fName = oss.str();
            std::ofstream ofs;
            ofs.exceptions(std::ofstream::failbit | std::ofstream::badbit);
            try
            {
                ofs.open(fName, std::ofstream::out | std::ofstream::trunc);
                lock_fs.unlock();
                MetricsCount::Instance().blocksIncr(std::this_thread::get_id());
                MetricsCount::Instance().cmdsIncr(std::this_thread::get_id(), cmd_pair.second.size());
                printOstream(ofs, cmd_pair.second);
                ofs.close();
            }
            catch (std::ofstream::failure e)
            {
                lock_fs.unlock();
                std::cerr << "Exception opening/reading/closing file: " << fName << std::endl;
            }
        }
    }
}

void Observer::subscribe_on_observable(const std::weak_ptr<Observable> &observable)
{
    auto item = observable.lock();
    if (item)
    {
        auto it = std::find_if(_observables.cbegin(), _observables.cend(), [&](std::weak_ptr<Observable> e) { return e.lock() == item; });
        if (it == _observables.cend())
        {
            _observables.emplace_back(item);
            item->subscribe(this->shared_from_this());
        }
        item.reset();
    }
}

void Observer::unsubscribe_on_observable(const std::weak_ptr<Observable> &observable_ptr)
{
    auto observable = observable_ptr.lock();
    if (observable)
    {
        _observables.erase(
            std::remove_if(_observables.begin(), _observables.end(),
                           [observable_ptr](const auto &p) { return !(observable_ptr.owner_before(p) || p.owner_before(observable_ptr)); }),
            _observables.end());

        observable.reset();
    }
}

#include <iostream>
#include <memory>
#include <cstddef>
#include <fstream>
#include "bulk_storage.h"
#include "bulk.h"
#include "bulk_observer.h"

void ToConsolePrint::printOut(BulkStorage &source, std::size_t id)
{
    std::unique_lock<std::mutex> lock_console(console_mutex);
    printOstream(_out, source, id);
}


void ToFilePrint::printOut(BulkStorage &source, std::size_t id)
{
    std::unique_lock<std::mutex> lock_fs(file_mutex);
    std::ostringstream oss;
    oss << "bulk";
    oss << source.get_timestamp(id);
    oss << ".log";
    std::string fName = oss.str();

    std::ofstream ofs;
    ofs.exceptions(std::ofstream::failbit | std::ofstream::badbit);
    try
    {
        ofs.open(fName, std::ofstream::out | std::ofstream::trunc);
        lock_fs.unlock();
        printOstream(ofs, source, id);
        ofs.close();
    }
    catch (std::ofstream::failure e)
    {
        lock_fs.unlock();
        std::cerr << "Exception opening/reading/closing file: " << fName << std::endl;
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

#pragma once
#include <string>
#include <cstddef>

struct Metric
{
    std::size_t thread_id = 0;
    std::size_t str_cnt = 0;
    std::size_t blk_cnt = 0;
    std::size_t cmd_cnt = 0;
    std::string thread_name{};
};
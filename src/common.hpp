#pragma once

// std
#include <string>
#include <vector>

std::string get_timestamp_filename();

std::string get_human_readable_timestamp(const std::string &filename);

std::vector<std::string> get_file_list(const std::string &directory);

std::vector<std::string>
get_human_readable_file_list(const std::string &directory);

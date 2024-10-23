#pragma once

// std
#include <filesystem>
#include <string>
#include <vector>

std::string get_timestamp_filename();

std::string get_human_readable_timestamp(const std::string &filename);

std::vector<std::filesystem::path>
get_file_list(const std::filesystem::path &directory);

std::vector<std::string>
get_human_readable_file_list(const std::filesystem::path &directory);

void create_dir(const std::filesystem::path &directory);

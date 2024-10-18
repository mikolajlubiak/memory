#pragma once

// std
#include <chrono>
#include <ftxui/dom/elements.hpp>
#include <string>

void change_blink(bool *const pBlink, std::chrono::milliseconds *const pTimer,
                  const bool blink_or_not,
                  std::chrono::milliseconds &timerDuration);

std::string get_timestamp_filename();

std::string get_human_readable_timestamp(const std::string &filename);

std::vector<std::string> get_file_list(const std::string &directory);
std::vector<std::string>
get_human_readable_file_list(const std::string &directory);

ftxui::Element file_list_to_element_list(std::vector<std::string>);

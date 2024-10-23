// header
#include "common.hpp"

// std
#include <ctime>

std::string get_timestamp_filename() {
  auto now = std::chrono::system_clock::now();
  std::time_t now_c = std::chrono::system_clock::to_time_t(now);
  std::string timestamp = std::to_string(now_c);
  return "saves/" + timestamp + ".dat";
}

std::string get_human_readable_timestamp(const std::string &filename) {
  std::time_t timestamp =
      std::stol(filename.substr(0, filename.find_first_of('.')));
  std::tm *local_time = std::localtime(&timestamp);
  char buffer[64];
  std::strftime(buffer, sizeof(buffer), "%d-%m-%Y %H:%M:%S", local_time);
  return buffer;
}

std::vector<std::filesystem::path> get_file_list(const std::filesystem::path &directory) {
  std::vector<std::filesystem::path> file_list{};

  if (!std::filesystem::exists(directory)) {
      return file_list;
  }

  for (const auto &entry : std::filesystem::directory_iterator(directory)) {
    if (std::filesystem::is_regular_file(entry)) {
      file_list.push_back(entry.path());
    }
  }
  return file_list;
}

std::vector<std::string>
get_human_readable_file_list(const std::filesystem::path &directory) {
  std::vector<std::string> file_list{};

  for (const auto &entry : get_file_list(directory)) {
    file_list.push_back(
        get_human_readable_timestamp(entry.filename().string()));
  }
  return file_list;
}

void create_dir(const std::filesystem::path &directory) {
    // Check if the directory exists
    if (!std::filesystem::exists(directory)) {
        // Create the directory
        (std::filesystem::create_directory(directory));
    }
}
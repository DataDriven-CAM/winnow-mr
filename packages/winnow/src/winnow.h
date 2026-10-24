#pragma once

#include <iostream>
#include <filesystem>
#include <thread>
#include <chrono>
#include <filesystem>

using namespace std::chrono_literals;

void watch_file(std::stop_token stop_token, std::filesystem::path target_file) {
    bool file_existed_last_check = false;
    std::filesystem::file_time_type last_known_time;

    std::cout << "[Watcher] Thread started. Monitoring: " << target_file << "\n";

    // Loop runs until the main program requests a stop
    while (!stop_token.stop_requested()) {
        if (std::filesystem::exists(target_file)) {
            try {
                // Get the current modification timestamp
                std::filesystem::file_time_type current_time = std::filesystem::last_write_time(target_file);

                if (!file_existed_last_check) {
                    std::cout << "\n[Watcher] File detected! (Newly created or found)\n>> " << std::flush;
                    file_existed_last_check = true;
                    last_known_time = current_time;
                    
                    // TODO: Trigger your file-load logic here
                } 
                else if (current_time != last_known_time) {
                    std::cout << "\n[Watcher] File modification or touch detected!\n>> " << std::flush;
                    last_known_time = current_time;

                    // TODO: Trigger your file-reload/recompile logic here
                }
            } catch (const std::filesystem::filesystem_error& e) {
                // Catches rare edge cases where a file is touched but temporarily locked/unreadable
                std::cerr << "\n[Watcher] Error reading file metrics: " << e.what() << "\n>> " << std::flush;
            }
        } else {
            if (file_existed_last_check) {
                std::cout << "\n[Watcher] File was deleted or moved.\n>> " << std::flush;
                file_existed_last_check = false;
            }
        }

        // Poll every 1 second (adjust interval as needed)
        std::this_thread::sleep_for(1s);
    }
    std::cout << "[Watcher] Thread cleanly exiting.\n";
}

void print_exception(const std::exception& e, int level =  0)
{
    std::cerr << std::string(level, ' ') << "exception: " << e.what() << '\n';
    try {
        std::rethrow_if_nested(e);
    } catch(const std::exception& e) {
        print_exception(e, level+1);
    } catch(...) {}
}

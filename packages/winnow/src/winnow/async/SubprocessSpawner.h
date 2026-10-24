#pragma once
#include <string>
#include <vector>
#include <iostream>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#endif

namespace sylvanmats::winnow::async {
    class SubprocessSpawner {
        public:
        SubprocessSpawner() = default;
        SubprocessSpawner(const SubprocessSpawner& orig) =  delete;
        virtual ~SubprocessSpawner() = default;
        SubprocessSpawner& operator=(const SubprocessSpawner& other) = delete;

        // A fast, zero-dependency, platform-native process spawner
        int operator()(const std::string& binary, const std::vector<std::string>& args) {
        #if defined(_WIN32)
            // 1. Rebuild a safe, clean Windows command line string
            std::string command_line = "\"" + binary + "\"";
            for (const auto& arg : args) {
                command_line += " " + arg; 
            }

            STARTUPINFOA si = {};
            si.cb = sizeof(si);
            // Explicitly hide child process windows to keep execution silent
            si.dwFlags = STARTF_USESHOWWINDOW;
            si.wShowWindow = SW_HIDE; 
            
            std::cout << "Executing: " << command_line << std::endl;
            PROCESS_INFORMATION pi = {};

            // Launch binary natively without invoking a raw cmd.exe middleman
            if (!CreateProcessA(
                    nullptr, 
                    command_line.data(), 
                    nullptr, nullptr, FALSE, 
                    CREATE_NO_WINDOW, // Guarantees background isolation
                    nullptr, nullptr, &si, &pi)) {
                return -1;
            }

            // Wait until download process finishes
            WaitForSingleObject(pi.hProcess, INFINITE);
            
            DWORD exit_code = 0;
            GetExitCodeProcess(pi.hProcess, &exit_code);

            // Clean up handle state to prevent resource bloat
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            return static_cast<int>(exit_code);

        #else
            // 2. POSIX Execution Loop (Linux/macOS)
            pid_t pid = fork();
            if (pid == 0) {
                int dev_null_fd = open("/dev/null", O_WRONLY);
                if (dev_null_fd >= 0) {
                    dup2(dev_null_fd, STDOUT_FILENO); // Overwrite stdout (1) -> black hole
                    dup2(dev_null_fd, STDERR_FILENO); // Overwrite stderr (2) -> black hole
                    close(dev_null_fd);               // Clean up our temporary descriptor copy
                }
                // Child Process: Build raw char* array matching execvp boundaries
                std::vector<char*> c_args;
                c_args.push_back(const_cast<char*>(binary.c_str()));
                for (const auto& arg : args) {
                    c_args.push_back(const_cast<char*>(arg.c_str()));
                }
                c_args.push_back(nullptr);

                std::vector<std::string> env_strings = {
                    "PATH=/usr/bin:/bin",
                    "HOME=" + std::string(getenv("HOME") ? getenv("HOME") : "")
                };
                
                std::vector<char*> c_env;
                for (auto& env_str : env_strings) {
                    c_env.push_back(const_cast<char*>(env_str.c_str()));
                }
                c_env.push_back(nullptr);

                // Execute natively, searching the host system PATH environment automatically
                execvpe(binary.c_str(), c_args.data(), c_env.data());
                
                // If execvp fails, exit child worker thread safely
                _exit(127); 
            } else if (pid > 0) {
                // Parent Process: Block cleanly until child finishes
                int status = 0;
                waitpid(pid, &status, 0);
                if (WIFEXITED(status)) {
                    return WEXITSTATUS(status);
                }
            }
            return -1;
        #endif
        }
    };

}
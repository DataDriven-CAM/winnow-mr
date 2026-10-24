#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <filesystem>
#include <memory>
#include <execution>

#include <stdexec/execution.hpp>
#include <exec/static_thread_pool.hpp>
#include <exec/start_detached.hpp>
#include <exec/async_scope.hpp> // Required to give the spawned task a tracking scope

#include "winnow/async/SubprocessSpawner.h"

namespace sylvanmats::winnow::schedules{

    struct model_data{
        std::filesystem::path model_path;
        std::string organization;
        std::string repository_name;
    };

    class Models{
        private:
        std::string home;
        std::string rootModelDirectory;
        std::vector<model_data>& modelPaths;
        std::string modelDirectory;
        exec::async_scope scope;

        public:
            Models()= delete;
            Models(std::string home, std::string rootModelDirectory,  std::vector<model_data>& modelPaths) : home(home), rootModelDirectory(rootModelDirectory), modelPaths(modelPaths){
                size_t offset=rootModelDirectory.find_last_of("/\\");
                modelDirectory=(offset!=std::string::npos) ? rootModelDirectory.substr(offset+1, rootModelDirectory.length()-offset-1) : rootModelDirectory;
            }
            Models(Models const&)= delete;
            Models(Models&&)= default;
            virtual ~Models()= default;
            Models& operator=(Models const&)= delete;
            Models& operator=(Models&&)= default;

            void operator()(std::shared_ptr<std::vector<size_t>>& missingIndices){
                std::cout<<"Models "<<missingIndices->size()<<std::endl;
            exec::static_thread_pool pool(std::thread::hardware_concurrency());
            auto scheduler = pool.get_scheduler();
                auto work=stdexec::schedule(scheduler) 
                        | stdexec::bulk(stdexec::par, missingIndices->size(), [&](std::size_t bulk_idx) noexcept {
                        auto& missingIndex=(*missingIndices)[bulk_idx];
                        std::vector<std::string> args;
                    args.push_back("-L");
                    args.push_back("-#");
                    args.push_back("-C");
                    args.push_back("-");
                    args.push_back("-o");
                    args.push_back(modelPaths[missingIndex].model_path.native_encoded_string());
                    args.push_back("https://huggingface.co/"+modelPaths[missingIndex].organization+"/"+modelPaths[missingIndex].repository_name+"/resolve/main/"+modelPaths[missingIndex].model_path.filename().native_encoded_string());

                    std::cout<<"args "<<args.size()<<std::endl;
                    for(auto& arg : args){
                        std::cout<<"\t"<<arg<<std::endl;
                    }
                    std::cout<<std::endl;
                // Launch non-blocking process hook instead of blocking std::system
                sylvanmats::winnow::async::SubprocessSpawner spawner;
                int exit_code = spawner("curl", args);
                if (exit_code == 0) {
                    std::cout << '.'; 
                    std::cout.flush(); // Only flush occasionally when we want the dot visible!
                }
                else{
                    std::cout << 'x'<<exit_code;
                    std::cout.flush(); // Only flush occasionally when we want the dot visible!
                }

                 return exit_code;
            })
                | stdexec::upon_error([](std::exception_ptr ep) noexcept {
                // ⚠️ Handle or log the scheduling/execution error here safely
                // This ensures the error channel is consumed and returns void.
            });

            scope.spawn(std::move(work));

            };
    };
}

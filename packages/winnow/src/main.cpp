#include "CLI/App.hpp"
#include "CLI/Formatter.hpp"
#include "CLI/Config.hpp"

#include <stdlib.h>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <thread>
#include <chrono>

#include "winnow.h"
#include "winnow/schedules/Models.h"

int main(int argc, char** argv, char **envp) {
    try{
        std::string home=(std::getenv("HOME")!=nullptr) ? std::getenv("HOME") : "c:/Users/Roger";
        std::string winnowHome=(std::getenv("WINNOW_HOME")!=nullptr) ?std::getenv("WINNOW_HOME") : ".";
        CLI::App app{"Winnow Makefile Agent"};

        std::string makefilePath{"Makefile"};
        app.add_option("input_file", makefilePath, "Path to the Makefile file")
            ->capture_default_str()      // Optional
            ->check(CLI::ExistingFile);  // Validates the file exists before running your code

        app.set_config("--config", winnowHome+"/config.toml", "Read a toml file");
        app.add_option("module-directory")->group("");
        app.add_option("model-directory")->group("");

        CLI11_PARSE(app, argc, argv);

        std::cout<<"home "<<home<<std::endl;
        std::string moduleDirectory=(app.count("module-directory"))? app.get_option("module-directory")->as<std::string>(): "cpp_modules";
        std::string modelDirectory=(app.count("model-directory"))? app.get_option("model-directory")->as<std::string>(): "models";
        // std::filesystem::path modulePath=std::filesystem::current_path()/moduleDirectory;
        // if(!std::filesystem::exists(modulePath))std::filesystem::create_directories(modulePath);
        std::filesystem::path modelPath=std::filesystem::path(home)/".winnow"/modelDirectory;
        if(!std::filesystem::exists(modelPath))std::filesystem::create_directories(modelPath);
        std::vector<sylvanmats::winnow::schedules::model_data> modelPaths{{.model_path=modelPath/"qwen2.5-coder-7b-instruct-q4_k_m.gguf", .organization="Qwen", .repository_name="Qwen2.5-Coder-7B-Instruct-GGUF"},
                                                                          {.model_path=modelPath/"qwen2.5-coder-1.5b-instruct-q8_0.gguf", .organization="Qwen", .repository_name="Qwen2.5-Coder-1.5B-Instruct-GGUF"}};
        std::shared_ptr<std::vector<size_t>> missingIndices(new std::vector<size_t>());
        for(size_t i=0; i<modelPaths.size(); i++){
            std::cout<<"modelPath "<<modelPaths[i].model_path<<std::endl;
            std::filesystem::path modelPath=modelPaths[i].model_path;
            if(!std::filesystem::exists(modelPaths[i].model_path))missingIndices->push_back(i);
        }
        sylvanmats::winnow::schedules::Models models(home, modelDirectory, modelPaths);
        models(missingIndices);

        // Spin up the background thread. Pass the stop_token implicitly, followed by parameters.
        std::jthread watcher_thread(watch_file, makefilePath);

        // Main Interactive Shell / Application Loop
        std::string command;
        while (true) {
            std::cout << ">> ";
            if (!std::getline(std::cin, command) || command == "quit" || command == "exit") {
                break;
            }
            std::cout << "Executing main thread command: " << command << "\n";
        }

        std::cout << "Shutting down... (jthread will join automatically)\n";


    }
    catch(std::filesystem::filesystem_error &e) {
        std::cout<<"what? "<<e.what() << std::endl;
        return  EXIT_FAILURE;
    }
    catch(const std::exception& e) {
        print_exception(e);
        return  EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

#define DOCTEST_CONFIG_TREAT_CHAR_STAR_AS_STRING
#include <doctest/doctest.h>

#include <cstdio>
#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <string_view>
#include <array>
#include <vector>
#include <algorithm>
#include <charconv>
#include <tuple>
#include <meta>
#include <future>
#include <execution>
#include <thread>
#include <utility>
#include <filesystem>
#include <unistd.h>
#include <spawn.h>
#include <sys/wait.h>
#include <atomic>

#include <stdexec/execution.hpp>

#include <zlib.h>

#include "llama.h"
#include "common.h"

#define protected public
#include "io/json/Binder.h"
#include "metaphrase/GBackusNaurFormation.h"

#define private public

extern char** environ;

void target_test_environment() {
    std::filesystem::path root = std::filesystem::current_path();
    
    // Resolve clean absolute paths
    std::filesystem::path bin_dir = root / "../cpp_modules/llama/dist/bin";
    std::filesystem::path lib_dir = root / "../cpp_modules/llama/dist/lib";

    // Append to PATH
    const char* old_path = std::getenv("PATH");
    std::string new_path = bin_dir.native_encoded_string() + (old_path ? ":" + std::string(old_path) : "");
    setenv("PATH", new_path.c_str(), 1);

    // Append to LD_LIBRARY_PATH
    const char* old_ld = std::getenv("LD_LIBRARY_PATH");
    std::string new_ld = lib_dir.native_encoded_string() + (old_ld ? ":" + std::string(old_ld) : "");
    setenv("LD_LIBRARY_PATH", new_ld.c_str(), 1);
}

bool validate_gbnf(std::string_view gbnfView, std::string_view jsonContent) {
    // Fix #2: Atomic index creates thread-safe unique filenames to eliminate test collisions
    static std::atomic<size_t> test_counter{0};
    size_t current_id = test_counter.fetch_add(1, std::memory_order_relaxed);

    std::filesystem::path tmpDir = std::filesystem::temp_directory_path();
    std::string grammarPath = (tmpDir / ("schema_test_" + std::to_string(current_id) + ".gbnf")).native_encoded_string();
    std::string jsonPath = (tmpDir / ("schema_test_" + std::to_string(current_id) + ".json")).native_encoded_string();

    // Write contents cleanly
    {
        std::ofstream g_file(grammarPath);
        g_file << gbnfView;
    }
    {
        std::ofstream j_file(jsonPath);
        j_file << jsonContent;
    }

    // Fix #1: Lock structural lifetime explicitly down to c_str arrays
    std::array<const char*, 4> args = {
        "test-gbnf-validator", 
        grammarPath.c_str(), 
        jsonPath.c_str(), 
        nullptr
    };

    pid_t pid;
    // Fix #3: Pass environ cleanly after your target_test_environment() modification
    int spawn_result = posix_spawnp(&pid, args[0], nullptr, nullptr, 
                                    const_cast<char* const*>(args.data()), environ);
    
    if (spawn_result != 0) {
        std::filesystem::remove(grammarPath);
        std::filesystem::remove(jsonPath);
        return false;
    }

    int status;
    waitpid(pid, &status, 0);

    // Wipe temp file artifacts from disk immediately
    std::filesystem::remove(grammarPath);
    std::filesystem::remove(jsonPath);

    // Evaluate exit status cleanly
    bool success = WIFEXITED(status) && WEXITSTATUS(status) == 0;
    return success;
}

struct LlamaPipeline {
    llama_model* model = nullptr;
    llama_context* ctx  = nullptr;
};


LlamaPipeline initialize_llama_native(const std::string& model_path, int context_window = 2048) {
    LlamaPipeline pipeline;

    // 1. Initialize backend hardware drivers globally (CPU/CUDA/Metal backends)
    // In modern llama.cpp versions, this handles setup automatically
    ggml_backend_load_all(); 

    // 2. Configure Model Parameters
    llama_model_params mparams = llama_model_default_params();
    
    // Performance tuning: Specify how many layers to offload to your GPU. 
    // Set to 0 for pure CPU, or high (e.g., 32 or -1) to push processing onto VRAM
    mparams.n_gpu_layers = 0; 

    // 3. Load the raw model file from the disk storage
    pipeline.model = llama_model_load_from_file(model_path.c_str(), mparams);
    if (pipeline.model == nullptr) {
        std::cerr << "[ERROR] Critical: Failed to load model file path: " << model_path << "\n";
        return pipeline;
    }

    // 4. Configure Context Execution Environment Parameters
    llama_context_params cparams = llama_context_default_params();
    cparams.n_ctx   = context_window; // Set context window constraints (e.g., 2048 or 4096 tokens)
    cparams.n_batch = 512;            // Maximum token grouping processing bandwidth

    // 5. Instantiate the computation graph context engine 
    pipeline.ctx = llama_init_from_model(pipeline.model, cparams);
    if (pipeline.ctx == nullptr) {
        std::cerr << "[ERROR] Critical: Failed to instantiate context compute space.\n";
        // Cleanup model pointer memory if context generation aborts
        llama_model_free(pipeline.model);
        pipeline.model = nullptr;
        return pipeline;
    }

    std::cout << "[SUCCESS] Native Llama pipeline instances generated smoothly.\n";
    return pipeline;
}

// Memory tracking cleanup function
void destroy_llama_pipeline(LlamaPipeline& pipeline) {
    if (pipeline.ctx) {
        llama_free(pipeline.ctx);
        pipeline.ctx = nullptr;
    }
    if (pipeline.model) {
        llama_model_free(pipeline.model);
        pipeline.model = nullptr;
    }
}

TEST_SUITE("llm"){

    TEST_CASE("test graphRAG of project relationships"){
        std::string model_file_path = "/home/roger/.cnpm/cpp_modules/llama/models/Llama-3.1-8B-Instruct-Q4_K_M.gguf"; 
        
        // 1. Spin up the engine
        LlamaPipeline pipeline = initialize_llama_native(model_file_path, 2048);
        
        // 3. Terminate instances safely upon exit
        destroy_llama_pipeline(pipeline);

}

    TEST_CASE("test metaphrasing csl-data.json schema to gbnf"){
        try{
    constexpr char8_t jsonBuffer[] ={
#embed "csl-data.json"
    };
            sylvanmats::metaphrase::GBackusNaurFormation<sizeof(jsonBuffer)> gBackusNaurFormation{};
            static constexpr auto gbnf=gBackusNaurFormation(std::u8string_view(jsonBuffer, sizeof(jsonBuffer)));
            constexpr std::string_view gbnfView=gbnf.view();
            std::cout << gbnfView<<std::endl;
            CHECK_NE(gbnfView.find("author"), std::string_view::npos);
            CHECK_NE(gbnfView.find("title"), std::string_view::npos);
            target_test_environment();
            CHECK(validate_gbnf(gbnfView, R"([
  {
    "id": "doe2024optics",
    "type": "article-journal",
    "title": "Advanced Metaphrasing in C++26",
    "container-title": "Journal of Compile-Time Engineering",
    "volume": "42",
    "issue": "7",
    "page": "101-115",
    "DOI": "10.1000/jcte.2024.42.7.101",
    "author": [
      {
        "family": "Doe",
        "given": "Jane",
        "suffix": "Jr."
      },
      {
        "family": "Smith",
        "given": "John"
      }
    ],
    "issued": {
      "date-parts": [
        [2024, 10, 14]
      ]
    }
  },
  {
    "id": "knuth1997art",
    "type": "book",
    "title": "The Art of Computer Programming",
    "publisher": "Addison-Wesley",
    "publisher-place": "Reading, Massachusetts",
    "volume": "1",
    "edition": "3",
    "author": [
      {
        "family": "Knuth",
        "given": "Donald E."
      }
    ],
    "issued": {
      "date-parts": [
        [1997]
      ]
    }
  }
]
)"));
        }
        catch(std::out_of_range& e){
            std::cout << "out of range "<<e.what()<<std::endl;
        }
        catch(std::exception& e){
            std::cout << "exception "<<e.what()<<std::endl;
        }
    }

    TEST_CASE("test metaphrasing package.json schema to gbnf"){
      try{
    constexpr char8_t jsonBuffer[] ={
#embed "package.json"
    };
    // sylvanmats::metaphrase::GBackusNaurFormation<sizeof(jsonBuffer)> gBackusNaurFormation{};
    // static constexpr auto gbnf=gBackusNaurFormation(std::u8string_view(jsonBuffer, sizeof(jsonBuffer)));
    // constexpr std::string_view gbnfView=gbnf.view();
    // std::cout << gbnfView<<std::endl;
    // CHECK_EQ(gbnfView.size(), 125);
    // CHECK_NE(gbnfView.find("name"), std::string_view::npos);
    // CHECK_NE(gbnfView.find("version"), std::string_view::npos);
    // CHECK_NE(gbnfView.find("description"), std::string_view::npos);
    // CHECK_NE(gbnfView.find("dependencies"), std::string_view::npos);
    // CHECK_NE(gbnfView.find("devDependencies"), std::string_view::npos);
    // CHECK_NE(gbnfView.find("repository"), std::string_view::npos);
    // CHECK_NE(gbnfView.find("license"), std::string_view::npos);
    // CHECK_NE(gbnfView.find("scripts"), std::string_view::npos);
    // CHECK_NE(gbnfView.find("directory"), std::string_view::npos);
    // CHECK_NE(gbnfView.find("author"), std::string_view::npos);
    // CHECK_NE(gbnfView.find("main"), std::string_view::npos);

  }
  catch(std::out_of_range& e){
    std::cout << "out of range "<<e.what()<<std::endl;
  }
  catch(std::exception& e){
    std::cout << "exception "<<e.what()<<std::endl;
  }
    }

    TEST_CASE("test similarity"){

        // std::ifstream is("/home/roger/Documents/Crystallography/1506.01455.pdf", std::ios::binary | std::ios::ate);
        // std::vector<uint8_t> jgf2SchemaContent((std::istreambuf_iterator<uint8_t>(is)), std::istreambuf_iterator<uint8_t>());

    }

}
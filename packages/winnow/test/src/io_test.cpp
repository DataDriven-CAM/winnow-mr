#define DOCTEST_CONFIG_TREAT_CHAR_STAR_AS_STRING
#include <doctest/doctest.h>

#include <cstdio>
#include <iostream>
#include <sstream>
#include <fstream>
#include <filesystem>
#include <string>
#include <tuple>
#include <meta>
#include <future>
#include <execution>
#include <thread>
#include <utility>

#include <stdexec/execution.hpp>

#include "llama.h"
#include "json-schema-to-grammar.h"

#define protected public
#include "io/json/Binder.h"

#define private public

TEST_SUITE("io"){

TEST_CASE("test jgf 2.0 to gbnf grammar"){
    std::ifstream is("../../../cpp_modules/json-graph-specification/json-graph-schema_v2.json");
    std::string jgf2SchemaContent((std::istreambuf_iterator<char>(is)), std::istreambuf_iterator<char>());
    std::filesystem::path currentDirectory=std::filesystem::current_path();
    std::cout<<"currentDirectory "<<currentDirectory<<std::endl;
    CHECK_EQ(jgf2SchemaContent.size(), 4167);
    size_t root_bracket_pos = jgf2SchemaContent.find('{');
    if (root_bracket_pos != std::string::npos) {
        // 2. Inject the mandatory "type": "object" constraint right into the header space
        std::string structural_patch = "\n  \"type\": \"object\",";
        jgf2SchemaContent.insert(root_bracket_pos + 1, structural_patch);
    }
    size_t pos = 0;
    while ((pos = jgf2SchemaContent.find("[\"object\"]", pos)) != std::string::npos) {
        jgf2SchemaContent.replace(pos, 10, "\"object\"");
        pos += 8;
    }
    
    pos = 0;
    while ((pos = jgf2SchemaContent.find("[ \"object\" ]", pos)) != std::string::npos) {
        jgf2SchemaContent.replace(pos, 12, "\"object\"");
        pos += 8;
    }

    // 2. Also guarantee the root object rule exists if the original text lacks it
    if (jgf2SchemaContent.find("\"type\"") == std::string::npos || jgf2SchemaContent.find("\"type\"") > 100) {
        size_t root_bracket = jgf2SchemaContent.find('{');
        if (root_bracket != std::string::npos) {
            jgf2SchemaContent.insert(root_bracket + 1, "\n  \"type\": \"object\",");
        }
    }
        if (jgf2SchemaContent.size() > 256) {
        std::cout << "\n--- SCHEMA HEADER INSPECTION ---\n";
        std::cout << jgf2SchemaContent.substr(0, 128) << "\n";
        std::cout << "...\n";
        std::cout << jgf2SchemaContent.substr(jgf2SchemaContent.size() - 128) << "\n";
        std::cout << "--------------------------------\n";
    }
    std::string gbnf_grammar_output = json_schema_to_grammar(jgf2SchemaContent);
    CHECK_EQ(gbnf_grammar_output.size(), 0);
    CHECK(gbnf_grammar_output.find("graph") != std::string::npos);
    CHECK(gbnf_grammar_output.find("node") != std::string::npos);
    CHECK(gbnf_grammar_output.find("edge") != std::string::npos);

    CHECK(gbnf_grammar_output.find("boolean") != std::string::npos);
}

}

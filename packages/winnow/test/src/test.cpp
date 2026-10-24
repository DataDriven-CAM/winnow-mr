#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#define DOCTEST_CONFIG_TREAT_CHAR_STAR_AS_STRING
#include <doctest/doctest.h>

#include <cstdio>
#include <iostream>
#include <sstream>
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

TEST_SUITE("test"){

TEST_CASE("test some stuff"){
}

}

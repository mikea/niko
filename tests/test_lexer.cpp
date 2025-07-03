#include "inter.h"

// Undefine CHECK to avoid conflict with Catch2
#ifdef CHECK
#undef CHECK
#endif

#include <catch2/catch_test_macros.hpp>

TEST_CASE("Lexer tokenizes numbers correctly", "[lexer]") {
    SECTION("Integer tokens") {
        const char* input = "42";
        const char* pos = input;
        
        token_t token = next_token(&pos);
        
        REQUIRE(token.tok == TOK_I64);
        REQUIRE(token.val.i == 42);
        REQUIRE(token.text.size() == 2);
        REQUIRE(string(token.text) == "42");
    }
    
    SECTION("Negative integer tokens") {
        const char* input = "-123";
        const char* pos = input;
        
        token_t token = next_token(&pos);
        
        REQUIRE(token.tok == TOK_I64);
        REQUIRE(token.val.i == -123);
        REQUIRE(string(token.text) == "-123");
    }
    
    SECTION("Float tokens") {
        const char* input = "3.14";
        const char* pos = input;
        
        token_t token = next_token(&pos);
        
        REQUIRE(token.tok == TOK_F64);
        REQUIRE(token.val.d == 3.14);
        REQUIRE(string(token.text) == "3.14");
    }
    
    SECTION("Scientific notation") {
        const char* input = "1.5e3";
        const char* pos = input;
        
        token_t token = next_token(&pos);
        
        REQUIRE(token.tok == TOK_F64);
        REQUIRE(token.val.d == 1500.0);
        REQUIRE(string(token.text) == "1.5e3");
    }
}

TEST_CASE("Lexer tokenizes words correctly", "[lexer]") {
    SECTION("Simple word") {
        const char* input = "hello";
        const char* pos = input;
        
        token_t token = next_token(&pos);
        
        REQUIRE(token.tok == TOK_WORD);
        REQUIRE(string(token.text) == "hello");
    }
    
    SECTION("Word with special characters") {
        const char* input = "add+";
        const char* pos = input;
        
        token_t token = next_token(&pos);
        
        REQUIRE(token.tok == TOK_WORD);
        REQUIRE(string(token.text) == "add+");
    }
}

TEST_CASE("Lexer tokenizes strings correctly", "[lexer]") {
    SECTION("Simple string") {
        const char* input = "\"hello world\"";
        const char* pos = input;
        
        token_t token = next_token(&pos);
        
        REQUIRE(token.tok == TOK_STR);
        REQUIRE(string(token.val.s) == "hello world");
    }
    
    SECTION("Empty string") {
        const char* input = "\"\"";
        const char* pos = input;
        
        token_t token = next_token(&pos);
        
        REQUIRE(token.tok == TOK_STR);
        REQUIRE(string(token.val.s) == "");
    }
}

TEST_CASE("Lexer tokenizes arrays correctly", "[lexer]") {
    SECTION("Array open bracket") {
        const char* input = "[";
        const char* pos = input;
        
        token_t token = next_token(&pos);
        
        REQUIRE(token.tok == TOK_ARR_OPEN);
        REQUIRE(string(token.text) == "[");
    }
    
    SECTION("Array close bracket") {
        const char* input = "]";
        const char* pos = input;
        
        token_t token = next_token(&pos);
        
        REQUIRE(token.tok == TOK_ARR_CLOSE);
        REQUIRE(string(token.text) == "]");
    }
}

TEST_CASE("Lexer handles comments correctly", "[lexer]") {
    SECTION("Comment followed by word") {
        const char* input = "( this is a comment ) hello";
        const char* pos = input;
        
        // Comments are skipped, so we should get the word after
        token_t token = next_token(&pos);
        
        REQUIRE(token.tok == TOK_WORD);
        REQUIRE(string(token.text) == "hello");
    }
}

TEST_CASE("Lexer handles whitespace correctly", "[lexer]") {
    SECTION("Skips whitespace between tokens") {
        const char* input = "  42   hello  ";
        const char* pos = input;
        
        token_t token1 = next_token(&pos);
        REQUIRE(token1.tok == TOK_I64);
        REQUIRE(token1.val.i == 42);
        
        token_t token2 = next_token(&pos);
        REQUIRE(token2.tok == TOK_WORD);
        REQUIRE(string(token2.text) == "hello");
    }
}

TEST_CASE("Lexer handles EOF correctly", "[lexer]") {
    SECTION("EOF token at end of input") {
        const char* input = "";
        const char* pos = input;
        
        token_t token = next_token(&pos);
        
        REQUIRE(token.tok == TOK_EOF);
    }
    
    SECTION("EOF after other tokens") {
        const char* input = "42";
        const char* pos = input;
        
        token_t token1 = next_token(&pos);
        REQUIRE(token1.tok == TOK_I64);
        
        token_t token2 = next_token(&pos);
        REQUIRE(token2.tok == TOK_EOF);
    }
}

TEST_CASE("Lexer tokenizes quoted words correctly", "[lexer]") {
    SECTION("Quoted word") {
        const char* input = "hello'";
        const char* pos = input;
        
        token_t token = next_token(&pos);
        
        REQUIRE(token.tok == TOK_QUOTE);
        REQUIRE(string(token.val.s) == "hello");
    }
}
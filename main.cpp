#include <iostream>
#include "Tests/testTokens.hpp"
#include "Tests/testLexer.hpp"

using namespace std;

int main() 
{

    test_token_def();
    //nuevo tester de lexer
    test_lexer_basico();
    return 0;
}
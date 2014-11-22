#include <iostream>
#include <assert.h>
#include "ast.h"

int main()
{
    extern void lex_test();
    extern void ast_test();
    lex_test();
    ast_test();
}

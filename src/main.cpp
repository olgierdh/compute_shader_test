#include <vector>

#include <stdio.h>
#include <cassert>
#include <iostream>
#include <cassert>
#include <random>

#include "debug.hpp"
#include "logger.hpp"
#include "stack_allocator.hpp"
#include "static_array.hpp"
#include "application.hpp"

#include "examples/example4.hpp"

#undef main

int main()
{
    {
        application< example4 > app;
        app.initialize();
        app.run();
        app.deinitialize();
    }

    log( "End" );

    return 0;
}

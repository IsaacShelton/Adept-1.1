
#include "../include/asmcontext.h"

AssembleContext::AssembleContext() : builder(context) {}
AssembleContext::AssembleContext(bool is_main_module) : builder(context) {
    is_main = is_main_module;
}

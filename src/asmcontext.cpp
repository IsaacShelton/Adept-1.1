
#include "../include/asmcontext.h"

AssembleContext::AssembleContext() : builder(context) {
    module = llvm::make_unique<llvm::Module>("Adept", context);
}

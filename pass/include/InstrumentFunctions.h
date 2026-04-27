
#pragma once
#include "llvm/IR/PassManager.h"

struct InstrumentFunctions : public llvm::PassInfoMixin<InstrumentFunctions> {
    llvm::PreservedAnalyses run(llvm::Module& M,
        llvm::ModuleAnalysisManager&);
    bool runOnModule(llvm::Module& M);
};


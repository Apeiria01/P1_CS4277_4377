
#include "InstrumentFunctions.h"

#include "llvm/ADT/Twine.h"
#include "llvm/IR/Attributes.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/Module.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Plugins/PassPlugin.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

#include <cstdlib>
#include <cstdint>
#include <string>

using namespace llvm;

#define DEBUG_TYPE "cs4277-p1"

namespace CS4277P1 {
    GlobalVariable* getOrCreateStringGlobal(Module& M, StringRef Name,
        StringRef Contents) {
        if (GlobalVariable* Existing = M.getGlobalVariable(Name, true))
            return Existing;

        LLVMContext& CTX = M.getContext();
        Constant* Initializer = ConstantDataArray::getString(CTX, Contents, true);
        auto* GV = new GlobalVariable(M, Initializer->getType(), true,
            GlobalValue::PrivateLinkage, Initializer,
            Name);
        GV->setUnnamedAddr(GlobalValue::UnnamedAddr::Global);
        GV->setAlignment(Align(1));
        return GV;
    }
    Value* createGlobalStringPtr(IRBuilder<>& Builder, GlobalVariable* GV,
        StringRef Name) {
        LLVMContext& CTX = Builder.getContext();
        Value* Zero = ConstantInt::get(Type::getInt32Ty(CTX), 0);
        return Builder.CreateInBoundsGEP(GV->getValueType(), GV, { Zero, Zero }, Name);
    }

    Instruction* getCanaryAllocaInsertBefore(Function& F) {
        BasicBlock& Entry = F.getEntryBlock();
        BasicBlock::iterator It = Entry.getFirstInsertionPt();
        while (It != Entry.end()) {
            Instruction& I = *It;
            if (auto* AI = dyn_cast<AllocaInst>(&I)) {
                if (AI->isStaticAlloca()) {
                    ++It;
                    continue;
                }
            }

            if (isa<DbgInfoIntrinsic>(&I)) {
                ++It;
                continue;
            }

            break;
        }

        return It == Entry.end() ? Entry.getTerminator() : &*It;
    }
} 

bool InstrumentFunctions::runOnModule(Module& M) {
    bool Changed = false;

    LLVMContext& CTX = M.getContext();
    const DataLayout& DL = M.getDataLayout();
    IntegerType* Int32Ty = Type::getInt32Ty(CTX);
    IntegerType* CanaryTy = IntegerType::get(CTX, DL.getPointerSizeInBits());
    Align CanaryAlign = DL.getPointerABIAlignment(0);
    PointerType* PtrTy = PointerType::get(CTX, 0);

    FunctionType* PrintfTy = FunctionType::get(Int32Ty, { PtrTy }, true);
    FunctionCallee Printf = M.getOrInsertFunction("printf", PrintfTy);

    if (Function* PrintfF = dyn_cast<Function>(Printf.getCallee()))
        PrintfF->setDoesNotThrow();

    Function* Trap = Intrinsic::getOrInsertDeclaration(&M, Intrinsic::trap);
    GlobalVariable* CanaryFailFormat = CS4277P1::getOrCreateStringGlobal(
        M, "__cs4277_canary_fail_format",
        "Stack canary corrupted before leaving %s\n");

    for (Function& F : M) {
        if (F.isDeclaration())
            continue;

        std::string CanaryName = "__cs4277_canary_" + F.getName().str();
        uint64_t CanaryValue =
            (static_cast<uint64_t>(rand()) << 32) ^ static_cast<uint64_t>(rand());
        ConstantInt* ExpectedCanary = ConstantInt::get(CanaryTy, CanaryValue);

        IRBuilder<> EntryBuilder(CS4277P1::getCanaryAllocaInsertBefore(F));
        AllocaInst* CanarySlot =
            EntryBuilder.CreateAlloca(CanaryTy, nullptr, CanaryName);
        CanarySlot->setAlignment(CanaryAlign);
        StoreInst* CanaryStore = EntryBuilder.CreateStore(ExpectedCanary, CanarySlot);
        CanaryStore->setVolatile(true);

        GlobalVariable* FunctionNameGlobal = EntryBuilder.CreateGlobalString(
            F.getName(), Twine(F.getName()) + ".name");
        Value* FunctionName = CS4277P1::createGlobalStringPtr(EntryBuilder, FunctionNameGlobal, "function.name");

        std::vector<Instruction*> Terminators;
        for (auto& BB : F) {
            if (Instruction* Term = BB.getTerminator())
                Terminators.push_back(Term);
        }

        for (auto* Term : Terminators) {
            if (!Term->getParent() || Term->getParent()->getParent() != &F)
                continue;

            IRBuilder<> CheckBuilder(Term);
            LoadInst* LoadedCanary =
                CheckBuilder.CreateLoad(CanaryTy, CanarySlot, "canary.load");
            LoadedCanary->setVolatile(true);

            Value* CanaryIsCorrupt = CheckBuilder.CreateICmpNE(
                LoadedCanary, ExpectedCanary, "canary.corrupt");

            Instruction* FailTerm =
                SplitBlockAndInsertIfThen(CanaryIsCorrupt, Term, true);
            IRBuilder<> FailBuilder(FailTerm);

            Value* FormatPtr = CS4277P1::createGlobalStringPtr(FailBuilder, CanaryFailFormat, "canary.fail.format");
            FailBuilder.CreateCall(Printf, { FormatPtr, FunctionName });
            FailBuilder.CreateCall(Trap);
        }

        Changed = true;
    }

    return Changed;
}

PreservedAnalyses InstrumentFunctions::run(Module& M,
    ModuleAnalysisManager&) {
    return runOnModule(M) ? PreservedAnalyses::none() : PreservedAnalyses::all();
}

PassPluginLibraryInfo getInstrumentFunctionsPluginInfo() {
    return { LLVM_PLUGIN_API_VERSION, "cs4277-p1", LLVM_VERSION_STRING,
            [](PassBuilder& PB) {
              PB.registerPipelineParsingCallback(
                  [](StringRef Name, ModulePassManager& MPM,
                     ArrayRef<PassBuilder::PipelineElement>) {
                    if (Name != "cs4277-p1")
                      return false;

                    MPM.addPass(InstrumentFunctions());
                    return true;
                  });
            } };
}

extern "C" ::llvm::PassPluginLibraryInfo llvmGetPassPluginInfo() {
    return getInstrumentFunctionsPluginInfo();
}

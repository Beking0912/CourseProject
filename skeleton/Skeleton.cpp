#include "llvm/IR/IRBuilder.h"
#include "llvm/Pass.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/ScalarEvolutionExpressions.h"
#include "llvm/IR/InstIterator.h"
#include <set>
#include <map>


using namespace llvm;

namespace {


struct SkeletonPass : public PassInfoMixin<SkeletonPass> {
  
  std::set<int> keyPointLines;
  std::vector<Value*> inputVariables;


void findInputVariables(Function &F) {
  std::set<std::string> ioFunctions = {"getc", "@_fopen", "scanf", "fclose", "fread", "fwrite", "fopen", "_fopen", "\01_fopen"};
  std::map<Value*, int> inputVarLines;
  std::vector<Value*> inputVariables;
  std::map<Value*, int> filePointerInitLines;

  for (Instruction &I : instructions(F)) {
    if (CallInst *CI = dyn_cast<CallInst>(&I)) {
      Function *calledFunc = CI->getCalledFunction();   
      if (calledFunc && ioFunctions.find(calledFunc->getName().str()) != ioFunctions.end()) {
        int ioCallLine = CI->getDebugLoc().getLine();
        errs() << "\nFound I/O call at line: " << ioCallLine << "\n\n";
        
        if (calledFunc->getName() == "\01_fopen" || calledFunc->getName() == "fopen" || calledFunc->getName() == "_fopen") {
        
          Value *filePointer = CI;
          filePointerInitLines[filePointer] = ioCallLine;
          errs() << "File pointer initialized at line: " << ioCallLine << "\n";
          
          //Need to implement it where it finds the name of the file pointer and finds the 
          //occurrences of that name and prints the line numbers with that name
          //For example the name would be "name" for this line FILE *name = fopen("file.txt", "r");
          
        }
		
		else if (calledFunc->getName() == "scanf") {
		
		  for (unsigned int i = 1; i < CI->getNumOperands() - 1; i++) {
            Value *operand = CI->getOperand(i);
            if (operand->getType()->isPointerTy()) {
              errs() << "Input variable (pointer) operand found: " << *operand << "\n";
              inputVariables.push_back(operand);
              inputVarLines[operand] = ioCallLine;

              for (User *U : operand->users()) {
                if (Instruction *useInst = dyn_cast<Instruction>(U)) {
                  errs() << "Input variable used at line: " << useInst->getDebugLoc().getLine() << "\n";
                }
              }
            }
          }   
		}
		
		else if (calledFunc->getName() == "getc" || calledFunc->getName() == "fclose" ) {
		
		  Value *operand = CI->getOperand(0); 
          if (operand->getType()->isPointerTy()) {
          
            std::string operandName;
            raw_string_ostream OS(operandName);
            operand->printAsOperand(OS, false);
            operandName = OS.str();
            
            errs() << "Input variable (file pointer) operand found: " << *operand << "\n";
            inputVariables.push_back(operand);
            inputVarLines[operand] = ioCallLine;
            
            
            for (Instruction &I : instructions(F)) {
  		      for (User::op_iterator OI = I.op_begin(), OE = I.op_end(); OI != OE; ++OI) {
    		    if (isa<Constant>(OI)) {
      			  continue;
    		    }
    
      		    Value *operand = *OI;
                std::string operandStr;
                raw_string_ostream OS(operandStr);
                operand->printAsOperand(OS, false);
    
                if (operandName != "" && operandStr.find(operandName) != std::string::npos) {
                  errs() << "Found input variable at line: " << I.getDebugLoc().getLine() << "\n";
                }
              }
            }
            
          }
		  
		}
      }
    }
  }

  std::set<Value*> keyPointInfluencers;
  for (Value *inputVar : inputVariables) {
    bool isKeyPointInfluencer = false;
    for (User *U : inputVar->users()) {
      if (Instruction *useInst = dyn_cast<Instruction>(U)) {
        int useLine = useInst->getDebugLoc().getLine();
        if (keyPointLines.find(useLine) != keyPointLines.end()) {
          isKeyPointInfluencer = true;
          errs() << "\nUsing Line " << useLine << ", the input variable determining runtime is: " << *inputVar << "\n";
          break;
        }
      }
    }
    if (isKeyPointInfluencer) {
      keyPointInfluencers.insert(inputVar);
    }
  }
  
}
  
PreservedAnalyses run(Module &M, ModuleAnalysisManager &AM) {
    std::set<int> processedLines;
    std::map<std::string, std::tuple<std::string, int, int>> branchDictionary;

    int branchNumber = 0;

    for (Function &F : M.functions()) {

      LLVMContext &Ctx = F.getContext();
      std::vector<Type *> paramTypes = {Type::getInt8PtrTy(Ctx)};
      Type *retType = Type::getVoidTy(Ctx);
      FunctionType *logFuncType = FunctionType::get(retType, paramTypes, false);
      FunctionCallee logFunc = F.getParent()->getOrInsertFunction("logPrint", logFuncType);

      bool flag = false;
      for (BasicBlock &B : F) {
        std::string fileName = F.getParent()->getSourceFileName();
        
        for (Instruction &I : B) {
          if (auto *BI = dyn_cast<BranchInst>(&I)) {
            if (BI->isConditional()) {
              const DebugLoc &Loc = BI->getDebugLoc();

              if (Loc) {
                std::string opcodeName = BI->getOpcodeName();
                int startLine = Loc.getLine();

                for (int i = 0; i < BI->getNumSuccessors(); ++i) {
                  BasicBlock *branch = BI->getSuccessor(i);
                  std::string branchID = opcodeName + "_" + std::to_string(branchNumber);

                  int targetLine = branch->getFirstNonPHI()->getDebugLoc().getLine();
                  branchDictionary[branchID] = std::make_tuple(fileName, startLine, targetLine);
                  branchNumber += 1;

                  IRBuilder<> builder(&*branch->getFirstInsertionPt());
                  Value* args[] = {builder.CreateGlobalStringPtr(branchID)};
                  builder.CreateCall(logFunc, args);

                  flag = true;
                }
              }
            }
          }
        }
      }
      if (flag) {
        errs() << F.getName() << ": func_" << &F << "\n";
        flag = false;
      }
    }
    
    for (const auto &entry : branchDictionary) {
      const std::tuple<std::string, int, int> &location = entry.second;
      keyPointLines.insert(std::get<1>(location));
    }
    
    for (Function &F : M) {
      if (!F.isDeclaration()) {
        findInputVariables(F);
      }
    }
    
    errs() << "\nBranch Dictionary:\n";
    for (const auto &entry : branchDictionary) {
      const std::string &branchID = entry.first;
      const std::tuple<std::string, int, int> &location = entry.second;
      errs() << branchID << ": " << std::get<0>(location) << ", "
             << std::get<1>(location) << ", " << std::get<2>(location) << "\n";
    }
            
    return PreservedAnalyses::all();
  }
};

} // namespace

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return {.APIVersion = LLVM_PLUGIN_API_VERSION,
          .PluginName = "Skeleton pass",
          .PluginVersion = "v0.1",
          .RegisterPassBuilderCallbacks = [](PassBuilder &PB) {
            PB.registerPipelineStartEPCallback(
                [](ModulePassManager &MPM, OptimizationLevel Level) {
                  MPM.addPass(SkeletonPass());
                });
          }};
}
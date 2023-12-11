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
#include "llvm/IR/DebugInfoMetadata.h" 
#include <set>
#include <map>

using namespace llvm;

namespace {

struct SkeletonPass : public PassInfoMixin<SkeletonPass> {
  
  //stores the key points of the branches
  std::set<int> keyPointLines;

void findInputVariables(Function &F) {
  
  //set of i/o functions to look for in the program
  std::set<std::string> ioFunctions = {"getc", "@_fopen", "scanf", "fclose", "fread", "fwrite", "_fread", "_fwrite", "_@fread", "_@fwrite", "\01_fread", "\01_fwrite", "fopen", "_fopen", "\01_fopen"};
  //stores line numbers of where variables are used
  std::vector<Value*> inputVariables;
  //map values to their variable names
  std::map<Value*, std::string> valueToNameMap;
  //Store usage lines of variables
  std::map<std::string, std::vector<int>> variableUsageLines;
  //store line numbers of i/o functions
  std::set<int> ioFunctionLines;
  //map to store variable and their initial lines
  std::map<Value*, std::pair<std::string, int>> varNameAndInitLineMap;
  //store unique variables per line
  std::map<int, std::set<std::string>> uniqueVariablesPerLine;

  // Iterate over all instructions in the function
  for (Instruction &I : instructions(F)) {
    if (auto *DDI = dyn_cast<DbgDeclareInst>(&I)) {
      //get the actual variable name
      if (auto *Var = DDI->getVariable()) {
        valueToNameMap[DDI->getAddress()] = Var->getName().str();
      }
    }
  }

  // Iterate over all instructions in the function
  for (Instruction &I : instructions(F)) {
    //get the actual variable name
    if (auto *DDI = dyn_cast<DbgDeclareInst>(&I)) {
      if (auto *Var = DDI->getVariable()) {
        int initLine = I.getDebugLoc().getLine();
        varNameAndInitLineMap[DDI->getAddress()] = std::make_pair(Var->getName().str(), initLine);
      }
    }
  }

  for (Instruction &I : instructions(F)) {
    if (CallInst *CI = dyn_cast<CallInst>(&I)) {
      Function *calledFunc = CI->getCalledFunction();
      //Handles I/O function calls
      if (calledFunc && ioFunctions.find(calledFunc->getName().str()) != ioFunctions.end()) {
        int ioCallLine = I.getDebugLoc().getLine();
        if (ioCallLine > 0) {
          ioFunctionLines.insert(ioCallLine);
        }
      }
    }

	// Check operands to find usage of variables
    for (User::op_iterator OI = I.op_begin(), OE = I.op_end(); OI != OE; ++OI) {
      Value *operand = *OI;
      if (valueToNameMap.find(operand) != valueToNameMap.end()) {
        std::string varName = valueToNameMap[operand];
        int lineNum = I.getDebugLoc().getLine();

        if (lineNum > 0) {
          variableUsageLines[varName].push_back(lineNum);
        }
      }
    }
  }

  //fill in the map
  for (const auto &varEntry : variableUsageLines) {
    for (int line : varEntry.second) {
      uniqueVariablesPerLine[line].insert(varEntry.first);
    }
  }

  //check if any variable is only used on a single line
  bool anyVariableUsedOnSingleLine = false;
  
  //check if any variable is only used on a single line
  for (const auto &varEntry : variableUsageLines) {
    const std::vector<int> &usageLines = varEntry.second;
    std::set<int> uniqueLines(usageLines.begin(), usageLines.end());

    if (uniqueLines.size() == 1) {
      anyVariableUsedOnSingleLine = true;
      break;
    }
  }

  // Logic to detect seminal inputs based on where the variables are used
  if (anyVariableUsedOnSingleLine) {
    errs() << "\nSeminal Inputs Detection:\n";
    for (const auto &varEntry : variableUsageLines) {
      const std::string &varName = varEntry.first;
      const std::vector<int> &usageLines = varEntry.second;

      std::set<int> uniqueLines(usageLines.begin(), usageLines.end());
      bool usedInIOFunction = false;
      bool usedOnKeyPointLine = false;

      for (int line : uniqueLines) {
        if (ioFunctionLines.find(line) != ioFunctionLines.end()) {
          usedInIOFunction = true;
        }
        if (keyPointLines.find(line) != keyPointLines.end()) {
          usedOnKeyPointLine = true;
        }
      }

      if (usedInIOFunction && usedOnKeyPointLine) {
        for (const auto &varInfo : varNameAndInitLineMap) {
          if (varInfo.second.first == varName) {
            int initLine = varInfo.second.second;
            errs() << "Line " << initLine << ": " << varName << "\n";
            break;
          }
        }
      }
    }
  } 
  else {
    errs() << "\nSeminal Inputs Detection:\n";
    for (const auto &varEntry : variableUsageLines) {
      const std::string &varName = varEntry.first;
      const std::vector<int> &usageLines = varEntry.second;

      bool usedInIOFunction = false;
      bool usedOnKeyPointLine = false;

      for (int line : usageLines) {
        if (ioFunctionLines.find(line) != ioFunctionLines.end() && uniqueVariablesPerLine[line].size() == 1) {
          usedInIOFunction = true;
        }
        if (keyPointLines.find(line) != keyPointLines.end()) {
          usedOnKeyPointLine = true;
        }
      }

      if (usedInIOFunction && usedOnKeyPointLine) {
        for (const auto &varInfo : varNameAndInitLineMap) {
          if (varInfo.second.first == varName) {
            int initLine = varInfo.second.second;
            errs() << "Line " << initLine << ": " << varName << "\n";
            break;
          }
        }
      }
    }
  }

  //Matching the lines of the variables with proper i/o function with those of key points to get the runtime variable
  std::set<Value*> keyPointInfluencers;
  for (Value *inputVar : inputVariables) {
    bool isKeyPointInfluencer = false;
    for (User *U : inputVar->users()) {
      if (Instruction *useInst = dyn_cast<Instruction>(U)) {
        int useLine = useInst->getDebugLoc().getLine();
        if (keyPointLines.find(useLine) != keyPointLines.end()) {
          isKeyPointInfluencer = true;
          errs() << "\nLine " << useLine << ": " << *inputVar << "\n";
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

    //getting the keypoints and branches from Part 1
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
    
    //getting the branches/key points from the branch dictionary
    for (const auto &entry : branchDictionary) {
      const std::tuple<std::string, int, int> &location = entry.second;
      keyPointLines.insert(std::get<1>(location));
    }
    
    //call the input variables
    // for (Function &F : M) {
    //   if (!F.isDeclaration()) {
    //     findInputVariables(F);
    //   }
    // }
    
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

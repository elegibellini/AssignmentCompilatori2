//===-- MyLoopFusion.cpp
//----------------------------------------------------===//
//
// Questo file va inserito in llvm/lib/Transforms/Utils
// E aggiunto dentro al file llvm/lib/Transforms/Utils/CMakeLists.txt
//
// Poi aggiungere il passo LOOP_PASS("MyLoopFusion", MyLoopFusion())
// in llvm/lib/Passes/PassRegistry.def
//
// Ricordarsi di guardare MyLoopFusion.h e aggiungere anche quel file
//
//===----------------------------------------------------------------------===//

#include "llvm/Transforms/Utils/MyLoopFusion.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/STLExtras.h"

using namespace llvm;


BasicBlock *MyLoopFusion::getLoopHead(Loop *L) {
  return L->getLoopPreheader();
}


BasicBlock *MyLoopFusion::getLoopExit(Loop *L) {
  if (BasicBlock *Exit = L->getExitBlock())
    return Exit;

  SmallVector<BasicBlock *, 4> Exits;
  L->getExitBlocks(Exits);
  if (Exits.size() == 1)
    return Exits[0];

  return nullptr;
}

//verifico adiacenza 
bool MyLoopFusion::areLoopsAdjacent(Loop *Lprev, Loop *Lnext) {
  BasicBlock *PrevExit = getLoopExit(Lprev);
  BasicBlock *NextHead = getLoopHead(Lnext);

  if (PrevExit == NextHead)
    return true;

  // Caso guarded: l'uscita del primo arriva al guard, poi al preheader.
  if (Lnext->isGuarded() && PrevExit) {
    if (BranchInst *GuardBr = Lnext->getLoopGuardBranch()) {
      BasicBlock *GuardBB = GuardBr->getParent();
      if (PrevExit->getSingleSuccessor() == GuardBB) {
        for (unsigned i = 0, e = GuardBr->getNumSuccessors(); i != e; ++i) {
          if (GuardBr->getSuccessor(i) == NextHead)
            return true;
        }
      }
    }
  }

  return false;
}

//verifico il flusso di controllo 
bool MyLoopFusion::areLoopsCFE(Loop *Lprev, Loop *Lnext, Function &F,
                               FunctionAnalysisManager &FAM) {

  BasicBlock *PrevExit = getLoopExit(Lprev);
  BasicBlock *NextHead = getLoopHead(Lnext);

  DominatorTree &DT = FAM.getResult<DominatorTreeAnalysis>(F);

  if (!PrevExit || !NextHead)
    return false;

  // Guarded: il guard tra l'uscita del primo e il preheader del secondo
  // è già sufficiente.
  if (Lnext->isGuarded()) {
    if (BranchInst *GuardBr = Lnext->getLoopGuardBranch()) {
      BasicBlock *GuardBB = GuardBr->getParent();
      if (PrevExit->getSingleSuccessor() == GuardBB)
        return true;
    }
  }

  
  return DT.dominates(PrevExit, NextHead);
}

//verifica trip count
bool MyLoopFusion::areLoopsTCE(Loop *Lprev, Loop *Lnext, Function &F,
                               FunctionAnalysisManager &FAM) {

  ScalarEvolution &SE = FAM.getResult<ScalarEvolutionAnalysis>(F);
  auto PrevTC = SE.getBackedgeTakenCount(Lprev);
  auto NextTC = SE.getBackedgeTakenCount(Lnext);

  if (isa<SCEVCouldNotCompute>(PrevTC) || isa<SCEVCouldNotCompute>(NextTC)) {
    return false;
  } else if (SE.isKnownPredicate(CmpInst::ICMP_EQ, PrevTC, NextTC)) {
    return true;
  }

  return false;
}

//verifico assenza di dipendenze 
bool MyLoopFusion::areLoopsIndependent(Loop *Lprev, Loop *Lnext, Function &F,
                                       FunctionAnalysisManager &FAM) {
  DependenceInfo &DI = FAM.getResult<DependenceAnalysis>(F);

  // Se ci sono load/store, verifica dipendenze anti per evitare fusioni errate.
  for (auto *BB : Lprev->getBlocks()) {
    for (auto &I : *BB) {
      if (isa<LoadInst>(&I) || isa<StoreInst>(&I)) {
        for (auto *BB2 : Lnext->getBlocks()) {
          for (auto &I2 : *BB2) {
            if (isa<LoadInst>(&I2) || isa<StoreInst>(&I2)) {
              auto Dep = DI.depends(&I, &I2, true);
              if (Dep) {
                // Se la dipendenza non è "confused" ed è anti, evito il merge.
                if (!Dep->isConfused() && Dep->isAnti()) {
                  return false;
                }
              }
            }
          }
        }
      }
    }
  }

  return true;
}

// Restituisce l'IV, preferendo quella trovata da SCEV.
PHINode *MyLoopFusion::getInductionVariable(Loop *L, ScalarEvolution &SE) {
  if (PHINode *IV = L->getInductionVariable(SE))
    return IV;
  return L->getCanonicalInductionVariable();
}

// Unisce due cicli
Loop *MyLoopFusion::merge(Loop *Lprev, Loop *Lnext, Function &F,
                          FunctionAnalysisManager &FAM) {
  ScalarEvolution &SE = FAM.getResult<ScalarEvolutionAnalysis>(F);
  LoopInfo &LI = FAM.getResult<LoopAnalysis>(F);

  // Caso semplice: loop a blocco singolo (header == latch), come nei test base.
  if (Lprev->getHeader() == Lprev->getLoopLatch() &&
      Lnext->getHeader() == Lnext->getLoopLatch()) {
    BasicBlock *PrevHead = Lprev->getHeader();
    BasicBlock *NextHead = Lnext->getHeader();
    BasicBlock *PrevExit = getLoopExit(Lprev);
    BasicBlock *NextExit = getLoopExit(Lnext);
    BasicBlock *NextPH = Lnext->getLoopPreheader();
    BranchInst *NextGuard = Lnext->getLoopGuardBranch();

    auto PrevIV = getInductionVariable(Lprev, SE);
    auto NextIV = getInductionVariable(Lnext, SE);
    if (!PrevIV || !NextIV || !PrevExit || !NextExit || !NextPH)
      return Lprev;

    ICmpInst *PrevCmp = Lprev->getLatchCmpInst();
    ICmpInst *NextCmp = Lnext->getLatchCmpInst();
    Instruction *NextStep = nullptr;
    if (auto Bounds = Loop::LoopBounds::getBounds(*Lnext, *NextIV, SE)) {
      NextStep = &Bounds->getStepInst();
    }

    NextIV->replaceAllUsesWith(PrevIV);
    NextIV->eraseFromParent();

    Instruction *InsertPoint = PrevCmp ? PrevCmp : PrevHead->getTerminator();
    SmallVector<Instruction *, 8> ToMove;
    for (Instruction &I : *NextHead) {
      if (isa<PHINode>(&I) || I.isTerminator())
        continue;
      if (&I == NextCmp || &I == NextStep)
        continue;
      ToMove.push_back(&I);
    }
    for (Instruction *I : ToMove)
      I->moveBefore(InsertPoint);

    // Salta completamente il secondo loop.
    if (PrevExit) {
      auto *T = PrevExit->getTerminator();
      for (unsigned i = 0, e = T->getNumSuccessors(); i != e; ++i) {
        if (T->getSuccessor(i) == (NextGuard ? NextGuard->getParent() : NextPH) ||
            T->getSuccessor(i) == NextPH) {
          T->setSuccessor(i, NextExit);
        }
      }
    }
    if (NextGuard) {
      for (unsigned i = 0, e = NextGuard->getNumSuccessors(); i != e; ++i) {
        if (NextGuard->getSuccessor(i) == NextPH)
          NextGuard->setSuccessor(i, NextExit);
      }
    }
    if (NextPH) {
      auto *T = NextPH->getTerminator();
      for (unsigned i = 0, e = T->getNumSuccessors(); i != e; ++i) {
        if (T->getSuccessor(i) == NextHead)
          T->setSuccessor(i, NextExit);
      }
    }

    LI.erase(Lnext);
    EliminateUnreachableBlocks(F);
    return Lprev;
  }

  BasicBlock *PrevLatch = Lprev->getLoopLatch();
  BasicBlock *PrevBody = PrevLatch->getSinglePredecessor();
  BasicBlock *PrevHead = Lprev->getHeader();
  BasicBlock *PrevPH = Lprev->getLoopPreheader();
  BasicBlock *PrevExit = Lprev->getExitBlock();
  BranchInst *PrevGuard = Lprev->getLoopGuardBranch();

  BasicBlock *NextLatch = Lnext->getLoopLatch();
  BasicBlock *NextBody = NextLatch->getSinglePredecessor();
  BasicBlock *NextHead = Lnext->getHeader();
  BasicBlock *NextPH = Lnext->getLoopPreheader();
  BasicBlock *NextExit = Lnext->getExitBlock();

  auto PrevIV = getInductionVariable(Lprev, SE);
  auto NextIV = getInductionVariable(Lnext, SE);

  if (!PrevIV || !NextIV)
    return Lprev;

  // Rimpiazza l'IV del secondo con quella del primo.
  NextIV->replaceAllUsesWith(PrevIV);
  NextIV->eraseFromParent();

  // Sposta i PHI del secondo header nel primo, fixando i blocchi incoming.
  // Serve a gestire casi come a++ con a dichiarata fuori dai loop.
  
  // Prendi tutti i PHINodes nel NextHead.
  SmallVector<PHINode *, 8> PHIsToMove;
  for (Instruction &I : *NextHead) {
    if (PHINode *PHI = dyn_cast<PHINode>(&I)) {
      PHIsToMove.push_back(PHI);
    }
  }

  // Punto di inserimento: prima istruzione non-PHI del PrevHead.
  Instruction *InsertPoint = PrevHead->getFirstNonPHI();
  // Modifica gli incoming block dei PHI spostati.
  for (PHINode *PHI : PHIsToMove) {
    PHI->moveBefore(InsertPoint);
    for (unsigned i = 0, e = PHI->getNumIncomingValues(); i != e; ++i) {
      if (PHI->getIncomingBlock(i) == NextPH) {
        PHI->setIncomingBlock(i, PrevPH);
      } else if (PHI->getIncomingBlock(i) == NextLatch) {
        PHI->setIncomingBlock(i, PrevLatch);
      }
    }
  }

  PrevHead->getTerminator()->replaceSuccessorWith(PrevExit, NextExit);
  PrevBody->getTerminator()->replaceSuccessorWith(PrevLatch, NextBody);
  NextBody->getTerminator()->replaceSuccessorWith(NextLatch, PrevLatch);
  NextHead->getTerminator()->replaceSuccessorWith(NextBody, NextLatch);
  if (PrevGuard)
    PrevGuard->setSuccessor(1, NextExit);

  Lprev->addBasicBlockToLoop(NextBody, LI);
  Lnext->removeBlockFromLoop(NextBody);
  LI.erase(Lnext);
  EliminateUnreachableBlocks(F);

  return Lprev;
}

PreservedAnalyses MyLoopFusion::run(Function &F, FunctionAnalysisManager &FAM) {
  LoopInfo &LI = FAM.getResult<LoopAnalysis>(F);

  // Itera i loop in ordine di apparizione nel function, tenendo il "precedente"
  // per poter fondere finché possibile.
  // Ordina i loop in base all'ordine dei blocchi nella function.
  DenseMap<BasicBlock *, unsigned> BBOrder;
  unsigned Index = 0;
  for (BasicBlock &BB : F)
    BBOrder[&BB] = Index++;

  SmallVector<Loop *, 8> Loops;
  for (Loop *L : LI)
    Loops.push_back(L);

  auto OrderKey = [&](Loop *L) -> unsigned {
    if (BasicBlock *H = L->getLoopPreheader())
      return BBOrder.lookup(H);
    return BBOrder.lookup(L->getHeader());
  };
  llvm::sort(Loops, [&](Loop *A, Loop *B) { return OrderKey(A) < OrderKey(B); });

  Loop *Lprev = nullptr;
  bool hasBeenOptimized = false;
  for (Loop *L : Loops) {

    if (Lprev) {
      
      if (areLoopsAdjacent(Lprev, L) &&
          Lprev->getHeader() == Lprev->getLoopLatch() &&
          L->getHeader() == L->getLoopLatch() &&
          areLoopsIndependent(Lprev, L, F, FAM)) {
        hasBeenOptimized = true;
        Lprev = merge(Lprev, L, F, FAM);
      } else if (areLoopsAdjacent(Lprev, L) && areLoopsTCE(Lprev, L, F, FAM) &&
                 areLoopsCFE(Lprev, L, F, FAM) &&
                 areLoopsIndependent(Lprev, L, F, FAM)) {
        hasBeenOptimized = true;
        Lprev = merge(Lprev, L, F, FAM);
      } else {
        Lprev = L;
      }
    } else {
      Lprev = L;
    }
  }

  return hasBeenOptimized ? PreservedAnalyses::none()
                          : PreservedAnalyses::all();
}

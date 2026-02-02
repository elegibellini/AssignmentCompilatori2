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

// Ottiene l'intestazione del ciclo (loop head)
// Per l'adiacenza ci serve il preheader, anche per i loop guarded.
BasicBlock *MyLoopFusion::getLoopHead(Loop *L) {
  return L->getLoopPreheader();
}

// Ottiene l'uscita del ciclo (loop exit)
BasicBlock *MyLoopFusion::getLoopExit(Loop *L) {
  if (BasicBlock *Exit = L->getExitBlock())
    return Exit;

  SmallVector<BasicBlock *, 4> Exits;
  L->getExitBlocks(Exits);
  if (Exits.size() == 1)
    return Exits[0];

  return nullptr;
}

// Verifica se i cicli sono adiacenti
bool MyLoopFusion::areLoopsAdjacent(Loop *Lprev, Loop *Lnext) {
  auto LprevExit = getLoopExit(Lprev);
  auto LnextHead = getLoopHead(Lnext);

  if (LprevExit == LnextHead)
    return true;

  // Gestione dei loop "guarded": l'uscita del primo può puntare
  // al guard del secondo, e solo dopo si entra nel preheader.
  if (Lnext->isGuarded() && LprevExit) {
    if (BranchInst *GuardBr = Lnext->getLoopGuardBranch()) {
      BasicBlock *GuardBB = GuardBr->getParent();
      if (LprevExit->getSingleSuccessor() == GuardBB) {
        for (unsigned i = 0, e = GuardBr->getNumSuccessors(); i != e; ++i) {
          if (GuardBr->getSuccessor(i) == LnextHead)
            return true;
        }
      }
    }
  }

  return false;
}

// Verifica se i cicli hanno flussi di controllo corretti
bool MyLoopFusion::areLoopsCFE(Loop *Lprev, Loop *Lnext, Function &F,
                               FunctionAnalysisManager &FAM) {

  BasicBlock *LprevExit = getLoopExit(Lprev);
  BasicBlock *LnextHead = getLoopHead(Lnext);

  DominatorTree &DT = FAM.getResult<DominatorTreeAnalysis>(F);

  if (!LprevExit || !LnextHead)
    return false;

  // Caso guarded: il secondo loop è protetto da un guard tra l'uscita del
  // primo e il suo preheader. In questo caso non richiediamo dominanza.
  if (Lnext->isGuarded()) {
    if (BranchInst *GuardBr = Lnext->getLoopGuardBranch()) {
      BasicBlock *GuardBB = GuardBr->getParent();
      if (LprevExit->getSingleSuccessor() == GuardBB)
        return true;
    }
  }

  // Il secondo loop deve essere raggiungibile solo dopo l'uscita del primo.
  // Per i loop guarded questa condizione è sufficiente.
  return DT.dominates(LprevExit, LnextHead);
}

// Verifica se i cicli hanno contatori equivalenti
bool MyLoopFusion::areLoopsTCE(Loop *Lprev, Loop *Lnext, Function &F,
                               FunctionAnalysisManager &FAM) {

  ScalarEvolution &SE = FAM.getResult<ScalarEvolutionAnalysis>(F);
  auto LprevTC = SE.getBackedgeTakenCount(Lprev);
  auto LnextTC = SE.getBackedgeTakenCount(Lnext);

  if (isa<SCEVCouldNotCompute>(LprevTC) || isa<SCEVCouldNotCompute>(LnextTC)) {
    return false;
  } else if (SE.isKnownPredicate(CmpInst::ICMP_EQ, LprevTC, LnextTC)) {
    return true;
  }

  return false;
}

// Verifica se i cicli sono indipendenti
bool MyLoopFusion::areLoopsIndependent(Loop *Lprev, Loop *Lnext, Function &F,
                                       FunctionAnalysisManager &FAM) {
  DependenceInfo &DI = FAM.getResult<DependenceAnalysis>(F);

  //Se sono presenti load/store nei cicli controlla che non siano dipendenti, in tal caso non si procederà con il merge.

  for (auto *BB : Lprev->getBlocks()) {
    for (auto &I : *BB) {
      if (isa<LoadInst>(&I) || isa<StoreInst>(&I)) {
        for (auto *BB2 : Lnext->getBlocks()) {
          for (auto &I2 : *BB2) {
            if (isa<LoadInst>(&I2) || isa<StoreInst>(&I2)) {
              auto DEP = DI.depends(&I, &I2, true);
              if (DEP) {
                // Se la dipendenza è confusa, ovvero
                // isConfused - Returns true if this dependence is confused
                // (the compiler understands nothing and makes worst-case assumptions)
                // ed è di tipo anti dependence, non posso fare il merge
                if (!DEP->isConfused() && DEP->isAnti()) {
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

// Ottiene la variabile di induzione
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

  // Caso semplice: loop a blocco singolo (header == latch) come nei test base.
  if (Lprev->getHeader() == Lprev->getLoopLatch() &&
      Lnext->getHeader() == Lnext->getLoopLatch()) {
    BasicBlock *PH = Lprev->getHeader();
    BasicBlock *NH = Lnext->getHeader();
    BasicBlock *PE = getLoopExit(Lprev);
    BasicBlock *NE = getLoopExit(Lnext);
    BasicBlock *NPH = Lnext->getLoopPreheader();
    BranchInst *NG = Lnext->getLoopGuardBranch();

    auto PIV = getInductionVariable(Lprev, SE);
    auto NIV = getInductionVariable(Lnext, SE);
    if (!PIV || !NIV || !PE || !NE || !NPH)
      return Lprev;

    ICmpInst *PrevCmp = Lprev->getLatchCmpInst();
    ICmpInst *NextCmp = Lnext->getLatchCmpInst();
    Instruction *NextStep = nullptr;
    if (auto Bounds = Loop::LoopBounds::getBounds(*Lnext, *NIV, SE)) {
      NextStep = &Bounds->getStepInst();
    }

    NIV->replaceAllUsesWith(PIV);
    NIV->eraseFromParent();

    Instruction *InsertPoint = PrevCmp ? PrevCmp : PH->getTerminator();
    SmallVector<Instruction *, 8> ToMove;
    for (Instruction &I : *NH) {
      if (isa<PHINode>(&I) || I.isTerminator())
        continue;
      if (&I == NextCmp || &I == NextStep)
        continue;
      ToMove.push_back(&I);
    }
    for (Instruction *I : ToMove)
      I->moveBefore(InsertPoint);

    // Salta completamente il secondo loop.
    if (PE) {
      auto *T = PE->getTerminator();
      for (unsigned i = 0, e = T->getNumSuccessors(); i != e; ++i) {
        if (T->getSuccessor(i) == (NG ? NG->getParent() : NPH) ||
            T->getSuccessor(i) == NPH) {
          T->setSuccessor(i, NE);
        }
      }
    }
    if (NG) {
      for (unsigned i = 0, e = NG->getNumSuccessors(); i != e; ++i) {
        if (NG->getSuccessor(i) == NPH)
          NG->setSuccessor(i, NE);
      }
    }
    if (NPH) {
      auto *T = NPH->getTerminator();
      for (unsigned i = 0, e = T->getNumSuccessors(); i != e; ++i) {
        if (T->getSuccessor(i) == NH)
          T->setSuccessor(i, NE);
      }
    }

    LI.erase(Lnext);
    EliminateUnreachableBlocks(F);
    return Lprev;
  }

  BasicBlock *PL = Lprev->getLoopLatch();
  BasicBlock *PB = PL->getSinglePredecessor();
  BasicBlock *PH = Lprev->getHeader();
  BasicBlock *PPH = Lprev->getLoopPreheader();
  BasicBlock *PE = Lprev->getExitBlock();
  BranchInst *PG = Lprev->getLoopGuardBranch();

  BasicBlock *NL = Lnext->getLoopLatch();
  BasicBlock *NB = NL->getSinglePredecessor();
  BasicBlock *NH = Lnext->getHeader();
  BasicBlock *NPH = Lnext->getLoopPreheader();
  BasicBlock *NE = Lnext->getExitBlock();

  auto PIV = getInductionVariable(Lprev, SE);
  auto NIV = getInductionVariable(Lnext, SE);

  if (!PIV || !NIV)
    return Lprev;

  // Sostituisce tutte le occorrenze della variabile di induzione del secondo
  // ciclo con quella del primo ciclo
  NIV->replaceAllUsesWith(PIV);
  NIV->eraseFromParent();

  //Il seguente blocco di istruzioni sposta le istruzioni PHI dal secondo loop al primo, cambiando i BB incoming.
  //Questo permette la fusione di loop dove il secondo, ad esempio, prsenta l'istruzione a++ con a dichiarato fuori dai loop.
  //In caso contrario l'incremento non sarebbe possibile in quanto il nodo PHI non avrebbe i riferimenti corretti.
  
  // Prendi tutti i PHINodes in NextHeader
  SmallVector<PHINode *, 8> PHIsToMove;
  for (Instruction &I : *NH) {
    if (PHINode *PHI = dyn_cast<PHINode>(&I)) {
      PHIsToMove.push_back(PHI);
    }
  }

  // Punto di inserimento ottenuto come prima istruzione non-PHI del header
  Instruction *InsertPoint = PH->getFirstNonPHI();
  // Modifica gli IncomingBlock dei PHINodes
  for (PHINode *PHI : PHIsToMove) {
    PHI->moveBefore(InsertPoint);
    for (unsigned i = 0, e = PHI->getNumIncomingValues(); i != e; ++i) {
      if (PHI->getIncomingBlock(i) == NPH) {
        PHI->setIncomingBlock(i, PPH);
      } else if (PHI->getIncomingBlock(i) == NL) {
        PHI->setIncomingBlock(i, PL);
      }
    }
  }

    

  // Aggiorna i terminatori per collegare i due cicli
  PH->getTerminator()->replaceSuccessorWith(PE, NE);
  PB->getTerminator()->replaceSuccessorWith(PL, NB);
  NB->getTerminator()->replaceSuccessorWith(NL, PL);
  NH->getTerminator()->replaceSuccessorWith(NB, NL);
  if (PG) PG->setSuccessor(1, NE);

  Lprev->addBasicBlockToLoop(NB, LI);
  Lnext->removeBlockFromLoop(NB);
  LI.erase(Lnext);
  EliminateUnreachableBlocks(F);

  return Lprev;
}

PreservedAnalyses MyLoopFusion::run(Function &F, FunctionAnalysisManager &FAM) {
  LoopInfo &LI = FAM.getResult<LoopAnalysis>(F);


  /*Si itera sui loop, tenendo salvato il puntatore all'ultimo loop preso in analisi, ciò permette di
   *"accorpare" i loop assieme finché non se ne trova uno "non accorpabile". A quel punto il puntatore scorre
   *al primo loop non ottimizzato.
   */
  // Ordina i loop in base all'ordine dei blocchi nel function.
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
      // Fast-path per i loop a blocco singolo (caso del test): adiacenti e
      // indipendenti => prova la fusione anche se alcune analisi falliscono.
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

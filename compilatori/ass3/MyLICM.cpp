//===-- MyLICM.cpp ----------------------------------------------------===//
//
// Questo file va inserito in llvm/lib/Transforms/Utils
// E aggiunto dentro al file llvm/lib/Transforms/Utils/CMakeLists.txt
//
// Poi aggiungere il passo LOOP_PASS("MyLICM", MyLICM())
// in llvm/lib/Passes/PassRegistry.def
//
// Ricordarsi di guardare MyLICM.h e aggiungere anche quel file
//
//===----------------------------------------------------------------------===//

#include "llvm/Transforms/Utils/MyLICM.h"

using namespace llvm;



//marco
void MyLICM::markInstruction(Instruction &Inst, int InstructionOrder) {
	LLVMContext &Ctx = Inst.getContext();
	MDNode *N = MDNode::get(Ctx, MDString::get(Ctx, std::to_string(InstructionOrder)));
	Inst.setMetadata("licm.candidate", N);
}

//demarco
void MyLICM::removeMarking(Instruction &Inst){
	
	Inst.setMetadata("licm.candidate", NULL);
}

//test marcatura
bool MyLICM::isInstructionMarked(const Instruction &Inst) {

	return Inst.getMetadata("licm.candidate") ? true : false;
}


// Se un operando è costante, argomento di funzione o riferimento a istruzione
// già invariante, allora è invariante.
bool MyLICM::isOperandInvariant(const Use &Usee, Loop &L) {
	if (isa<Constant>(Usee) || isa<Argument>(Usee)) return true;

	if (auto *Inst = dyn_cast<Instruction>(Usee)) {
		
		if (isInstructionMarked(*Inst)) return true;
		if (!L.contains(Inst)) return true;
	}

	return false;
	
}


//se ha tutti gli operandi invarianti 
// Le PHI non possono essere invarianti.
bool MyLICM::isInstructionInvariant(const Instruction &Inst,Loop &L) {

	if (isa<PHINode>(Inst)) return false;


	bool invariantFlag = true;

	for (const auto &Usee : Inst.operands()) {
		if (!isOperandInvariant(Usee, L)) invariantFlag = false;
	}

	return invariantFlag;
}

//marcatura quelle invarianti
// La variabile i è utile per debug: nel metadato resta l'ordine di discovery.

void MyLICM::checkForInvariantInstructions(Loop &L) {
	int i = 0;
	for (auto *BB : L.getBlocks()) {
		for (auto &Inst : *BB) {
			if (BB && isInstructionInvariant(Inst, L)) {
				markInstruction(Inst, i);
				i++;
			}
		}
	}
}



// istruzione non ha usi al di fuori del loop
bool MyLICM::isDOL(const Instruction &Inst,Loop &L) {
	for (auto *User : Inst.users()) {
		if (!L.contains(dyn_cast<Instruction>(User))) return false;
	}

	return true;
}


// Sposta le istruzioni candidate se sono hoistable.
// Se un'istruzione è candidata e senza usi viene rimossa, altrimenti
// viene cancellato il metadato e spostata nel preheader se domina le uscite o è DOL.


bool MyLICM::moveHoistableInstructions(Loop &L,LoopStandardAnalysisResults &LAR) {
	bool hasChanged = false;
	llvm::SmallVector<std::pair<BasicBlock *, BasicBlock*>> EE;
	L.getExitEdges(EE);
	auto *PH = L.getLoopPreheader();

	if (!PH) {
		llvm::outs()<<"Il loop non è in forma canonica.\n";
		return false;
	}

	
	for (auto *BB : L.getBlocks()) {
		
		for (auto iter = BB->begin(), end = BB->end(); iter != end;) {
			Instruction &I = *iter++;
			if (isInstructionMarked(I)) {

				removeMarking(I);

				if (I.getNumUses() == 0) {
					hasChanged = true;
					I.eraseFromParent();
					continue;
				}

				for (auto &E : EE) {

					if (LAR.DT.dominates(&I, E.second) || isDOL(I, L)) {
							bool areUseesMoved = true;

						for (const auto &U : I.operands()) {

							if (auto *Usee = dyn_cast<Instruction>(U)) {
								if (!(LAR.DT.dominates(Usee, E.second) || isDOL(*Usee, L)))  {
									areUseesMoved = false;
							}
							}
						}
						if (areUseesMoved) {
							hasChanged = true;
							I.moveBefore(PH->getTerminator());


					}

				}
				}
			}
		
		}
	}

	return hasChanged;
	
}



PreservedAnalyses MyLICM::run(Loop &L, LoopAnalysisManager &LAM, LoopStandardAnalysisResults &LAR, LPMUpdater &LU) {
	checkForInvariantInstructions(L);
	return moveHoistableInstructions(L,LAR) ? PreservedAnalyses::none() : PreservedAnalyses::all();
}

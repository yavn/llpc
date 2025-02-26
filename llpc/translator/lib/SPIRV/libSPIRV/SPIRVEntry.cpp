//===- SPIRVEntry.cpp - Base Class for SPIR-V Entities -----------*- C++ -*-===//
//
//                     The LLVM/SPIRV Translator
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
// Copyright (c) 2014 Advanced Micro Devices, Inc. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal with the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// Redistributions of source code must retain the above copyright notice,
// this list of conditions and the following disclaimers.
// Redistributions in binary form must reproduce the above copyright notice,
// this list of conditions and the following disclaimers in the documentation
// and/or other materials provided with the distribution.
// Neither the names of Advanced Micro Devices, Inc., nor the names of its
// contributors may be used to endorse or promote products derived from this
// Software without specific prior written permission.
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS WITH
// THE SOFTWARE.
//
//===----------------------------------------------------------------------===//
/// \file
///
/// This file implements base class for SPIR-V entities.
///
//===----------------------------------------------------------------------===//

#include "SPIRVEntry.h"
#include "SPIRVBasicBlock.h"
#include "SPIRVDebug.h"
#include "SPIRVDecorate.h"
#include "SPIRVFunction.h"
#include "SPIRVInstruction.h"
#include "SPIRVStream.h"
#include "SPIRVType.h"
#include <algorithm>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <utility>

using namespace SPIRV;

namespace SPIRV {

template <typename T> SPIRVEntry *create() {
  return new T();
}

SPIRVEntry *SPIRVEntry::create(Op OpCode) {
  typedef SPIRVEntry *(*SPIRVFactoryTy)();
  struct TableEntry {
    Op Opn;
    SPIRVFactoryTy Factory;
    operator std::pair<const Op, SPIRVFactoryTy>() { return std::make_pair(Opn, Factory); }
  };

  static TableEntry Table[] = {
#define _SPIRV_OP(x, ...) {Op##x, &SPIRV::create<SPIRV##x>},
#include "SPIRVOpCodeEnum.h"
#undef _SPIRV_OP
  };

  typedef std::map<Op, SPIRVFactoryTy> OpToFactoryMapTy;
  static const OpToFactoryMapTy OpToFactoryMap(std::begin(Table), std::end(Table));

  OpToFactoryMapTy::const_iterator Loc = OpToFactoryMap.find(OpCode);
  if (Loc != OpToFactoryMap.end())
    return Loc->second();

  assert(0 && "Not implemented");
  return 0;
}

std::unique_ptr<SPIRV::SPIRVEntry> SPIRVEntry::createUnique(Op OC) {
  return std::unique_ptr<SPIRVEntry>(create(OC));
}

std::unique_ptr<SPIRV::SPIRVExtInst> SPIRVEntry::createUnique(SPIRVExtInstSetKind Set, unsigned ExtOp) {
  return std::unique_ptr<SPIRVExtInst>(new SPIRVExtInst(Set, ExtOp));
}

SPIRVErrorLog &SPIRVEntry::getErrorLog() const {
  return Module->getErrorLog();
}

bool SPIRVEntry::exist(SPIRVId TheId) const {
  return Module->exist(TheId);
}

SPIRVEntry *SPIRVEntry::getOrCreate(SPIRVId TheId) const {
  SPIRVEntry *Entry = nullptr;
  bool Found = Module->exist(TheId, &Entry);
  if (!Found)
    return Module->addForward(TheId, nullptr);
  return Entry;
}

SPIRVValue *SPIRVEntry::getValue(SPIRVId TheId) const {
  return get<SPIRVValue>(TheId);
}

SPIRVType *SPIRVEntry::getValueType(SPIRVId TheId) const {
  return get<SPIRVValue>(TheId)->getType();
}

SPIRVDecoder SPIRVEntry::getDecoder(std::istream &I) {
  return SPIRVDecoder(I, *Module);
}

void SPIRVEntry::setWordCount(SPIRVWord TheWordCount) {
  WordCount = TheWordCount;
}

void SPIRVEntry::setName(const std::string &TheName) {
  Name = TheName;
}

void SPIRVEntry::setModule(SPIRVModule *TheModule) {
  assert(TheModule && "Invalid module");
  if (TheModule == Module)
    return;
  assert(Module == NULL && "Cannot change owner of entry");
  Module = TheModule;
}

bool SPIRVEntry::isEndOfBlock() const {
  switch (OpCode) {
  case OpBranch:
  case OpBranchConditional:
  case OpSwitch:
  case OpKill:
  case OpReturn:
  case OpReturnValue:
  case OpUnreachable:
    return true;
  default:
    return false;
  }
}

// Read words from SPIRV binary and create members for SPIRVEntry.
// The word count and op code has already been read before calling this
// function for creating the SPIRVEntry. Therefore the input stream only
// contains the remaining part of the words for the SPIRVEntry.
void SPIRVEntry::decode(std::istream &I) {
  assert(0 && "Not implemented");
}

std::vector<SPIRVValue *> SPIRVEntry::getValues(const std::vector<SPIRVId> &IdVec) const {
  std::vector<SPIRVValue *> ValueVec;
  ValueVec.reserve(IdVec.size());
  for (auto I : IdVec)
    ValueVec.push_back(getValue(I));
  return ValueVec;
}

std::vector<SPIRVType *> SPIRVEntry::getValueTypes(const std::vector<SPIRVId> &IdVec) const {
  std::vector<SPIRVType *> TypeVec;
  TypeVec.reserve(IdVec.size());
  for (auto I : IdVec)
    TypeVec.push_back(getValue(I)->getType());
  return TypeVec;
}

std::vector<SPIRVId> SPIRVEntry::getIds(const std::vector<SPIRVValue *> &ValueVec) const {
  std::vector<SPIRVId> IdVec;
  IdVec.reserve(ValueVec.size());
  for (auto I : ValueVec)
    IdVec.push_back(I->getId());
  return IdVec;
}

SPIRVEntry *SPIRVEntry::getEntry(SPIRVId TheId) const {
  return Module->getEntry(TheId);
}

void SPIRVEntry::validateFunctionControlMask(SPIRVWord TheFCtlMask) const {
  SPIRVCK(isValidFunctionControlMask(TheFCtlMask), InvalidFunctionControlMask, "");
}

void SPIRVEntry::validateValues(const std::vector<SPIRVId> &Ids) const {
  for (auto I : Ids)
    getValue(I)->validate();
}

void SPIRVEntry::validateBuiltin(SPIRVWord TheSet, SPIRVWord Index) const {
  assert(TheSet != SPIRVWORD_MAX && Index != SPIRVWORD_MAX && "Invalid builtin");
}

void SPIRVEntry::addDecorate(const SPIRVDecorate *Dec) {
  auto Kind = Dec->getDecorateKind();
  Decorates.insert(std::make_pair(Dec->getDecorateKind(), Dec));
  Module->addDecorate(Dec);
  if (Kind == spv::DecorationLinkageAttributes) {
    auto *LinkageAttr = static_cast<const SPIRVDecorateLinkageAttr *>(Dec);
    setName(LinkageAttr->getLinkageName());
  }
}

void SPIRVEntry::addDecorate(Decoration Kind) {
  addDecorate(new SPIRVDecorate(Kind, this));
}

void SPIRVEntry::addDecorate(Decoration Kind, SPIRVWord Literal) {
  addDecorate(new SPIRVDecorate(Kind, this, Literal));
}

void SPIRVEntry::eraseDecorate(Decoration Dec) {
  Decorates.erase(Dec);
}

void SPIRVEntry::takeDecorates(SPIRVEntry *E) {
  assert(E);
  Decorates = std::move(E->Decorates);
}

void SPIRVEntry::setLine(const std::shared_ptr<const SPIRVLine> &L) {
  Line = L;
}

void SPIRVEntry::addMemberDecorate(const SPIRVMemberDecorate *Dec) {
  assert(Dec);
  assert(canHaveMemberDecorates());
  MemberDecorates[Dec->getPair()] = Dec;
  Module->addDecorate(Dec);
}

void SPIRVEntry::addMemberDecorate(SPIRVWord MemberNumber, Decoration Kind) {
  addMemberDecorate(new SPIRVMemberDecorate(Kind, MemberNumber, this));
}

void SPIRVEntry::addMemberDecorate(SPIRVWord MemberNumber, Decoration Kind, SPIRVWord Literal) {
  addMemberDecorate(new SPIRVMemberDecorate(Kind, MemberNumber, this, Literal));
}

void SPIRVEntry::eraseMemberDecorate(SPIRVWord MemberNumber, Decoration Dec) {
  MemberDecorates.erase(std::make_pair(MemberNumber, Dec));
}

void SPIRVEntry::takeMemberDecorates(SPIRVEntry *E) {
  assert(E);
  MemberDecorates = std::move(E->MemberDecorates);
}

void SPIRVEntry::takeAnnotations(SPIRVForward *E) {
  assert(E);
  Module->setName(this, E->getName());
  takeDecorates(E);
  takeMemberDecorates(E);
  if (OpCode == OpFunction)
    static_cast<SPIRVFunction *>(this)->takeExecutionModes(E);
}

// Check if an entry has Kind of decoration and get the literal of the
// first decoration of such kind at Index.
bool SPIRVEntry::hasDecorate(Decoration Kind, size_t Index, SPIRVWord *Result) const {
  DecorateMapType::const_iterator Loc = Decorates.find(Kind);
  if (Loc == Decorates.end())
    return false;
  if (Result)
    *Result = Loc->second->getLiteral(Index);
  return true;
}

const char *SPIRVEntry::getDecorateString(Decoration kind) const {
  DecorateMapType::const_iterator loc = Decorates.find(kind);
  if (loc == Decorates.end())
    return nullptr;

  return loc->second->getLiteralString();
}

// Check if an entry has Kind of member decoration at MemberIndex and get the
// literal of the first decoration of such kind at Index.
bool SPIRVEntry::hasMemberDecorate(SPIRVWord MemberIndex, Decoration Kind, size_t Index, SPIRVWord *Result) const {
  MemberDecorateMapType::const_iterator Loc = MemberDecorates.find(std::make_pair(MemberIndex, Kind));
  if (Loc == MemberDecorates.end())
    return false;
  if (Result)
    *Result = Loc->second->getLiteral(Index);
  return true;
}

// Get literals of all decorations of Kind at Index.
std::set<SPIRVWord> SPIRVEntry::getDecorate(Decoration Kind, size_t Index) const {
  auto Range = Decorates.equal_range(Kind);
  std::set<SPIRVWord> Value;
  for (auto I = Range.first, E = Range.second; I != E; ++I) {
    assert(Index < I->second->getLiteralCount() && "Invalid index");
    Value.insert(I->second->getLiteral(Index));
  }
  return Value;
}

bool SPIRVEntry::hasLinkageType() const {
  return OpCode == OpFunction || OpCode == OpVariable;
}

bool SPIRVEntry::isExtInst(const SPIRVExtInstSetKind InstSet, const SPIRVWord ExtOp) const {
  if (isExtInst()) {
    const SPIRVExtInst *EI = static_cast<const SPIRVExtInst *>(this);
    if (EI->getExtSetKind() == InstSet) {
      return EI->getExtOp() == ExtOp;
    }
  }
  return false;
}

SPIRVLinkageTypeKind SPIRVEntry::getLinkageType() const {
  assert(hasLinkageType());
  DecorateMapType::const_iterator Loc = Decorates.find(DecorationLinkageAttributes);
  if (Loc == Decorates.end())
    return LinkageTypeInternal;
  return static_cast<const SPIRVDecorateLinkageAttr *>(Loc->second)->getLinkageType();
}

void SPIRVEntry::setLinkageType(SPIRVLinkageTypeKind LT) {
  assert(isValid(LT));
  assert(hasLinkageType());
  addDecorate(new SPIRVDecorateLinkageAttr(this, Name, LT));
}

void SPIRVEntry::updateModuleVersion() const {
  if (!Module)
    return;

  Module->setMinSPIRVVersion(getRequiredSPIRVVersion());
}

std::istream &operator>>(std::istream &I, SPIRVEntry &E) {
  E.decode(I);
  return I;
}

SPIRVEntryPoint::SPIRVEntryPoint(SPIRVModule *TheModule, SPIRVExecutionModelKind TheExecModel, SPIRVId TheId,
                                 const std::string &TheName)
    : SPIRVAnnotation(TheModule->get<SPIRVFunction>(TheId), getSizeInWords(TheName) + 3), ExecModel(TheExecModel),
      Name(TheName) {
}

void SPIRVEntryPoint::decode(std::istream &I) {
  uint32_t Start = I.tellg();
  getDecoder(I) >> ExecModel >> Target >> Name;
  uint32_t Curr = I.tellg();
  uint32_t NumInOuts = WordCount - (Curr - Start) / sizeof(uint32_t) - 1;
  InOuts.resize(NumInOuts);
  getDecoder(I) >> InOuts;
  Module->setName(getOrCreateTarget(), Name);
  Module->addEntryPoint(this);
}

void SPIRVExecutionMode::decode(std::istream &I) {
  getDecoder(I) >> Target >> ExecMode;
  bool MergeEM = false;
  switch (ExecMode) {
  case ExecutionModeLocalSize:
    WordLiterals.resize(3);
    break;
  case ExecutionModeInvocations:
  case ExecutionModeOutputVertices:
  case ExecutionModeOutputPrimitivesEXT:
    WordLiterals.resize(1);
    break;
  case ExecutionModeDenormPreserve:
  case ExecutionModeDenormFlushToZero:
  case ExecutionModeSignedZeroInfNanPreserve:
  case ExecutionModeRoundingModeRTE:
  case ExecutionModeRoundingModeRTZ:
    WordLiterals.resize(1);
    MergeEM = true;
    break;
  default:
    // Do nothing. Keep this to avoid VS2013 warning.
    break;
  }
  getDecoder(I) >> WordLiterals;
  if (MergeEM)
    getOrCreateTarget()->mergeExecutionMode(this);
  else
    getOrCreateTarget()->addExecutionMode(this);
}

void SPIRVExecutionModeId::decode(std::istream &I) {
  getDecoder(I) >> Target >> ExecMode;
  switch (ExecMode) {
  case ExecutionModeLocalSizeId:
    Operands.resize(3);
    break;
  default:
    // Do nothing. Keep this to avoid VS2013 warning.
    break;
  }
  getDecoder(I) >> Operands;
}

SPIRVForward *SPIRVAnnotationGeneric::getOrCreateTarget() const {
  SPIRVEntry *Entry = nullptr;
  bool Found = Module->exist(Target, &Entry);
  assert((!Found || Entry->getOpCode() == OpForward) && "Annotations only allowed on forward");
  if (!Found)
    Entry = Module->addForward(Target, nullptr);
  return static_cast<SPIRVForward *>(Entry);
}

SPIRVName::SPIRVName(const SPIRVEntry *TheTarget, const std::string &TheStr)
    : SPIRVAnnotation(TheTarget, getSizeInWords(TheStr) + 2), Str(TheStr) {
}

void SPIRVName::decode(std::istream &I) {
  getDecoder(I) >> Target >> Str;
  Module->setName(getOrCreateTarget(), Str);
}

void SPIRVName::validate() const {
  assert(WordCount == getSizeInWords(Str) + 2 && "Incorrect word count");
}

_SPIRV_IMP_ENCDEC2(SPIRVString, Id, Str)
_SPIRV_IMP_DECODE3(SPIRVMemberName, Target, MemberNumber, Str)

void SPIRVLine::decode(std::istream &I) {
  getDecoder(I) >> FileName >> Line >> Column;
  std::shared_ptr<const SPIRVLine> L(this);
  Module->setCurrentLine(L);
}

void SPIRVLine::validate() const {
  assert(OpCode == OpLine);
  assert(WordCount == 4);
  assert(get<SPIRVEntry>(FileName)->getOpCode() == OpString);
  assert(Line != SPIRVWORD_MAX);
  assert(Column != SPIRVWORD_MAX);
  assert(!hasId());
}

void SPIRVMemberName::validate() const {
  assert(OpCode == OpMemberName);
  assert(WordCount == getSizeInWords(Str) + FixedWC);
  assert(get<SPIRVEntry>(Target)->getOpCode() == OpTypeStruct);
  assert(MemberNumber < get<SPIRVTypeStruct>(Target)->getStructMemberCount());
}

SPIRVExtInstImport::SPIRVExtInstImport(SPIRVModule *TheModule, SPIRVId TheId, const std::string &TheStr)
    : SPIRVEntry(TheModule, 2 + getSizeInWords(TheStr), OC, TheId), Str(TheStr) {
  validate();
}

void SPIRVExtInstImport::decode(std::istream &I) {
  getDecoder(I) >> Id >> Str;
  Module->importBuiltinSetWithId(Str, Id);
}

void SPIRVExtInstImport::validate() const {
  SPIRVEntry::validate();
  assert(!Str.empty() && "Invalid builtin set");
}

void SPIRVMemoryModel::decode(std::istream &I) {
  SPIRVAddressingModelKind AddrModel;
  SPIRVMemoryModelKind MemModel;
  getDecoder(I) >> AddrModel >> MemModel;
  Module->setAddressingModel(AddrModel);
  Module->setMemoryModel(MemModel);
}

void SPIRVMemoryModel::validate() const {
  auto AM = Module->getAddressingModel();
  auto MM = Module->getMemoryModel();
  SPIRVCK(isValid(AM), InvalidAddressingModel, "Actual is " + std::to_string(AM));
  SPIRVCK(isValid(MM), InvalidMemoryModel, "Actual is " + std::to_string(MM));
}

void SPIRVSource::decode(std::istream &I) {
  SourceLanguage Lang = SourceLanguageUnknown;
  SPIRVWord Ver = SPIRVWORD_MAX;
  getDecoder(I) >> Lang >> Ver;
  Module->setSourceLanguage(Lang, Ver);
  if (WordCount > 3) {
    getDecoder(I) >> File;
    Module->setSourceFile(File);
  }
  if (WordCount > 4)
    getDecoder(I) >> Source;
}

SPIRVSourceContinued::SPIRVSourceContinued(SPIRVModule *M, const std::string &SS)
    : SPIRVEntryNoId(M, 1 + getSizeInWords(SS)), Str(SS) {
}

void SPIRVSourceContinued::decode(std::istream &I) {
  getDecoder(I) >> Str;
}

SPIRVSourceExtension::SPIRVSourceExtension(SPIRVModule *M, const std::string &SS)
    : SPIRVEntryNoId(M, 1 + getSizeInWords(SS)), S(SS) {
}

void SPIRVSourceExtension::decode(std::istream &I) {
  getDecoder(I) >> S;
  Module->getSourceExtension().insert(S);
}

SPIRVExtension::SPIRVExtension(SPIRVModule *M, const std::string &SS)
    : SPIRVEntryNoId(M, 1 + getSizeInWords(SS)), S(SS) {
}

void SPIRVExtension::decode(std::istream &I) {
  getDecoder(I) >> S;
  Module->getExtension().insert(S);
}

SPIRVCapability::SPIRVCapability(SPIRVModule *M, SPIRVCapabilityKind K) : SPIRVEntryNoId(M, 2), Kind(K) {
  updateModuleVersion();
}

void SPIRVCapability::decode(std::istream &I) {
  getDecoder(I) >> Kind;
  Module->addCapability(Kind);
}

SPIRVModuleProcessed::SPIRVModuleProcessed(SPIRVModule *M, const std::string &SS)
    : SPIRVEntryNoId(M, 1 + getSizeInWords(SS)), Str(SS) {
}

void SPIRVModuleProcessed::decode(std::istream &I) {
  getDecoder(I) >> Str;
}

} // namespace SPIRV

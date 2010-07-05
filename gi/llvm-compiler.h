/* -*- Mode: C++; c-basic-offset: 2 -*-
 * vim: tabstop=2 shiftwidth=2 expandtab
 *
 * Copyright (C) 2010 Johan Dahlin <johan@gnome.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301
 * USA
 */

#ifndef __PYGI_LLVM_COMPILER_H__
#define __PYGI_LLVM_COMPILER_H__

#include <girepository.h>
#include <Python.h>

#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/LLVMContext.h>
#include <llvm/Module.h>
#include <llvm/Support/IRBuilder.h>

namespace pygi {

  class LLVMCompiler {
  private:
    llvm::LLVMContext &mCtx;
    llvm::ExecutionEngine *mEE;
    llvm::Module *mModule;

    llvm::ExecutionEngine* createExecutionEngine(void);

    llvm::Value* tupleGetItem(llvm::BasicBlock *block,
                              llvm::Value *value,
                              unsigned i);
    void typeCheck(GICallableInfo *callableInfo,
                   GIArgInfo *argInfo,
                   GITypeInfo *typeInfo,
                   int i,
                   llvm::BasicBlock **block,
                   llvm::Value *value);
    llvm::Value* valueAsNative(GITypeInfo *typeInfo,
                               llvm::BasicBlock *parentBB,
                               llvm::Value *value);
    llvm::Value* valueFromNative(GITypeInfo *typeInfo,
                                 llvm::BasicBlock *parentBB,
                                 llvm::Value *value);
    const llvm::Type* getTypeFromTypeInfo(GITypeInfo *typeInfo);
    llvm::BasicBlock* createException(GICallableInfo *callableInfo,
                                      GIArgInfo *argInfo,
                                      GITypeInfo *typeInfo,
                                      int i,
                                      llvm::BasicBlock *block);
    void createIf(llvm::BasicBlock **block,
                  llvm::ICmpInst::Predicate pred,
                  llvm::Value* LHS,
                  llvm::Value* RHS,
                  llvm::BasicBlock *exitBB);
    const char * formatTypeForException(GITypeInfo *typeInfo);
    char * getFunctionName(GIFunctionInfo *info);
    llvm::Value * createPyNone();
    void loadSymbols();

  public:
    LLVMCompiler(llvm::LLVMContext &ctx);
    PyCFunction compile(GIFunctionInfo *info);
  };

} // End pygi namespace

#endif /* __PYGI_LLVM_COMPILER_H__ */

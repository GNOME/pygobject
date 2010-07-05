/* -*- Mode: C++; c-basic-offset: 2 -*-
 * vim: tabstop=2 shiftwidth=2 expandtab
 *
 * Copyright (C) 2010 Johan Dahlin <johan@gnome.org>
 *
 *   llvm-compiler.cpp: pygi llvm compiler
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

#include "llvm-compiler.h"

#include <math.h>
#include <string>

#include <Python.h>
#define NO_IMPORT_PYGOBJECT
#include <pygobject.h>
#include "pygobject-external.h"
#include "pygi.h"
#include "pygi-info.h"

#include <llvm/Analysis/Verifier.h>
#include <llvm/DerivedTypes.h>
#include <llvm/ExecutionEngine/JIT.h>
#include <llvm/ExecutionEngine/JITMemoryManager.h>
#include <llvm/CodeGen/MachineFunction.h>
#include <llvm/Linker.h>
#include <llvm/LLVMContext.h>
#include <llvm/Module.h>
#include <llvm/Support/IRBuilder.h>
#include <llvm/Target/TargetSelect.h>

#define DEBUG 1
#define TIMEIT 1

/*
 * PyGI LLVM runtime functions
 */
// FIXME: replace with o->ob_type == &Py_Type
long _PyLong_Check(PyObject *o) {
  return PyLong_Check(o) || PyInt_Check(o);
}

long _PyFloat_Check(PyObject *o) {
  return PyFloat_Check(o) || PyInt_Check(o) || PyLong_Check(o);
}

long _PyString_Check(PyObject *o) {
  return PyString_Check(o);
}

long _PyGObject_Check(PyObject *o, GType type) {
    if (!pygobject_check(o, _PyGObject_Type))
        return 0;
    PyGObject *go = (PyGObject*)o;
    if (!g_type_is_a(G_OBJECT_TYPE(go->obj), type))
        return 0;
    return 1;
}

GObject * _PyGObject_Get(PyObject *o) {
   return pygobject_get(o);
}

PyObject* _PyGObject_New(GObject *o) {
   return pygobject_new(o);
}


/*
 * LLVMCompiler
 *
 */

using namespace pygi;

static llvm::IRBuilder<> Builder(llvm::getGlobalContext());
// PyObject = { ssize_t ob_refcnt, struct* ob_type }
// PyTupleObject = { ssize_t ob_refcnt, struct* ob_type, ssize_t ob_size, PyObject* }
static llvm::Type* pyObjectPtr = NULL;
static llvm::Type* gObjectPtr = NULL;

LLVMCompiler::LLVMCompiler(llvm::LLVMContext &ctx) :
  mCtx(ctx)
{
  mModule = new llvm::Module("pygi", mCtx);
  mEE = createExecutionEngine();

}

llvm::ExecutionEngine *
LLVMCompiler::createExecutionEngine(void)
{
  std::string errStr;
  llvm::ExecutionEngine *EE;

  llvm::InitializeNativeTarget();

  EE = llvm::EngineBuilder(mModule).setErrorStr(&errStr).setEngineKind(llvm::EngineKind::JIT).create();
  if (!EE) {
    g_error("Failed to construct ExecutionEngine: %s\n", errStr.c_str());
  }
  return EE;
}

const llvm::Type *
LLVMCompiler::getTypeFromTypeInfo(GITypeInfo *typeInfo)
{
  switch (g_type_info_get_tag(typeInfo)) {
  case GI_TYPE_TAG_VOID:
    return llvm::Type::getVoidTy(mCtx);
  case GI_TYPE_TAG_DOUBLE:
    return llvm::Type::getDoubleTy(mCtx);
  case GI_TYPE_TAG_INT8:
  case GI_TYPE_TAG_UINT8:
    return llvm::Type::getInt8Ty(mCtx);
  case GI_TYPE_TAG_INT:
    if (sizeof(int) == 4)
        return llvm::Type::getInt32Ty(mCtx);
    else if (sizeof(int) == 8)
        return llvm::Type::getInt64Ty(mCtx);
    else
        g_assert_not_reached();
  case GI_TYPE_TAG_INT32:
  case GI_TYPE_TAG_UINT32:
    return llvm::Type::getInt32Ty(mCtx);
  case GI_TYPE_TAG_ARRAY:
    return llvm::PointerType::getUnqual(llvm::Type::getInt32Ty(mCtx));
  case GI_TYPE_TAG_UTF8:
    return llvm::PointerType::getUnqual(llvm::Type::getInt8Ty(mCtx));
  case GI_TYPE_TAG_INTERFACE:
    return gObjectPtr;
  default:
    g_error("%s: unsupported type tag: %d\n", __FUNCTION__, g_type_info_get_tag(typeInfo));
    return 0;
  }
}

void
LLVMCompiler::createIf(llvm::BasicBlock **block,
                       llvm::ICmpInst::Predicate pred,
                       llvm::Value* LHS,
                       llvm::Value* RHS,
                       llvm::BasicBlock *exitBB)
{
  llvm::Function *parent = (*block)->getParent();

  llvm::BasicBlock *thenBB = llvm::BasicBlock::Create(mCtx, "bb", parent, 0);

  // if (LHS PRED RHS) { okay } else { ... }
  llvm::ICmpInst* cmpInst = new llvm::ICmpInst(*(*block), pred, LHS, RHS, "c");
  llvm::BranchInst::Create(exitBB, thenBB, cmpInst, *block);

  *block = thenBB;
  Builder.SetInsertPoint(thenBB);
}

const char *
LLVMCompiler::formatTypeForException(GITypeInfo *typeInfo)
{
  switch (g_type_info_get_tag(typeInfo)) {
  case GI_TYPE_TAG_DOUBLE:
  case GI_TYPE_TAG_INT:
  case GI_TYPE_TAG_UINT:
  case GI_TYPE_TAG_INT8:
  case GI_TYPE_TAG_UINT8:
  case GI_TYPE_TAG_INT16:
  case GI_TYPE_TAG_UINT16:
  case GI_TYPE_TAG_INT32:
  case GI_TYPE_TAG_UINT32:
  case GI_TYPE_TAG_INT64:
  case GI_TYPE_TAG_UINT64:
     return "a number";
  case GI_TYPE_TAG_UTF8:
     return "a string";
  default:
    g_warning("%s: unsupported type tag: %d\n", __FUNCTION__, g_type_info_get_tag(typeInfo));
    return "an unknown type";
  }
}

llvm::BasicBlock *
LLVMCompiler::createException(GICallableInfo *callableInfo,
                              GIArgInfo *argInfo,
                              GITypeInfo *typeInfo,
                              int i,
                              llvm::BasicBlock *block)
{
  llvm::Function *parent = block->getParent();
  llvm::BasicBlock* exitBB = llvm::BasicBlock::Create(mCtx, "bb", parent, 0);
  Builder.SetInsertPoint(exitBB);

  llvm::Constant *f = mModule->getOrInsertFunction("PyErr_Format",
        llvm::Type::getVoidTy(mCtx),
        pyObjectPtr, llvm::PointerType::getUnqual(llvm::IntegerType::get(mCtx, 8)), NULL);

  llvm::Value *exc = new llvm::GlobalVariable(pyObjectPtr, true, llvm::GlobalValue::ExternalLinkage, 0, "PyExc_TypeError");
  // FIXME: add "not a, float" to the end
  char *msg = g_strdup_printf("argument %d to %s must be %s",
                              i+1, g_base_info_get_name((GIBaseInfo*)callableInfo),
                              this->formatTypeForException(typeInfo));
  Builder.CreateCall2(f,
                      Builder.CreateLoad(exc),
                      Builder.CreateGlobalStringPtr(msg, "format"));
  g_free(msg);

  Builder.CreateRet(llvm::ConstantInt::get(llvm::Type::getInt32Ty(mCtx), 0));
  return exitBB;
}

void
LLVMCompiler::typeCheck(GICallableInfo *callableInfo,
                        GIArgInfo *argInfo,
                        GITypeInfo *typeInfo,
                        int i,
                        llvm::BasicBlock **block,
                        llvm::Value *value)
{
  switch (g_type_info_get_tag(typeInfo)) {
  case GI_TYPE_TAG_DOUBLE: {
    llvm::Constant *f = mModule->getOrInsertFunction("_PyFloat_Check", llvm::Type::getInt32Ty(mCtx), pyObjectPtr, NULL);
    llvm::Value *v = Builder.CreateCall(f, value, "l");
    llvm::BasicBlock *excBlock = this->createException(callableInfo, argInfo, typeInfo, i, *block);
    this->createIf(block, llvm::ICmpInst::ICMP_EQ, v, llvm::ConstantInt::get(llvm::Type::getInt32Ty(mCtx), 0), excBlock);
    Builder.SetInsertPoint((*block));
    break;
  }
  case GI_TYPE_TAG_INT:
  case GI_TYPE_TAG_INT8:
  case GI_TYPE_TAG_UINT8:
  case GI_TYPE_TAG_INT16:
  case GI_TYPE_TAG_UINT16:
  case GI_TYPE_TAG_INT32:
  case GI_TYPE_TAG_UINT32: {
    llvm::Constant *f = mModule->getOrInsertFunction("_PyLong_Check", llvm::Type::getInt32Ty(mCtx), pyObjectPtr, NULL);
    llvm::Value *v = Builder.CreateCall(f, value);
    llvm::BasicBlock *excBlock = this->createException(callableInfo, argInfo, typeInfo, i, *block);
    this->createIf(block, llvm::ICmpInst::ICMP_EQ, v, llvm::ConstantInt::get(llvm::Type::getInt32Ty(mCtx), 0), excBlock);
    break;
  }
  case GI_TYPE_TAG_UTF8: {
    llvm::Constant *f = mModule->getOrInsertFunction("_PyString_Check", llvm::Type::getInt32Ty(mCtx), pyObjectPtr, NULL);
    llvm::Value *v = Builder.CreateCall(f, value);
    llvm::BasicBlock *excBlock = this->createException(callableInfo, argInfo, typeInfo, i, *block);
    this->createIf(block, llvm::ICmpInst::ICMP_EQ, v, llvm::ConstantInt::get(llvm::Type::getInt32Ty(mCtx), 0), excBlock);
    break;
  }
  case GI_TYPE_TAG_ARRAY: {
    /* FIXME */
    break;
  }
  case GI_TYPE_TAG_INTERFACE: {
    GIBaseInfo *ifaceInfo = g_type_info_get_interface(typeInfo);
    g_assert(ifaceInfo != NULL);
    GIInfoType infoType = g_base_info_get_type(ifaceInfo);
    switch (infoType) {
        case GI_INFO_TYPE_OBJECT: {
           GType objectType = g_registered_type_info_get_g_type((GIRegisteredTypeInfo*)ifaceInfo);
           llvm::Constant *f = mModule->getOrInsertFunction("_PyGObject_Check", llvm::Type::getInt32Ty(mCtx), pyObjectPtr,
                                                           llvm::Type::getInt32Ty(mCtx), NULL);
           llvm::Value *v = Builder.CreateCall2(f, value, llvm::ConstantInt::get(llvm::Type::getInt32Ty(mCtx), objectType));
           llvm::BasicBlock *excBlock = this->createException(callableInfo, argInfo, typeInfo, i, *block);
           this->createIf(block, llvm::ICmpInst::ICMP_EQ, v, llvm::ConstantInt::get(llvm::Type::getInt32Ty(mCtx), 0), excBlock);
          break;
        }
        default:
          g_error("%s: unsupported info type: %d\n", __FUNCTION__, infoType);
          break;
    }
    g_base_info_unref((GIBaseInfo*)ifaceInfo);
    break;
  }
  default:
    g_error("%s: unsupported type tag: %d\n", __FUNCTION__, g_type_info_get_tag(typeInfo));
    break;
  }
}

llvm::Value *
LLVMCompiler::valueAsNative(GITypeInfo *typeInfo,
                            llvm::BasicBlock *parentBB,
                            llvm::Value *value)
{
  llvm::Value *retval;
  bool isSigned = false;
  switch (g_type_info_get_tag(typeInfo)) {
  case GI_TYPE_TAG_DOUBLE: {
    // FIXME: ->ob_fval
    llvm::Constant *f = mModule->getOrInsertFunction("PyFloat_AsDouble",
                                                    llvm::Type::getDoubleTy(mCtx), pyObjectPtr, NULL);
    llvm::Value *v = Builder.CreateCall(f, value);
    retval = Builder.CreateSIToFP(v, llvm::Type::getDoubleTy(mCtx), "arg_sitofp");
    break;
  }
  case GI_TYPE_TAG_INT:
  case GI_TYPE_TAG_INT8:
  case GI_TYPE_TAG_INT16:
  case GI_TYPE_TAG_INT32:
    isSigned = true;
  case GI_TYPE_TAG_UINT8:
  case GI_TYPE_TAG_UINT16:
  case GI_TYPE_TAG_UINT32: {
    llvm::Constant *f = mModule->getOrInsertFunction("PyLong_AsLong",
                                                    llvm::Type::getInt32Ty(mCtx), pyObjectPtr, NULL);
    llvm::Value *v = Builder.CreateCall(f, value);
    retval = Builder.CreateIntCast(v, this->getTypeFromTypeInfo(typeInfo), isSigned, "cast");
    break;
  }
  case GI_TYPE_TAG_UTF8: {
    llvm::Constant *f = mModule->getOrInsertFunction("PyString_AsString",
                                                    llvm::PointerType::getUnqual(llvm::IntegerType::get(mCtx, 8)),
                                                    pyObjectPtr, NULL);
    retval = Builder.CreateCall(f, value);
    break;
  }
  case GI_TYPE_TAG_ARRAY: {
    retval = llvm::ConstantExpr::getNullValue(llvm::Type::getInt32Ty(mCtx)->getPointerTo());
    break;
  }
  case GI_TYPE_TAG_INTERFACE: {
    GIBaseInfo *ifaceInfo = g_type_info_get_interface(typeInfo);
    g_assert(ifaceInfo != NULL);
    GIInfoType infoType = g_base_info_get_type(ifaceInfo);
    switch (infoType) {
        case GI_INFO_TYPE_OBJECT: {
           llvm::Constant *f = mModule->getOrInsertFunction("_PyGObject_Get",
                                                           gObjectPtr, pyObjectPtr, NULL);
           retval = Builder.CreateCall(f, value);
          break;
        }
        default:
          g_error("%s: unsupported info type: %d\n", __FUNCTION__, infoType);
          retval = 0;
          break;
    }
    g_base_info_unref((GIBaseInfo*)ifaceInfo);
    break;
  }
  default:
    g_error("%s: unsupported type tag: %d\n", __FUNCTION__, g_type_info_get_tag(typeInfo));
    retval = 0;
    break;
  }
  return retval;
}

llvm::Value *
LLVMCompiler::valueFromNative(GITypeInfo *typeInfo,
                              llvm::BasicBlock *parentBB,
                              llvm::Value *value)
{
  llvm::Value *retval;
  bool isSigned = false;
  switch (g_type_info_get_tag(typeInfo)) {
  case GI_TYPE_TAG_DOUBLE: {
    llvm::Constant *f = mModule->getOrInsertFunction("PyFloat_FromDouble",
                                                    pyObjectPtr, llvm::Type::getDoubleTy(mCtx), NULL);
    retval = Builder.CreateCall(f, value);
    break;
  }
  case GI_TYPE_TAG_INT8:
  case GI_TYPE_TAG_INT16:
  case GI_TYPE_TAG_INT32:
    isSigned = true;
  case GI_TYPE_TAG_UINT8:
  case GI_TYPE_TAG_UINT16:
  case GI_TYPE_TAG_UINT32: {
    llvm::Constant *f = mModule->getOrInsertFunction("PyLong_FromLong",
                                                    pyObjectPtr, llvm::Type::getInt32Ty(mCtx), NULL);
    llvm::Value *casted = Builder.CreateIntCast(value, llvm::Type::getInt32Ty(mCtx), isSigned, "cast");
    retval = Builder.CreateCall(f, casted);
    break;
  }
  case GI_TYPE_TAG_UTF8: {
    llvm::Constant *f = mModule->getOrInsertFunction("PyString_FromString",
                                                    pyObjectPtr, llvm::PointerType::getUnqual(llvm::IntegerType::get(mCtx, 8)), NULL);
    retval = Builder.CreateCall(f, value);
    break;
  }
  case GI_TYPE_TAG_ARRAY: {
    retval = llvm::ReturnInst::Create(mCtx, llvm::ConstantInt::get(llvm::Type::getInt32Ty(mCtx), 0), parentBB);
    break;
  }
  case GI_TYPE_TAG_INTERFACE: {
    GIBaseInfo *ifaceInfo = g_type_info_get_interface(typeInfo);
    g_assert(ifaceInfo != NULL);
    GIInfoType infoType = g_base_info_get_type(ifaceInfo);
    switch (infoType) {
        case GI_INFO_TYPE_OBJECT: {
           llvm::Constant *f = mModule->getOrInsertFunction("_PyGObject_New",
                                                          pyObjectPtr, gObjectPtr, NULL);
           retval = Builder.CreateCall(f, value);
          break;
        }
        default:
          g_error("%s: unsupported info type: %d\n", __FUNCTION__, infoType);
          retval = 0;
          break;
    }
    g_base_info_unref((GIBaseInfo*)ifaceInfo);
    break;
  }
  default:
    g_error("%s: unsupported type tag: %d\n", __FUNCTION__, g_type_info_get_tag(typeInfo));
    retval = 0;
    break;
  }
  return retval;
}

llvm::Value *
LLVMCompiler::tupleGetItem(llvm::BasicBlock *block,
                           llvm::Value *value,
                           unsigned int i)
{
  // FIXME: ->ob_item
  llvm::Constant *f = mModule->getOrInsertFunction("PyTuple_GetItem",
                                                  pyObjectPtr, pyObjectPtr, llvm::Type::getInt32Ty(mCtx), NULL);
  return Builder.CreateCall2(f, value,
                             llvm::ConstantInt::get(llvm::Type::getInt32Ty(mCtx), i));
}

static void *
pyg_get_native_address(GIFunctionInfo *info)
{
  void * nativeAddress;

  const gchar * shlib = g_irepository_get_shared_library(NULL, g_base_info_get_namespace((GIBaseInfo*)info));
  const gchar * symbol = g_function_info_get_symbol(info);
  GModule *m = g_module_open(shlib, G_MODULE_BIND_LAZY);
  g_module_symbol(m, symbol, &nativeAddress);
  g_module_close(m);

  return nativeAddress;
}

static llvm::Function *
pyg_create_native_call(llvm::Module *module,
                       const llvm::Type *nativeRetvalType,
                       std::vector<const llvm::Type*> nativeTypes,
                       std::vector<llvm::Value*> nativeArgValues,
                       GIFunctionInfo *functionInfo)
{
  std::string symbol(g_function_info_get_symbol(functionInfo));

  // native call
  llvm::FunctionType *nativeFT = llvm::FunctionType::get(nativeRetvalType, nativeTypes, false);
  return llvm::Function::Create(nativeFT, llvm::Function::ExternalLinkage, symbol, module);
}

char *
LLVMCompiler::getFunctionName(GIFunctionInfo *info)
{
  return g_strdup_printf("pygi_wrap_%s_%s",
                         g_base_info_get_namespace((GIBaseInfo*)info),
                         g_base_info_get_name((GIBaseInfo*)info));
}

llvm::Value *
LLVMCompiler::createPyNone()
{
  llvm::Value *retval = new llvm::GlobalVariable(pyObjectPtr,
                                                 true,
                                                 llvm::GlobalValue::ExternalLinkage,
                                                 0, "_Py_NoneStruct");
  // FIXME: Increase reference count
  return retval;
}

PyCFunction
LLVMCompiler::compile(GIFunctionInfo *info)
{
  GICallableInfo *callableInfo = (GICallableInfo*)info;

#ifdef TIMEIT
  struct timeval start_tv;
  gettimeofday(&start_tv, NULL);
#endif
  // llvm
  static int symbolsLoaded = 0;

  if (!symbolsLoaded) {
      llvm::Constant *f;
      pyObjectPtr = llvm::PointerType::getUnqual(llvm::StructType::get(mCtx, NULL, NULL));
      gObjectPtr = llvm::PointerType::getUnqual(llvm::StructType::get(mCtx, NULL, NULL));
      f = mModule->getOrInsertFunction("_PyLong_Check", llvm::Type::getInt32Ty(mCtx), pyObjectPtr, NULL);
      mEE->updateGlobalMapping(llvm::cast<llvm::Function>(f), (void*)&_PyLong_Check);
      f = mModule->getOrInsertFunction("_PyFloat_Check", llvm::Type::getInt32Ty(mCtx), pyObjectPtr, NULL);
      mEE->updateGlobalMapping(llvm::cast<llvm::Function>(f), (void*)&_PyFloat_Check);
      f = mModule->getOrInsertFunction("_PyString_Check", llvm::Type::getInt32Ty(mCtx), pyObjectPtr, NULL);
      mEE->updateGlobalMapping(llvm::cast<llvm::Function>(f), (void*)&_PyString_Check);
      f = mModule->getOrInsertFunction("_PyGObject_Check", llvm::Type::getInt32Ty(mCtx), pyObjectPtr,
                                      llvm::Type::getInt32Ty(mCtx), NULL);
      mEE->updateGlobalMapping(llvm::cast<llvm::Function>(f), (void*)&_PyGObject_Check);
      f = mModule->getOrInsertFunction("_PyGObject_Get", gObjectPtr, pyObjectPtr, NULL);
      mEE->updateGlobalMapping(llvm::cast<llvm::Function>(f), (void*)&_PyGObject_Get);
      f = mModule->getOrInsertFunction("_PyGObject_New", pyObjectPtr, gObjectPtr, NULL);
      mEE->updateGlobalMapping(llvm::cast<llvm::Function>(f), (void*)&_PyGObject_New);
      symbolsLoaded = 1;
  }

  // wrapper
  std::vector<const llvm::Type*> wrapperArgTypes(2, pyObjectPtr);
  llvm::FunctionType *FT = llvm::FunctionType::get(pyObjectPtr, wrapperArgTypes, false);

  char *funcName = this->getFunctionName(info);
  llvm::Function *wrapperFunc = llvm::Function::Create(FT, llvm::Function::ExternalLinkage, funcName, mModule);
  g_free(funcName);

  llvm::BasicBlock *funcBB = llvm::BasicBlock::Create(mCtx, "entry", wrapperFunc);
  Builder.SetInsertPoint(funcBB);

  std::vector<llvm::Value*> wrapperArgValues(2);
  unsigned Idx = 0;
  for (llvm::Function::arg_iterator AI = wrapperFunc->arg_begin(); Idx != wrapperArgTypes.size();
       ++AI, ++Idx) {
    AI->setName("py_arg0");
    wrapperArgValues[Idx] = AI;
  }

  llvm::BasicBlock *block = funcBB;

  // type checking
  // py->native args
  std::vector<llvm::Value*> nativeArgValues;
  std::vector<const llvm::Type*> nativeTypes;
  int n_args = g_callable_info_get_n_args(callableInfo);
  for (int i = 0; i < n_args; ++i) {
    GIArgInfo *argInfo = g_callable_info_get_arg(callableInfo, i);
    GITypeInfo *typeInfo = g_arg_info_get_type(argInfo);
    llvm::Value *value;

    if (n_args == 1) {
      value = wrapperArgValues[1];
    } else {
      // PyTuple_GetItem()
      value = this->tupleGetItem(block, wrapperArgValues[1], i);
    }
    // PyXXX_Check()
    this->typeCheck(callableInfo, argInfo, typeInfo, i, &block, value);

    // PyXXX_FromXXX
    llvm::Value *arg = this->valueAsNative(typeInfo, block, value);
    const llvm::Type *type = this->getTypeFromTypeInfo(typeInfo);
    nativeTypes.push_back(type);
    nativeArgValues.push_back(arg);
    g_base_info_unref((GIBaseInfo*)typeInfo);
    g_base_info_unref((GIBaseInfo*)argInfo);
  }

  // py->native return type
  GIArgInfo *retTypeInfo = g_callable_info_get_return_type(callableInfo);
  const llvm::Type *nativeRetvalType = this->getTypeFromTypeInfo(retTypeInfo);
  llvm::Function * nativeF = pyg_create_native_call(mModule, nativeRetvalType, nativeTypes, nativeArgValues, info);
  mEE->updateGlobalMapping(nativeF, pyg_get_native_address(info));

  const char *retValName;
  if (g_type_info_get_tag(retTypeInfo) == GI_TYPE_TAG_VOID)
     retValName = "";
  else
     retValName = "retval";

  llvm::Value *nativeCallRetval = Builder.CreateCall(nativeF, nativeArgValues.begin(), nativeArgValues.end(), retValName);

  // arg->py conversion
  llvm::Value *retval;
  if (g_type_info_get_tag(retTypeInfo) == GI_TYPE_TAG_VOID) {
    retval = this->createPyNone();
  } else {
    retval = this->valueFromNative(retTypeInfo, block, nativeCallRetval);
  }
  llvm::ReturnInst::Create(mCtx, retval, block);

  g_base_info_unref((GIBaseInfo*)retTypeInfo);

#if DEBUG
  mModule->dump();
#endif

  // JIT it
  PyCFunction f = (PyCFunction)mEE->getPointerToFunction(wrapperFunc);

#if TIMEIT
  struct timeval end_tv;
  gettimeofday(&end_tv, NULL);
  g_print("Compiled: %s in %2.2f ms\n", g_function_info_get_symbol(info),
          (end_tv.tv_usec-start_tv.tv_usec)/1000.0);
#elif DEBUG
  g_print("Compiled: %s\n", g_function_info_get_symbol(info));
#endif

  return f;
}

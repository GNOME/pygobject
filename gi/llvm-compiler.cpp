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
#include "pygi-private.h"
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
#include <llvm/Support/TypeBuilder.h>
#include <llvm/Target/TargetSelect.h>

#define DEBUG 0
#define TIMEIT 1

using namespace pygi;

/*
 * PyGI LLVM runtime functions
 */
// FIXME: replace with o->ob_type == &Py_Type
long _PyLong_Check(PyObject *o) {
  int r =  PyLong_Check(o) || PyInt_Check(o);
  return r;
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

PyObject* _PyGBoxed_New(GType gtype,
                        gpointer boxed) {
  PyObject *py_type = _pygi_type_get_from_g_type(gtype);
  if (py_type == NULL)
    return NULL;
  PyObject *py_boxed = _pygi_boxed_new((PyTypeObject *)py_type, boxed, TRUE);
  Py_DECREF(py_type);
  return py_boxed;
}

gpointer _PyGBoxed_Get(PyObject *object) {
  return pyg_boxed_get(object, void);
}

PY_LONG_LONG _PyLong_AsUnsignedLongLong(PyObject *o) {
    PY_LONG_LONG i = 0;
    i = PyLong_AsUnsignedLongLong(o);
    return i;
}

PyObject *_PyLong_FromUnsignedLongLong(PY_LONG_LONG v) {
    PY_LONG_LONG i = 0;
    PyObject *rv = PyLong_FromUnsignedLongLong(v);
    return rv;
}

/*
 * LLVMCompiler
 *
 */


static llvm::IRBuilder<> Builder(llvm::getGlobalContext());
static llvm::Type* pyObjectPtr = NULL;
static llvm::Type* gObjectPtr = NULL;
static llvm::Type* voidPtr = NULL;
static llvm::Function *_PyGObject_GetFunc = NULL;
static llvm::Function *_PyGObject_NewFunc = NULL;
static llvm::Function *_PyGBoxed_GetFunc = NULL;
static llvm::Function *_PyGBoxed_NewFunc = NULL;
static llvm::Function *_PyLong_CheckFunc = NULL;
static llvm::Function *_PyFloat_CheckFunc = NULL;
static llvm::Function *_PyString_CheckFunc = NULL;
static llvm::Function *_PyGObject_CheckFunc = NULL;
static llvm::Function *_PyLong_AsUnsignedLongLongFunc = NULL;
static llvm::Function *_PyLong_FromUnsignedLongLongFunc = NULL;
static llvm::Value *_PyExc_TypeErrorVar = NULL;

LLVMCompiler::LLVMCompiler(llvm::LLVMContext &ctx) :
  mCtx(ctx)
{
  mModule = new llvm::Module("pygi", mCtx);
  mEE = createExecutionEngine();
  this->loadSymbols();
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
  case GI_TYPE_TAG_BOOLEAN:
    return llvm::Type::getInt1Ty(mCtx);
  case GI_TYPE_TAG_DOUBLE:
    return llvm::Type::getDoubleTy(mCtx);
  case GI_TYPE_TAG_INT8:
  case GI_TYPE_TAG_UINT8:
    return llvm::Type::getInt8Ty(mCtx);
  case GI_TYPE_TAG_LONG:
  case GI_TYPE_TAG_ULONG:
    if (sizeof(long) == 4)
        return llvm::Type::getInt32Ty(mCtx);
    else if (sizeof(long) == 8)
        return llvm::Type::getInt64Ty(mCtx);
    else
        g_assert_not_reached();
  case GI_TYPE_TAG_INT:
  case GI_TYPE_TAG_UINT:
    if (sizeof(int) == 4)
        return llvm::Type::getInt32Ty(mCtx);
    else if (sizeof(int) == 8)
        return llvm::Type::getInt64Ty(mCtx);
    else
        g_assert_not_reached();
  case GI_TYPE_TAG_INT32:
  case GI_TYPE_TAG_UINT32:
    return llvm::Type::getInt32Ty(mCtx);
  case GI_TYPE_TAG_INT64:
  case GI_TYPE_TAG_UINT64:
    return llvm::Type::getInt64Ty(mCtx);
  case GI_TYPE_TAG_ARRAY:
    return llvm::PointerType::getUnqual(llvm::Type::getInt32Ty(mCtx));
  case GI_TYPE_TAG_UTF8:
    return llvm::PointerType::getUnqual(llvm::Type::getInt8Ty(mCtx));
  case GI_TYPE_TAG_INTERFACE: {
    GIBaseInfo *ifaceInfo = g_type_info_get_interface(typeInfo);
    GType type = g_registered_type_info_get_g_type((GIRegisteredTypeInfo *)ifaceInfo);
    if (g_type_is_a(type, G_TYPE_OBJECT)) {
      return gObjectPtr;
    } else if (g_type_is_a(type, G_TYPE_BOXED)) {
      return voidPtr;
    } else if (g_type_is_a(type, G_TYPE_VALUE)) {
      return voidPtr;
    } else if (g_type_is_a(type, G_TYPE_POINTER)) {
      return voidPtr;
    } else {
      g_error("%s: unsupported info tag: %s %d\n", __FUNCTION__, g_type_name(type), g_base_info_get_type(ifaceInfo));
    }
  }
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
  case GI_TYPE_TAG_INTERFACE:
     return "an object";
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

  // FIXME: add "not a, float" to the end
  char *msg = g_strdup_printf("argument %d to %s must be %s",
                              i+1, g_base_info_get_name((GIBaseInfo*)callableInfo),
                              this->formatTypeForException(typeInfo));
  Builder.CreateCall2(f,
                      Builder.CreateLoad(_PyExc_TypeErrorVar),
                      Builder.CreateGlobalStringPtr(msg, "format"));
  g_free(msg);
  Builder.CreateRet(llvm::ConstantExpr::getNullValue(pyObjectPtr));
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
    llvm::Value *v = Builder.CreateCall(_PyFloat_CheckFunc, value, "l");
    llvm::BasicBlock *excBlock = this->createException(callableInfo, argInfo, typeInfo, i, *block);
    this->createIf(block, llvm::ICmpInst::ICMP_EQ, v, llvm::ConstantInt::get(llvm::Type::getInt32Ty(mCtx), 0), excBlock);
    break;
  }
  case GI_TYPE_TAG_INT:
  case GI_TYPE_TAG_INT8:
  case GI_TYPE_TAG_UINT8:
  case GI_TYPE_TAG_INT16:
  case GI_TYPE_TAG_UINT16:
  case GI_TYPE_TAG_INT32:
  case GI_TYPE_TAG_UINT32:
  case GI_TYPE_TAG_INT64:
  case GI_TYPE_TAG_UINT64: {
    llvm::Value *v = Builder.CreateCall(_PyLong_CheckFunc, value);
    llvm::BasicBlock *excBlock = this->createException(callableInfo, argInfo, typeInfo, i, *block);
    this->createIf(block, llvm::ICmpInst::ICMP_EQ, v, llvm::ConstantInt::get(llvm::Type::getInt32Ty(mCtx), 0), excBlock);
    break;
  }
  case GI_TYPE_TAG_UTF8: {
    llvm::Value *v = Builder.CreateCall(_PyString_CheckFunc, value);
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
           llvm::Value *v = Builder.CreateCall2(_PyGObject_CheckFunc, value,
                                                llvm::ConstantInt::get(llvm::Type::getInt32Ty(mCtx), objectType));
           llvm::BasicBlock *excBlock = this->createException(callableInfo, argInfo, typeInfo, i, *block);
           this->createIf(block, llvm::ICmpInst::ICMP_EQ, v, llvm::ConstantInt::get(llvm::Type::getInt32Ty(mCtx), 0), excBlock);
          break;
        }
        case GI_INFO_TYPE_STRUCT: {
          /* FIXME */
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
                            llvm::Value *value)
{
  llvm::Value *retval;
  const llvm::Type *valueType = this->getTypeFromTypeInfo(typeInfo);
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
    retval = Builder.CreateCall(f, value);
    if (retval->getType() != valueType)
        retval = Builder.CreateIntCast(retval, valueType, isSigned, "cast");
    break;
  }
  case GI_TYPE_TAG_INT64: {
    llvm::Constant *f = mModule->getOrInsertFunction("PyLong_AsLongLong",
                                                     llvm::Type::getInt64Ty(mCtx), pyObjectPtr, NULL);
    retval = Builder.CreateCall(f, value);
    if (retval->getType() != valueType)
        retval = Builder.CreateIntCast(retval, valueType, true, "cast");
    break;
  }
  case GI_TYPE_TAG_UINT64: {
    retval = Builder.CreateCall(_PyLong_AsUnsignedLongLongFunc, value);
    if (retval->getType() != valueType)
        retval = Builder.CreateIntCast(retval, valueType, false, "cast");
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
           retval = Builder.CreateCall(_PyGObject_GetFunc, value);
          break;
        }
        case GI_INFO_TYPE_STRUCT: {
           retval = Builder.CreateCall(_PyGBoxed_GetFunc, value);
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
                              llvm::Value *value)
{
  llvm::Value *retval;
  const llvm::Type *valueType = this->getTypeFromTypeInfo(typeInfo);
  bool isSigned = false;
  switch (g_type_info_get_tag(typeInfo)) {
  case GI_TYPE_TAG_DOUBLE: {
    llvm::Constant *f = mModule->getOrInsertFunction("PyFloat_FromDouble",
                                                     pyObjectPtr, llvm::Type::getDoubleTy(mCtx), NULL);
    retval = Builder.CreateCall(f, value);
    break;
  }
  case GI_TYPE_TAG_INT:
  case GI_TYPE_TAG_INT8:
  case GI_TYPE_TAG_INT16:
  case GI_TYPE_TAG_INT32:
  case GI_TYPE_TAG_LONG:
    isSigned = true;
  case GI_TYPE_TAG_BOOLEAN:
  case GI_TYPE_TAG_ULONG:
  case GI_TYPE_TAG_UINT:
  case GI_TYPE_TAG_UINT8:
  case GI_TYPE_TAG_UINT16: {
    llvm::Constant *f = mModule->getOrInsertFunction("PyLong_FromLong",
                                                     pyObjectPtr, llvm::Type::getInt32Ty(mCtx), NULL);
    llvm::Value *casted = Builder.CreateIntCast(value, llvm::Type::getInt32Ty(mCtx), isSigned, "cast");
    retval = Builder.CreateCall(f, casted);
    break;
  }
  case GI_TYPE_TAG_INT64:
    isSigned = true;
  case GI_TYPE_TAG_UINT32: {
    llvm::Constant *f = mModule->getOrInsertFunction("PyLong_FromLongLong",
                                                     pyObjectPtr, llvm::Type::getInt64Ty(mCtx), NULL);
    llvm::Value *casted = Builder.CreateIntCast(value, llvm::Type::getInt64Ty(mCtx), isSigned, "cast");
    retval = Builder.CreateCall(f, casted);
    break;
  }
  case GI_TYPE_TAG_UINT64: {
    llvm::Value *casted = Builder.CreateIntCast(value, llvm::Type::getInt64Ty(mCtx), false, "cast");
    retval = Builder.CreateCall(_PyLong_FromUnsignedLongLongFunc, casted);
    break;
  }
  case GI_TYPE_TAG_UTF8: {
    llvm::Constant *f = mModule->getOrInsertFunction("PyString_FromString",
                                                    pyObjectPtr, llvm::PointerType::getUnqual(llvm::IntegerType::get(mCtx, 8)), NULL);
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
      retval = Builder.CreateCall(_PyGObject_NewFunc, value);
      break;
    }
    case GI_INFO_TYPE_BOXED:
    case GI_INFO_TYPE_UNION:
    case GI_INFO_TYPE_STRUCT: {
      GType type = g_registered_type_info_get_g_type((GIRegisteredTypeInfo *)ifaceInfo);
      if (g_type_is_a(type, G_TYPE_BOXED)) {
        retval = Builder.CreateCall2(_PyGBoxed_NewFunc,
                                     llvm::ConstantInt::get(llvm::Type::getInt32Ty(mCtx), type),
                                     value);
        this->pyIncRef(retval);
      } else {
        g_error("%s: unsupported GType for struct/boxed/union: %s\n", __FUNCTION__, g_type_name(type));
      }
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
LLVMCompiler::tupleGetItem(llvm::Value *value,
                           unsigned int i)
{
  // FIXME: ->ob_item
  static llvm::Constant *f =
      mModule->getOrInsertFunction("PyTuple_GetItem",
                                   pyObjectPtr, pyObjectPtr, llvm::Type::getInt32Ty(mCtx), NULL);
  return Builder.CreateCall2(f, value,
                             llvm::ConstantInt::get(llvm::Type::getInt32Ty(mCtx), i));
}

void *
LLVMCompiler::getNativeAddress(GIFunctionInfo *info)
{
  void * nativeAddress;
  const char * namespace_ = g_base_info_get_namespace((GIBaseInfo*)info);
  moduleMapType::iterator i = mGModules.find(namespace_);
  GModule *module;
  if (i == mGModules.end()) {
    const gchar * shlib = g_irepository_get_shared_library(NULL, namespace_);
    module = g_module_open(shlib, G_MODULE_BIND_LAZY);
    mGModules.insert(std::make_pair(namespace_, module));
  } else {
    module= i->second;
  }

  const gchar * symbol = g_function_info_get_symbol(info);
  g_module_symbol(module, symbol, &nativeAddress);

  return nativeAddress;
}

llvm::Function *
LLVMCompiler::createNativeCall(GIFunctionInfo *functionInfo,
                               const llvm::Type *nativeRetvalType,
                               std::vector<const llvm::Type*> nativeTypes,
                               std::vector<llvm::Value*> nativeArgValues)
{
  std::string symbol(g_function_info_get_symbol(functionInfo));

  llvm::FunctionType *nativeFT = llvm::FunctionType::get(nativeRetvalType, nativeTypes, false);
  return llvm::Function::Create(nativeFT, llvm::Function::ExternalLinkage, symbol, mModule);
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
  return retval;
}

// (PyObject*(ob))->ob_refcnt++
void
LLVMCompiler::pyIncRef(llvm::Value *value)
{
  assert(value->getType() == pyObjectPtr);
  llvm::Value *refCnt = Builder.CreateStructGEP(value, 0, "ob_refcnt");
  Builder.CreateAdd(Builder.CreateLoad(refCnt), llvm::ConstantInt::get(llvm::Type::getInt32Ty(mCtx), 1));
}

void
LLVMCompiler::loadSymbols()
{
  // PyObject = { ssize_t ob_refcnt, struct* ob_type }
  llvm::StructType *pyObject = llvm::StructType::get(mCtx,
                                                     llvm::TypeBuilder<ssize_t, false>::get(mCtx),
                                                     llvm::TypeBuilder<void*, false>::get(mCtx), NULL);
  mModule->addTypeName("PyObject", pyObject);
  pyObjectPtr = llvm::PointerType::getUnqual(pyObject);

  llvm::StructType *gObject = llvm::StructType::get(mCtx, NULL, NULL);
  mModule->addTypeName("GObject", gObject);
  gObjectPtr = llvm::PointerType::getUnqual(gObject);

  const llvm::Type *void_ = llvm::Type::getInt8Ty(mCtx);
  mModule->addTypeName("void", gObject);
  voidPtr = llvm::PointerType::getUnqual(void_);

  const llvm::Type * gType = llvm::Type::getInt32Ty(mCtx);

  _PyLong_CheckFunc =
    llvm::cast<llvm::Function>(mModule->getOrInsertFunction("_PyLong_Check",
                                                            llvm::Type::getInt32Ty(mCtx), pyObjectPtr, NULL));
  mEE->updateGlobalMapping(llvm::cast<llvm::Function>(_PyLong_CheckFunc), (void*)&_PyLong_Check);

  _PyFloat_CheckFunc =
    llvm::cast<llvm::Function>(mModule->getOrInsertFunction("_PyFloat_Check",
                                                            llvm::Type::getInt32Ty(mCtx), pyObjectPtr, NULL));
  mEE->updateGlobalMapping(llvm::cast<llvm::Function>(_PyFloat_CheckFunc), (void*)&_PyFloat_Check);

  _PyString_CheckFunc
    = llvm::cast<llvm::Function>(mModule->getOrInsertFunction("_PyString_Check",
                                                              llvm::Type::getInt32Ty(mCtx), pyObjectPtr, NULL));
  mEE->updateGlobalMapping(llvm::cast<llvm::Function>(_PyString_CheckFunc), (void*)&_PyString_Check);

  _PyGObject_CheckFunc
    = llvm::cast<llvm::Function>(mModule->getOrInsertFunction("_PyGObject_Check",
                                                              llvm::Type::getInt32Ty(mCtx), pyObjectPtr, gType, NULL));
  mEE->updateGlobalMapping(llvm::cast<llvm::Function>(_PyGObject_CheckFunc), (void*)&_PyGObject_Check);

  _PyLong_AsUnsignedLongLongFunc
    = llvm::cast<llvm::Function>(mModule->getOrInsertFunction("PyLong_AsUnsignedLongLong",
                                                              llvm::Type::getInt64Ty(mCtx), pyObjectPtr, NULL));
  mEE->updateGlobalMapping(llvm::cast<llvm::Function>(_PyLong_AsUnsignedLongLongFunc), (void*)&_PyLong_AsUnsignedLongLong);

  _PyLong_FromUnsignedLongLongFunc
    = llvm::cast<llvm::Function>(mModule->getOrInsertFunction("_PyLong_FromUnsignedLongLong",
                                                              pyObjectPtr, llvm::Type::getInt64Ty(mCtx), NULL));
  mEE->updateGlobalMapping(llvm::cast<llvm::Function>(_PyLong_FromUnsignedLongLongFunc), (void*)&_PyLong_FromUnsignedLongLong);

  _PyGObject_GetFunc = llvm::cast<llvm::Function>(mModule->getOrInsertFunction("_PyGObject_Get", gObjectPtr, pyObjectPtr, NULL));
  mEE->updateGlobalMapping(llvm::cast<llvm::Function>(_PyGObject_GetFunc), (void*)&_PyGObject_Get);
  _PyGObject_NewFunc = llvm::cast<llvm::Function>(mModule->getOrInsertFunction("_PyGObject_New", pyObjectPtr, gObjectPtr, NULL));
  mEE->updateGlobalMapping(llvm::cast<llvm::Function>(_PyGObject_NewFunc), (void*)&_PyGObject_New);

  _PyGBoxed_GetFunc = llvm::cast<llvm::Function>(mModule->getOrInsertFunction("_PyGBoxed_Get", voidPtr, pyObjectPtr, NULL));
  mEE->updateGlobalMapping(llvm::cast<llvm::Function>(_PyGBoxed_GetFunc), (void*)&_PyGBoxed_Get);
  _PyGBoxed_NewFunc = llvm::cast<llvm::Function>(mModule->getOrInsertFunction("_PyGBoxed_New", pyObjectPtr, gType, voidPtr, NULL));
  mEE->updateGlobalMapping(llvm::cast<llvm::Function>(_PyGBoxed_NewFunc), (void*)&_PyGBoxed_New);

  _PyExc_TypeErrorVar = new llvm::GlobalVariable(pyObjectPtr, true, llvm::GlobalValue::ExternalLinkage, 0, "PyExc_TypeError");
}

PyCFunction
LLVMCompiler::compile(GIFunctionInfo *info)
{
  GICallableInfo *callableInfo = (GICallableInfo*)info;

#ifdef TIMEIT
  struct timeval start_tv;
  gettimeofday(&start_tv, NULL);
#endif

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
      value = this->tupleGetItem(wrapperArgValues[1], i);
    }
    // PyXXX_Check()
    this->typeCheck(callableInfo, argInfo, typeInfo, i, &block, value);

    // PyXXX_FromXXX
    llvm::Value *arg = this->valueAsNative(typeInfo, value);
    const llvm::Type *type = this->getTypeFromTypeInfo(typeInfo);
    nativeTypes.push_back(type);
    nativeArgValues.push_back(arg);
    g_base_info_unref((GIBaseInfo*)typeInfo);
    g_base_info_unref((GIBaseInfo*)argInfo);
  }

  // py->native return type
  GIArgInfo *retTypeInfo = g_callable_info_get_return_type(callableInfo);
  const llvm::Type *nativeRetvalType = this->getTypeFromTypeInfo(retTypeInfo);
  llvm::Function * nativeF = this->createNativeCall(info, nativeRetvalType, nativeTypes, nativeArgValues);
  mEE->updateGlobalMapping(nativeF, this->getNativeAddress(info));

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
    this->pyIncRef(Builder.CreateLoad(retval));
  } else {
    retval = this->valueFromNative(retTypeInfo, nativeCallRetval);
  }
  Builder.CreateRet(retval);

  g_base_info_unref((GIBaseInfo*)retTypeInfo);

  // JIT it
  //llvm::verifyFunction(*wrapperFunc);
  PyCFunction f = (PyCFunction)mEE->getPointerToFunction(wrapperFunc);

  //printf("PyC: %p %p\n", f, &f);
  //G_BREAKPOINT();
#if TIMEIT
  struct timeval end_tv;
  gettimeofday(&end_tv, NULL);
#endif

#if DEBUG
  mModule->dump();
#endif

#if TIMEIT
  g_print("Compiled: %s in %2.2f ms\n", g_function_info_get_symbol(info),
          (end_tv.tv_usec-start_tv.tv_usec)/1000.0);
#elif DEBUG
  g_print("Compiled: %s\n", g_function_info_get_symbol(info));
#endif

  return f;
}

#include "CIRToCpp.h"

#include "clang/Basic/IdentifierTable.h"
#include "clang/Basic/LangOptions.h"
#include "clang/CIR/Dialect/IR/CIRDialect.h"

#include "mlir/Dialect/EmitC/IR/EmitC.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/Verifier.h"
#include "mlir/Target/Cpp/CppEmitter.h"

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/TargetParser/Triple.h"

#include <string>
#include <vector>

namespace {

bool isCppIdentifier(llvm::StringRef name,
                     clang::IdentifierTable &identifiers) {
  if (name.empty() || !(llvm::isAlpha(name.front()) || name.front() == '_'))
    return false;
  if (!llvm::all_of(name.drop_front(), [](char character) {
        return llvm::isAlnum(character) || character == '_';
      }))
    return false;
  return identifiers.get(name).getTokenID() == clang::tok::identifier;
}

uint64_t naturalAlignment(mlir::Type type) {
  if (auto integer = mlir::dyn_cast<cir::IntType>(type))
    return std::max(1U, integer.getWidth() / 8);
  if (mlir::isa<cir::BoolType>(type))
    return 1;
  if (mlir::isa<cir::SingleType>(type))
    return 4;
  if (mlir::isa<cir::DoubleType, cir::PointerType>(type))
    return 8;
  return 0;
}

static bool hasNonEmptyDictionaries(std::optional<mlir::ArrayAttr> attributes) {
  if (!attributes)
    return false;
  return llvm::any_of(*attributes, [](mlir::Attribute attribute) {
    auto dictionary = mlir::dyn_cast<mlir::DictionaryAttr>(attribute);
    return !dictionary || !dictionary.empty();
  });
}

void createInclude(mlir::OpBuilder &builder, mlir::Location location,
                   llvm::StringRef name) {
  mlir::OperationState state(location,
                             mlir::emitc::IncludeOp::getOperationName());
  state.addAttribute("include", builder.getStringAttr(name));
  state.addAttribute("is_standard_include", builder.getUnitAttr());
  builder.create(state);
}

mlir::FailureOr<mlir::Type> convertType(mlir::Type type,
                                        mlir::Operation *context,
                                        mlir::OpBuilder &builder) {
  if (auto integer = mlir::dyn_cast<cir::IntType>(type)) {
    if (integer.isBitInt() ||
        !llvm::is_contained({8U, 16U, 32U, 64U}, integer.getWidth())) {
      context->emitError("only fundamental 8-, 16-, 32-, and 64-bit CIR "
                         "integer types are supported");
      return mlir::failure();
    }
    mlir::IntegerType::SignednessSemantics signedness =
        integer.isSigned() ? mlir::IntegerType::SignednessSemantics::Signed
                           : mlir::IntegerType::SignednessSemantics::Unsigned;
    return mlir::IntegerType::get(builder.getContext(), integer.getWidth(),
                                  signedness);
  }
  if (mlir::isa<cir::BoolType>(type))
    return builder.getI1Type();
  if (mlir::isa<cir::SingleType>(type))
    return builder.getF32Type();
  if (mlir::isa<cir::DoubleType>(type))
    return builder.getF64Type();
  if (auto pointer = mlir::dyn_cast<cir::PointerType>(type)) {
    if (pointer.getAddrSpace()) {
      context->emitError("non-default pointer address spaces are not "
                         "supported");
      return mlir::failure();
    }
    auto pointee = convertType(pointer.getPointee(), context, builder);
    if (mlir::failed(pointee))
      return mlir::failure();
    return mlir::emitc::PointerType::get(*pointee);
  }
  context->emitError("unsupported CIR type for C++ translation: ") << type;
  return mlir::failure();
}

mlir::FailureOr<mlir::Value>
lookup(mlir::Value value, mlir::Operation *context,
       const llvm::DenseMap<mlir::Value, mlir::Value> &values) {
  auto found = values.find(value);
  if (found == values.end()) {
    context->emitError("value is not produced by a supported CIR operation");
    return mlir::failure();
  }
  return found->second;
}

mlir::FailureOr<mlir::Value>
getLValue(mlir::Value address, mlir::Operation *context,
          mlir::OpBuilder &builder,
          const llvm::DenseMap<mlir::Value, mlir::Value> &values,
          const llvm::DenseMap<mlir::Value, mlir::Value> &localObjects) {
  if (auto found = localObjects.find(address); found != localObjects.end())
    return found->second;
  auto mapped = lookup(address, context, values);
  if (mlir::failed(mapped))
    return mlir::failure();
  if (mlir::isa<mlir::emitc::LValueType>((*mapped).getType()))
    return *mapped;

  auto pointer = mlir::dyn_cast<mlir::emitc::PointerType>((*mapped).getType());
  if (!pointer) {
    context->emitError("memory address did not lower to an EmitC pointer or "
                       "lvalue");
    return mlir::failure();
  }
  mlir::OperationState state(context->getLoc(),
                             mlir::emitc::DereferenceOp::getOperationName());
  state.addOperands(*mapped);
  state.addTypes(mlir::emitc::LValueType::get(pointer.getPointee()));
  return builder.create(state)->getResult(0);
}

mlir::LogicalResult
lowerConstant(cir::ConstantOp constant, mlir::OpBuilder &builder,
              llvm::DenseMap<mlir::Value, mlir::Value> &values) {
  auto type = convertType(constant.getType(), constant, builder);
  if (mlir::failed(type))
    return mlir::failure();

  mlir::OperationState state(constant.getLoc(),
                             mlir::emitc::ConstantOp::getOperationName());
  state.addTypes(*type);
  if (auto integer = constant.getValueAttr<cir::IntAttr>()) {
    state.addAttribute("value",
                       mlir::IntegerAttr::get(*type, integer.getValue()));
  } else if (auto boolean = constant.getValueAttr<cir::BoolAttr>()) {
    state.addAttribute("value", builder.getBoolAttr(boolean.getValue()));
  } else if (auto floating = constant.getValueAttr<cir::FPAttr>()) {
    if (!floating.getValue().isFinite())
      return constant.emitError("non-finite floating-point constants are not "
                                "yet supported");
    state.addAttribute("value",
                       mlir::FloatAttr::get(*type, floating.getValue()));
  } else if (constant.isNullPtr()) {
    state.name = mlir::OperationName(mlir::emitc::LiteralOp::getOperationName(),
                                     builder.getContext());
    state.addAttribute("value", builder.getStringAttr("nullptr"));
  } else {
    return constant.emitError("unsupported CIR constant for C++ translation");
  }
  mlir::Operation *lowered = builder.create(state);
  values[constant.getResult()] = lowered->getResult(0);
  return mlir::success();
}

mlir::LogicalResult
lowerAlloca(cir::AllocaOp alloca, mlir::OpBuilder &builder,
            llvm::DenseMap<mlir::Value, mlir::Value> &values,
            llvm::DenseMap<mlir::Value, mlir::Value> &localObjects) {
  if (alloca.isDynamic() || alloca.getConstant() ||
      alloca.getCleanupDestSlot() || alloca.getAnnotations())
    return alloca.emitError("dynamic, const, cleanup, and annotated allocas "
                            "are not yet supported");
  auto valueType = convertType(alloca.getAllocaType(), alloca, builder);
  if (mlir::failed(valueType))
    return mlir::failure();
  uint64_t alignment = naturalAlignment(alloca.getAllocaType());
  if (alignment == 0 || alloca.getAlignment() > alignment)
    return alloca.emitError("over-aligned allocas are not representable by "
                            "the current EmitC lowering");

  mlir::OperationState state(alloca.getLoc(),
                             mlir::emitc::VariableOp::getOperationName());
  state.addTypes(mlir::emitc::LValueType::get(*valueType));
  state.addAttribute("value",
                     mlir::emitc::OpaqueAttr::get(builder.getContext(), ""));
  mlir::Operation *lowered = builder.create(state);
  mlir::Value localObject = lowered->getResult(0);
  localObjects[alloca.getAddr()] = localObject;

  mlir::OperationState addressState(
      alloca.getLoc(), mlir::emitc::AddressOfOp::getOperationName());
  addressState.addOperands(localObject);
  addressState.addTypes(mlir::emitc::PointerType::get(*valueType));
  values[alloca.getAddr()] = builder.create(addressState)->getResult(0);
  return mlir::success();
}

mlir::LogicalResult
lowerLoad(cir::LoadOp load, mlir::OpBuilder &builder,
          llvm::DenseMap<mlir::Value, mlir::Value> &values,
          const llvm::DenseMap<mlir::Value, mlir::Value> &localObjects) {
  if (load.getIsVolatile() || load.getIsNontemporal() || load.getAlignment() ||
      load.getSyncScope() || load.getMemOrder() || load.getInvariant())
    return load.emitError("specialized loads are not yet supported");
  auto lvalue = getLValue(load.getAddr(), load, builder, values, localObjects);
  auto type = convertType(load.getType(), load, builder);
  if (mlir::failed(lvalue) || mlir::failed(type))
    return mlir::failure();

  mlir::OperationState state(load.getLoc(),
                             mlir::emitc::LoadOp::getOperationName());
  state.addOperands(*lvalue);
  state.addTypes(*type);
  mlir::Operation *lowered = builder.create(state);
  values[load.getResult()] = lowered->getResult(0);
  return mlir::success();
}

mlir::LogicalResult
lowerStore(cir::StoreOp store, mlir::OpBuilder &builder,
           const llvm::DenseMap<mlir::Value, mlir::Value> &values,
           const llvm::DenseMap<mlir::Value, mlir::Value> &localObjects) {
  if (store.getIsVolatile() || store.getIsNontemporal() ||
      store.getAlignment() || store.getSyncScope() || store.getMemOrder())
    return store.emitError("specialized stores are not yet supported");
  auto value = lookup(store.getValue(), store, values);
  auto lvalue =
      getLValue(store.getAddr(), store, builder, values, localObjects);
  if (mlir::failed(value) || mlir::failed(lvalue))
    return mlir::failure();

  mlir::OperationState state(store.getLoc(),
                             mlir::emitc::AssignOp::getOperationName());
  state.addOperands({*lvalue, *value});
  builder.create(state);
  return mlir::success();
}

mlir::LogicalResult
lowerReturn(cir::ReturnOp returnOp, mlir::OpBuilder &builder,
            const llvm::DenseMap<mlir::Value, mlir::Value> &values) {
  mlir::OperationState state(returnOp.getLoc(),
                             mlir::emitc::ReturnOp::getOperationName());
  if (returnOp.hasOperand()) {
    auto value = lookup(returnOp.getOperand(0), returnOp, values);
    if (mlir::failed(value))
      return mlir::failure();
    state.addOperands(*value);
  }
  builder.create(state);
  return mlir::success();
}

mlir::LogicalResult lowerFunction(cir::FuncOp function,
                                  mlir::OpBuilder &builder,
                                  clang::IdentifierTable &identifiers) {
  cir::FuncType sourceType = function.getFunctionType();
  if (!isCppIdentifier(function.getSymName(), identifiers))
    return function.emitError("function name is not a valid C++ identifier");
  if (sourceType.isVarArg() || function.getNoProto())
    return function.emitError("variadic and no-prototype functions are not "
                              "yet supported");
  if (function.getCallingConv() != cir::CallingConv::C)
    return function.emitError("only the C calling convention is supported");
  if (function.getLinkage() != cir::GlobalLinkageKind::ExternalLinkage)
    return function.emitError("only external function linkage is supported");
  if (function.getGlobalVisibility() != cir::VisibilityKind::Default ||
      function.getDsoLocal() || function.getSymVisibility() ||
      function.getComdat() || hasNonEmptyDictionaries(function.getArgAttrs()) ||
      hasNonEmptyDictionaries(function.getResAttrs()) ||
      function.getSideEffect() || function.getGlobalCtorPriority() ||
      function.getGlobalDtorPriority() || function.getFuncInfo())
    return function.emitError("function has visibility, ABI, or metadata "
                              "attributes that are not yet supported");
  if (function.getBuiltin() || function.getCoroutine() ||
      function.getLambda() || function.getInlineKind() ||
      function.getAliasee() || function.getPersonality() ||
      function.getAnnotations())
    return function.emitError("function has attributes that are not yet "
                              "supported");

  llvm::SmallVector<mlir::Type> inputs;
  for (mlir::Type input : sourceType.getInputs()) {
    auto converted = convertType(input, function, builder);
    if (mlir::failed(converted))
      return mlir::failure();
    inputs.push_back(*converted);
  }
  llvm::SmallVector<mlir::Type> results;
  if (!sourceType.hasVoidReturn()) {
    auto converted = convertType(sourceType.getReturnType(), function, builder);
    if (mlir::failed(converted))
      return mlir::failure();
    results.push_back(*converted);
  }

  mlir::OperationState state(function.getLoc(),
                             mlir::emitc::FuncOp::getOperationName());
  state.addAttribute(mlir::SymbolTable::getSymbolAttrName(),
                     builder.getStringAttr(function.getSymName()));
  state.addAttribute(
      "function_type",
      mlir::TypeAttr::get(builder.getFunctionType(inputs, results)));
  state.addRegion();
  auto targetFunction = mlir::cast<mlir::emitc::FuncOp>(builder.create(state));
  if (function.isDeclaration())
    return mlir::success();
  if (!llvm::hasSingleElement(function.getBody()))
    return function.emitError("only single-block functions are supported");

  llvm::DenseMap<mlir::Value, mlir::Value> values;
  llvm::DenseMap<mlir::Value, mlir::Value> localObjects;

  mlir::OpBuilder::InsertionGuard guard(builder);
  llvm::SmallVector<mlir::Location> argumentLocations(inputs.size(),
                                                      function.getLoc());
  mlir::Block *targetBlock = builder.createBlock(&targetFunction.getBody(), {},
                                                 inputs, argumentLocations);
  mlir::Block &sourceBlock = function.getBody().front();
  for (auto [source, target] :
       llvm::zip_equal(sourceBlock.getArguments(), targetBlock->getArguments()))
    values[source] = target;

  for (mlir::Operation &operation : sourceBlock.getOperations()) {
    if (auto constant = mlir::dyn_cast<cir::ConstantOp>(operation)) {
      if (mlir::failed(lowerConstant(constant, builder, values)))
        return mlir::failure();
    } else if (auto alloca = mlir::dyn_cast<cir::AllocaOp>(operation)) {
      if (mlir::failed(lowerAlloca(alloca, builder, values, localObjects)))
        return mlir::failure();
    } else if (auto store = mlir::dyn_cast<cir::StoreOp>(operation)) {
      if (mlir::failed(lowerStore(store, builder, values, localObjects)))
        return mlir::failure();
    } else if (auto load = mlir::dyn_cast<cir::LoadOp>(operation)) {
      if (mlir::failed(lowerLoad(load, builder, values, localObjects)))
        return mlir::failure();
    } else if (auto returnOp = mlir::dyn_cast<cir::ReturnOp>(operation)) {
      if (mlir::failed(lowerReturn(returnOp, builder, values)))
        return mlir::failure();
    } else {
      return operation.emitError("unsupported operation for C++ translation");
    }
  }
  return mlir::success();
}

mlir::FailureOr<mlir::OwningOpRef<mlir::ModuleOp>>
lowerToEmitC(mlir::ModuleOp sourceModule, clang::LangStandard::Kind standard) {
  clang::LangOptions languageOptions;
  std::vector<std::string> includes;
  clang::LangOptions::setLangDefaults(languageOptions, clang::Language::CXX,
                                      llvm::Triple(), includes, standard);
  languageOptions.CXXOperatorNames = true;
  clang::IdentifierTable identifiers(languageOptions);
  mlir::OpBuilder builder(sourceModule.getContext());
  mlir::OwningOpRef<mlir::ModuleOp> targetModule =
      mlir::ModuleOp::create(sourceModule.getLoc());
  builder.setInsertionPointToStart(targetModule->getBody());
  createInclude(builder, sourceModule.getLoc(), "stdint.h");
  builder.setInsertionPointToEnd(targetModule->getBody());

  for (mlir::Operation &operation : sourceModule.getBody()->getOperations()) {
    auto function = mlir::dyn_cast<cir::FuncOp>(operation);
    if (!function) {
      operation.emitError("unsupported top-level operation for C++ "
                          "translation");
      return mlir::failure();
    }
    if (mlir::failed(lowerFunction(function, builder, identifiers)))
      return mlir::failure();
  }

  if (mlir::failed(mlir::verify(*targetModule)))
    return mlir::failure();
  return std::move(targetModule);
}

} // namespace

mlir::LogicalResult circc::translateToCpp(mlir::ModuleOp sourceModule,
                                          llvm::raw_ostream &output,
                                          clang::LangStandard::Kind standard) {
  auto emitCModule = lowerToEmitC(sourceModule, standard);
  if (mlir::failed(emitCModule))
    return mlir::failure();
  return mlir::emitc::translateToCpp(**emitCModule, output);
}

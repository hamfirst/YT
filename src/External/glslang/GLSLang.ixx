
module;

#include <glslang/Public/ShaderLang.h>
#include <glslang/Public/ResourceLimits.h>
#include <glslang/SPIRV/GlslangToSpv.h>

export module YT:GLSLang;

// C interface and C-style types from ShaderLang.h / ResourceLimits.h.
export using ::ShInitialize;
export using ::ShFinalize;
export using ::EShLanguage;
export using ::EShLanguageMask;
export using ::EShExecutable;
export using ::EShOptimizationLevel;
export using ::EShTextureSamplerTransformMode;
export using ::EShMessages;
export using ::EShReflectionOptions;
export using ::ShBinding;
export using ::ShBindingTable;
export using ::ShHandle;
export using ::ShConstructCompiler;
export using ::ShConstructLinker;
export using ::ShConstructUniformMap;
export using ::ShDestruct;
export using ::ShCompile;
export using ::ShLinkExt;
export using ::ShSetEncryptionMethod;
export using ::ShGetInfoLog;
export using ::ShGetExecutable;
export using ::ShSetVirtualAttributeBindings;
export using ::ShSetFixedAttributeBindings;
export using ::ShExcludeAttributes;
export using ::ShGetUniformLocation;
export using ::GetResources;
export using ::GetDefaultResources;
export using ::GetDefaultTBuiltInResourceString;
export using ::DecodeResourceLimits;

// Public C++ API from ShaderLang.h and GlslangToSpv.h.
export namespace glslang {
  using ::glslang::TType;
  using ::glslang::EShSource;
  using ::glslang::EShClient;
  using ::glslang::EShTargetLanguage;
  using ::glslang::EShTargetClientVersion;
  using ::glslang::EshTargetClientVersion;
  using ::glslang::EShTargetLanguageVersion;
  using ::glslang::TLayoutPacking;
  using ::glslang::TInputLanguage;
  using ::glslang::TClient;
  using ::glslang::TTarget;
  using ::glslang::TEnvironment;
  using ::glslang::StageName;
  using ::glslang::Version;
  using ::glslang::GetVersion;
  using ::glslang::GetEsslVersionString;
  using ::glslang::GetGlslVersionString;
  using ::glslang::GetKhronosToolId;
  using ::glslang::TIntermediate;
  using ::glslang::TProgram;
  using ::glslang::TPoolAllocator;
  using ::glslang::TIoMapResolver;
  using ::glslang::InitializeProcess;
  using ::glslang::FinalizeProcess;
  using ::glslang::TResourceType;
  using ::glslang::TBlockStorageClass;
  using ::glslang::TShader;
  using ::glslang::TObjectReflection;
  using ::glslang::TIoMapper;
  using ::glslang::GetGlslIoMapper;
  using ::glslang::SpvOptions;
  using ::glslang::GetSpirvVersion;
  using ::glslang::GetSpirvGeneratorVersion;
  using ::glslang::GlslangToSpv;
  using ::glslang::OutputSpvBin;
  using ::glslang::OutputSpvHex;
}

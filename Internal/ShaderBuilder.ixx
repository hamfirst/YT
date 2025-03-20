
module;

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>

#include <glslang/Public/ShaderLang.h>
#include <glslang/Public/ResourceLimits.h>
#include <glslang/SPIRV/GlslangToSpv.h>

export module YT:ShaderBuilder;

import :Types;

namespace YT
{
	export class ShaderBuilder
	{
	public:
		ShaderBuilder()
		{
			glslang::InitializeProcess();
		}

		~ShaderBuilder()
		{
			glslang::FinalizeProcess();
		}

		static EShLanguage FindLanguage(const vk::ShaderStageFlagBits shader_type)
		{
			switch (shader_type)
			{
			case vk::ShaderStageFlagBits::eVertex:
				return EShLangVertex;
			case vk::ShaderStageFlagBits::eTessellationControl:
				return EShLangTessControl;
			case vk::ShaderStageFlagBits::eTessellationEvaluation:
				return EShLangTessEvaluation;
			case vk::ShaderStageFlagBits::eGeometry:
				return EShLangGeometry;
			case vk::ShaderStageFlagBits::eFragment:
				return EShLangFragment;
			case vk::ShaderStageFlagBits::eCompute:
				return EShLangCompute;
			default:
				return EShLangVertex;
			}
		}

		static bool GLSLtoSPV(const vk::ShaderStageFlagBits shader_type, const char * shader_code,
		                      std::vector<unsigned int> & spirv)
		{
			EShLanguage stage = FindLanguage(shader_type);
			glslang::TShader shader(stage);
			glslang::TProgram program;
			const char * shaderStrings[1];

			// Enable SPIR-V and Vulkan rules when parsing GLSL
			EShMessages messages = (EShMessages)(EShMsgSpvRules | EShMsgVulkanRules);

			shaderStrings[0] = shader_code;
			shader.setStrings(shaderStrings, 1);

			if (!shader.parse(GetDefaultResources(), 100, false, messages))
			{
				puts(shader.getInfoLog());
				puts(shader.getInfoDebugLog());
				return false; // something didn't work
			}

			program.addShader(&shader);

			if (!program.link(messages))
			{
				puts(shader.getInfoLog());
				puts(shader.getInfoDebugLog());
				fflush(stdout);
				return false;
			}

			glslang::GlslangToSpv(*program.getIntermediate(stage), spirv);
			return true;
		}
	};
}

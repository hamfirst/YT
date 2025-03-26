
module;

#include <string>
#include <ranges>

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>

#include <glslang/Public/ShaderLang.h>
#include <glslang/Public/ResourceLimits.h>
#include <glslang/SPIRV/GlslangToSpv.h>

export module YT:ShaderBuilder;

import :Types;

namespace YT
{
	export class ShaderBuilder final
	{
	public:
		ShaderBuilder() noexcept
		{
			glslang::InitializeProcess();
		}

		~ShaderBuilder()
		{
			glslang::FinalizeProcess();
		}

		void SetInclude(const StringView & include_name, const StringView & include_text) noexcept
		{
			m_Includes.emplace(include_name, include_text);
		}

		bool BuildShader(const vk::ShaderStageFlagBits shader_type, const StringView & file_name,
			const StringView & shader_code, std::vector<unsigned int> & spirv)
		{
		String result, error;
			Set<String> already_included;
			if (!ReplaceIncludes(shader_code, file_name, already_included, result, error))
			{
				FatalPrint("{}", error);
				return false;
			}

			for (size_t pos = result.find("\t"); pos != std::string::npos; pos = result.find("\t", pos + 1))
			{
				result.replace(pos, 1, "  ");
			}

			return GLSLtoSPV(shader_type, result.data(), spirv);
		}

		static EShLanguage FindLanguage(const vk::ShaderStageFlagBits shader_type) noexcept
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
			case vk::ShaderStageFlagBits::eMeshEXT:
				return EShLangMesh;
			default:
				return EShLangVertex;
			}
		}

	private:

		static StringView TrimStart(const StringView & s)
		{
			for (auto & c : s)
			{
				if (!std::isspace(c))
				{
					return StringView{ &c, s.end() };
				}
			}

			return {};
		}

		static StringView TrimEnd(const StringView & s)
		{
			for (auto itr = s.rbegin(); itr != s.rend(); ++itr)
			{
				if (!std::isspace(*itr))
				{
					return StringView{ s.begin(), &*itr };
				}
			}
			return {};
		}

		bool ReplaceIncludes(const StringView & shader_code, const StringView & file_name,
			Set<String> & already_included, String & result, String & error)
		{
			constexpr StringView delim = "\n";
			constexpr StringView include_directive = "#include";

			int line_number = 1;
			for (const auto line : std::views::split(shader_code, delim))
			{

				StringView line_view = StringView(line.begin(), line.end());

				if (line_view.starts_with(include_directive))
				{
					StringView include(TrimStart(StringView(line_view.begin() + include_directive.size(), line_view.end())));
					if (!include.starts_with("\""))
					{
						error = std::format("error: {}:{} invalid include directive", file_name, line_number);
						return false;
					}

					const char * include_start = include.data() + 1;
					const char * include_end = include_start + include.size();
					const char * include_char = include_start;

					bool is_escaping = false;
					bool found_end = false;
					while (include_char < include_end)
					{
						if (*include_char == '"')
						{
							if (!is_escaping)
							{
								include_end = include_char;
								found_end = true;
								break;
							}

							is_escaping = false;
						}
						else if (*include_char == '\\')
						{
							is_escaping = !is_escaping;
						}
						else
						{
							is_escaping = false;
						}

						include_char++;
					}

					if (!found_end)
					{
						error = std::format("error: {}:{} invalid include directive", file_name, line_number);
						return false;
					}

					String include_name(include_start, include_char);
					StringView remainder(include_char + 1, line.end());

					if (auto itr = m_Includes.find(include_name); itr != m_Includes.end())
					{
						if (already_included.contains(include_name))
						{
							continue;
						}

						already_included.insert(include_name);

						String include_code;
						if (!ReplaceIncludes(itr->second, include_name, already_included, include_code, error))
						{
							return false;
						}

						VerbosePrint("Include code {}\n", include_code);

						result.append(include_code);
						result.append(remainder);

						result.append(Format("#line {}\n", line_number));
					}
					else
					{
						error = Format("error: {}:{} unknown include file \"{}\"", file_name, line_number, include_name);
						return false;
					}
				}
				else
				{
					result.append(line_view);
				}

				result.append(delim);
				line_number++;
			}

			return true;
		}

		static bool GLSLtoSPV(const vk::ShaderStageFlagBits shader_type, const char * shader_code,
		                      std::vector<unsigned int> & spirv)
		{
			VerbosePrint("Compiling shader:\n{}", shader_code);

			EShLanguage stage = FindLanguage(shader_type);
			glslang::TShader shader(stage);
			glslang::TProgram program;
			const char * shader_strings[1];

			// Enable SPIR-V and Vulkan rules when parsing GLSL
			EShMessages messages = (EShMessages)(EShMsgSpvRules | EShMsgVulkanRules);

			shader_strings[0] = shader_code;
			shader.setStrings(shader_strings, 1);

			if (!shader.parse(GetDefaultResources(), 100, false, messages))
			{
				FatalPrint("{}", shader.getInfoLog());
				FatalPrint("{}", shader.getInfoDebugLog());
				return false; // something didn't work
			}

			program.addShader(&shader);

			if (!program.link(messages))
			{
				FatalPrint("{}", shader.getInfoLog());
				FatalPrint("{}", shader.getInfoDebugLog());
				return false;
			}

			glslang::GlslangToSpv(*program.getIntermediate(stage), spirv);
			return true;
		}

	private:

		Map<String, String> m_Includes;
	};
}

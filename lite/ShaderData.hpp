#pragma once

#include "BasicIO.hpp"
#include "Console.hpp"
#include "D3DInfo.hpp"
#include "PathInfo.hpp"

namespace lite
{
  // Possible shader types.
  enum ShaderType
  {
    PixelShader,
    VertexShader,
    ShaderTypeCount
  };

  // Stores bytecode and D3D shader object.
  class ShaderData
  {
  private: // data

    // Bytecode as a blob of data.
    BlobHandle bytecode;

    // Name of the shader.
    string name;

    // Handle to the shader.
    UnknownHandle shader;

    // Type of the shader.
    ShaderType type;

  public:

    const BlobHandle& Bytecode() const { return bytecode; }

  public: // methods

    ShaderData() = default;

    ShaderData(ShaderData&& b) :
      bytecode(move(b.bytecode)),
      name(move(b.name)),
      shader(move(b.shader)),
      type(b.type)
    {
    }

    // Creates the shader data given the name of the shader.
    ShaderData(const string& name_) :
      name(name_)
    {
      // Read the code.
      string fileString = Read(config::Shaders + name + ".hlsl");
      if (fileString.empty())
      {
        Warn("Shader load failed for " + config::Shaders + name + ".hlsl");
        return;
      }

      bool result = true;

      // Compile shader corresponding to its type.
      switch (name[name.size() - 2]) // the 'p' in '.ps' or 'v' in '.vs'
      {
      case 'p':
      case 'P':
        result = Compile(PixelShader, fileString);
        break;
      case 'v':
      case 'V':
        result = Compile(VertexShader, fileString);
        break;
      default:
        Warn("Unknown shader type: " + name);
        break;
      }

      if (!result) return;

      CreateD3DShader(D3DInfo::CurrentInstance()->Device);
    }

    ~ShaderData() = default;

    ShaderData& operator=(ShaderData&& b)
    {
      bytecode = move(b.bytecode);
      name = move(b.name);
      shader = move(b.shader);
      type = b.type;

      return *this;
    }

    // Casts this shader to a pixel shader pointer.
    //  Returns null if the shader is not loaded or
    //  the shader is not a pixel shader.
    ID3D11PixelShader* GetPixelShader() const
    {
      if (!shader) return nullptr;
      if (type != PixelShader) return nullptr;
      return static_cast<ID3D11PixelShader*>(shader.Get());
    }

    // Casts this shader to a vertex shader pointer.
    //  Returns null if the shader is not loaded or
    //  the shader is not a vertex shader.
    ID3D11VertexShader* GetVertexShader() const
    {
      if (!shader) return nullptr;
      if (type != VertexShader) return nullptr;
      return static_cast<ID3D11VertexShader*>(shader.Get());
    }

    // Whether the shader successfully loaded.
    bool IsLoaded() const
    {
      return bool(shader);
    }

  private: // methods
    
    // Compiles a particular shader type.
    bool Compile(ShaderType type, const string& str)
    {
      // Set up the compilation flags.
      DWORD shaderFlags = D3DCOMPILE_ENABLE_STRICTNESS | (DebugMode ? D3DCOMPILE_DEBUG : 0);

      unsigned typeIndex = unsigned(type);

      // Arrays for indexing the entry point and shader model tag.
      static const char* entryPoints [] = { "PS", "VS" };
      static const char* shaderModels [] = { "ps_4_0", "vs_4_0" };

      BlobHandle errorBlob;
      HRESULT hr = D3DCompile(
        str.data(),               // source string
        str.size(),               // source length
        name.c_str(),             // source name
        nullptr,                  // defines pointer
        nullptr,                  // ID3DInclude pointer
        entryPoints[typeIndex],   // entry point
        shaderModels[typeIndex],  // shader model target
        shaderFlags,              // compilation flags 1
        0,                        // compilation flags 2
        bytecode,                 // output bytecode blob
        errorBlob);               // output error blob

      if (FAILED(hr))
      {
        if (errorBlob)
        {
          Warn("Compilation failed for " + name + ": " + (char*) errorBlob->GetBufferPointer());
        }
        else
        {
          Warn("Shader load failed and no error information was provided for: " + name);
        }

        return false;
      }

      this->type = type;

      return true;
    }

    // Creates the D3D shader object.
    void CreateD3DShader(DeviceHandle& device)
    {
      HRESULT hr = E_FAIL;

      IUnknown** address = shader.Address();
      LPVOID code = bytecode->GetBufferPointer();
      SIZE_T size = bytecode->GetBufferSize();

      switch (type)
      {
      case PixelShader:
        hr = device->CreatePixelShader(code, size, 0, reinterpret_cast<ID3D11PixelShader**>(address));
        break;
      case VertexShader:
        hr = device->CreateVertexShader(code, size, 0, reinterpret_cast<ID3D11VertexShader**>(address));
        break;
      default:
        Warn("Shader type unknown: " << type);
        break;
      }

      WarnIf(FAILED(hr), "Create*Shader failed for " + name);
    }

    // Reads a shader into memory.
    string Read(const string& path)
    {
      // Read the entire file.
      string fileString;
      if (!ReadEntireFile(path, fileString))
      {
        Warn("Unable to read shader " + path);
        return string();
      }

      // Parse for #includes and read the included files.
      if (!ReadIncludes(PathInfo(path).DirectoryPath(), fileString))
      {
        Warn("Unable to read includes for shader " + path);
        return string();
      }

      return move(fileString);
    }

    // Reads #include directives and moves the contents of such files into the final string.
    bool ReadIncludes(const string& directory, string& fileString)
    {
      // Find the first include directive.
      auto includeDirective = fileString.find("#include");

      // While we have a valid include directive:
      while (includeDirective != fileString.npos)
      {
        // Set up the path to the included file.
        auto includeFileStart = fileString.find('"', includeDirective) + 1;
        auto includeFileEnd = fileString.find('"', includeFileStart);
        string includeFile = fileString.substr(includeFileStart, includeFileEnd - includeFileStart);
        string includeFilePath = directory + includeFile;

        // Read the included file.
        string includeContents;
        if (!ReadEntireFile(includeFilePath, includeContents))
        {
          Warn("Failed to read file " + includeFilePath);
          return false;
        }

        // Recursively read any includes this included file might have.
        if (!ReadIncludes(PathInfo(includeFilePath).DirectoryPath(), includeContents))
        {
          Warn("Failed to read shader includes for " + includeFilePath);
          return false;
        }

        // Replace the #include line with the actual contents of the included file.
        fileString.replace(includeDirective, includeFileEnd - includeDirective + 1, includeContents);

        // Find the next include directive.
        includeDirective = fileString.find("#include");
      }

      return true;
    }
  };
} // namespace lite
#pragma once

#include <algorithm>
#include "Essentials.hpp"
#include <iterator>
#include "Logging.hpp"
#include "WindowsInclude.h"

namespace lite
{
  enum class PathType
  {
    Invalid,
    Directory,
    File
  };

  PathType GetPathType(const string& path);
  string GetProgramDirectory();
  string GetWorkingDirectory();
  bool SetWorkingDirectory(const string& dir);

  class PathInfo
  {
  private: // data

    string baseFilename;
    vector<string> directories;
    string directoryPath;
    string fileExtension;
    string filename;
    vector<string> files;
    string fullPath;
    bool valid;

  public: // properties

    // Base part of the filename with the directory or extension.
    const string& BaseFilename() const { return baseFilename; }

    // Child directories of this directory.
    const vector<string>& Directories() const { return directories; }

    // Path to the directory of this file or directory.
    const string& DirectoryPath() const { return directoryPath; };

    // Extension of the file.
    const string& FileExtension() const { return fileExtension; }

    // Filename including the extension.
    const string& Filename() const { return filename; }

    // Files within this directory.
    const vector<string>& Files() const { return files; }

    // Full path to this file or directory.
    const string& FullPath() const { return fullPath; }

    // Whether this is a valid file or directory.
    const bool& Valid() const { return valid; }

  public: // methods

    // Initializes path info properties with a path to a directory or file.
    explicit PathInfo(string path = GetWorkingDirectory()) :
      fullPath(move(path))
    {
      PathType type = GetPathType(fullPath);

      if (type == PathType::Directory)
      {
        InitializeAsDirectory();
      }
      else if (type == PathType::File)
      {
        InitializeAsFile();
      }
      else
      {
        Warn("Unknown path type for " + fullPath);
      }
    }

    vector<string> FilesWithExtension(const string& extension)
    {
      string dotExtension = "." + extension;
      vector<string> result;

      copy_if(
        files.begin(),                  // input begin
        files.end(),                    // input end
        inserter(result, result.end()), // output iterator (insert_iterator used to push back to 'result')
        [&](const string& file)         // predicate returning whether the string should be copied
        {
          return file.substr(file.rfind('.')) == dotExtension;
        });

      return move(result);
    }

  private: // methods

    void InitializeAsFile()
    {
      // Find the last slash in the file name.
      auto lastSlash = fullPath.find_last_of("/\\");

      // Find the dot at the beginning of the extension.
      auto dot = fullPath.rfind('.');

      // Dissect the file name into its parts.
      baseFilename = fullPath.substr(lastSlash + 1, dot - (lastSlash + 1));
      directoryPath = fullPath.substr(0U, lastSlash + 1);
      fileExtension = fullPath.substr(dot + 1, fullPath.size() - (dot + 1));
      filename = baseFilename + "." + fileExtension;

      valid = true;
    }

    void InitializeAsDirectory()
    {
      WIN32_FIND_DATA fileFindData;

      // Find the first file or subdirectory in the current directory. http://msdn.microsoft.com/en-us/library/windows/desktop/aa364418%28v=vs.85%29.aspx
      auto searchPattern = fullPath + "\\*.*";
      auto fileHandle = FindFirstFileA(searchPattern.c_str(), &fileFindData);
      if (fileHandle == INVALID_HANDLE_VALUE) return;

      // For each file or subdirectory:
      do
      {
        // Ignore the "current directory" subdirectory.
        if (fileFindData.cFileName[0] == '.') continue;

        // Create the full path to the current file or subdirectory.
        auto filePath = fullPath + "\\" + fileFindData.cFileName;

        if (fileFindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
          // Save the path as a subdirectory name.
          directories.push_back(move(filePath));
        }
        else
        {
          // Save the path as a file name.
          files.push_back(move(filePath));
        }
      } while (FindNextFile(fileHandle, &fileFindData) == TRUE);
      //       ^^^^^^^^^^^^ Continue to the next file. http://msdn.microsoft.com/en-us/library/windows/desktop/aa364428%28v=vs.85%29.aspx

      // Clean up.
      FindClose(fileHandle);

      auto dwError = GetLastError();
      if (dwError != ERROR_NO_MORE_FILES)
      {
        Warn("Directory error: " + to_string(dwError));
        return;
      }

      directoryPath = fullPath;
      valid = true;
    }
  };

  inline PathType GetPathType(const string& path)
  {
    WIN32_FIND_DATAA fileFindData;

    // Try to find as a file.
    auto fileHandle = FindFirstFileA(path.c_str(), &fileFindData);
    if (fileHandle == INVALID_HANDLE_VALUE)
    {
      // Try to find as a directory.
      string searchPattern = path + "\\*.*";
      fileHandle = FindFirstFileA(searchPattern.c_str(), &fileFindData);

      if (fileHandle == INVALID_HANDLE_VALUE) return PathType::Invalid;
    }

    // Check whether the file is a directory.
    PathType type;
    if (fileFindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
    {
      type = PathType::Directory;
    }
    else
    {
      type = PathType::File;
    }

    FindClose(fileHandle);

    return type;
  }

  inline string GetProgramDirectory()
  {
    // Get the command line as argc and argv parameters. http://msdn.microsoft.com/en-us/library/windows/desktop/bb776391%28v=vs.85%29.aspx
    int argc;
    auto wargv = CommandLineToArgvW(GetCommandLineW(), &argc);
    WarnIf(argc == 0, "Arg count for command line could not be retrieved from GetCommandLineW");

    // Convert the first command from wide-character string to multi-byte string.
    string executablePath = WideCharToMultibyte(wstring(wargv[0]));

    auto pathInfo = PathInfo(executablePath);
    return pathInfo.DirectoryPath();
  }

  inline string GetWorkingDirectory()
  {
    // Create a buffer to store the current directory.
    static const size_t bufferLength = 1024;
    char buffer[bufferLength];

    // Get the current directory. http://msdn.microsoft.com/en-us/library/windows/desktop/aa364934%28v=vs.85%29.aspx
    BOOL bresult = GetCurrentDirectoryA(bufferLength, buffer);
    if (bresult == FALSE) return string();
    else return string(buffer) + "/";
  }

  inline bool SetWorkingDirectory(const string& dir)
  {
    return SetCurrentDirectoryA(dir.c_str()) == TRUE;
  }

} // namespace lite
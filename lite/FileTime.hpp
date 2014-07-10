#pragma once

#include "Essentials.hpp"
#include "WindowsInclude.h"

namespace lite
{
  class FileTime
  {
  public: // types

    enum Comparison
    {
      Earlier = -1,
      Equal = 0,
      Later = 1
    };

  private: // data

    FILETIME creation = { { 0 } };
    FILETIME lastAccess = { { 0 } };
    FILETIME lastWrite = { { 0 } };

  public: // methods

    FileTime() = default;

    FileTime(const string& name)
    {
      // Open the file.
      OFSTRUCT openFileInfo;
      HFILE hFile = OpenFile(name.c_str(), &openFileInfo, OF_READ);
      if (hFile == HFILE_ERROR) return;

      // Retrieve the file times for the file.
      GetFileTime((HANDLE) hFile, &creation, &lastAccess, &lastWrite);
      CloseHandle((HANDLE) hFile);
    }

    // Whether the passed-in file time is earlier, equal, or later.
    Comparison CompareTo(const FileTime& fileTime) const
    {
      return Comparison(CompareFileTime(&fileTime.lastWrite, &lastWrite));
    }

    explicit operator bool() const
    {
      return lastWrite.dwHighDateTime && lastWrite.dwLowDateTime;
    }
  };
} // namespace lite
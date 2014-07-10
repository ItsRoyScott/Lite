#pragma once

#include "Essentials.hpp"
#include <fstream>

namespace lite
{
  namespace detail
  {
    template <class Container>
    bool ReadEntireFile(ifstream& file, Container& vec)
    {
      if (file.is_open() == false) { return false; }

      // Seek to the end of the file to the get the size of the file.
      file.seekg(0, std::ios_base::end);
      auto size = file.tellg();
      file.seekg(0, std::ios_base::beg);

      // Read the file into the string.
      vec.resize(size_t(size));
      file.read((char*) &vec[0], size);

      return true;
    }
  } // namespace detail

  // Reads an entire file as a string.
  inline bool ReadEntireFile(const string& name, string& str)
  {
    auto file = ifstream(name);
    if (!detail::ReadEntireFile(file, str)) return false;

    // Remove all null characters read from the end of the file.
    auto lastNonNull = str.find_last_not_of('\0');
    if (lastNonNull != str.npos)
    {
      str.erase(lastNonNull + 1);
    }

    return true;
  }

  // Reads an entire file as binary.
  inline bool ReadEntireFile(const string& name, vector<unsigned char>& str)
  {
    auto file = ifstream(name, ifstream::binary);
    return detail::ReadEntireFile(file, str);
  }
} // namespace lite
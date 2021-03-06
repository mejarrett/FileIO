/*
 * File:   FileIO.h
 * Author: kjell/weberr13
 *
 * https://github.com/weberr13/FileIO
 * Created on August 15, 2013, 2:28 PM
 */

#pragma once
#include <string>
#include <utility>
#include <vector>
#include <fstream>
#include <dirent.h>
#include <sys/fsuid.h>
#include <unistd.h>
#include <fcntl.h>
#include <pwd.h>
#include "DirectoryReader.h"
#include "Result.h"
#include <functional>
#include <mutex>
#include <memory>

namespace FileIO {
void SetInterruptFlag(volatile int* interrupted = nullptr);
bool Interrupted();
Result<bool> IsMountPoint(const std::string& pathToDirectory);
Result<std::vector<uint8_t>> ReadBinaryFileContent(const std::string& pathToFile);
Result<std::string> ReadAsciiFileContent(const std::string& pathToFile);

Result<bool> WriteAppendBinaryFileContent(const std::string& filename, const std::vector<uint8_t>& content);
Result<bool> WriteAsciiFileContent(const std::string& pathToFile, const std::string& content);
Result<bool> AppendWriteAsciiFileContent(const std::string& pathToFile, const std::string& content);
Result<bool> WriteFileContentInternal(const std::string& pathToFile, const std::string& content, std::ios_base::openmode mode);

Result<bool> ChangeFileOrDirOwnershipToUser(const std::string& path, const std::string& username);
bool DoesFileExist(const std::string& pathToFile);
bool DoesDirectoryExist(const std::string& pathToDirectory);
bool DoesDirectoryHaveContent(const std::string& pathToDirectory) ;

Result<bool> CleanDirectoryOfFileContents(const std::string& location, size_t& filesRemoved, std::vector<std::string>& foundDirectories);
Result<bool> CleanDirectory(const std::string& directory, const bool removeDirectory, size_t& filesRemoved);
Result<bool> CleanDirectory(const std::string& directory, const bool removeDirectory);
Result<bool> RemoveEmptyDirectories(const std::vector<std::string>& fullPathDirectories);
Result<bool> RemoveFile(const std::string& filename);
bool MoveFile(const std::string& source, const std::string& dest);

Result<std::vector<std::string>> GetDirectoryContents(const std::string& directory);

struct passwd* GetUserFromPasswordFile(const std::string& username);
void SetUserFileSystemAccess(const std::string& username);

struct ScopedFileDescriptor {
   int fd;
   ScopedFileDescriptor(const std::string& location, const int flags, const int permission) {
      fd = open(location.c_str(), flags, permission);
   }

   ~ScopedFileDescriptor() {
      close(fd);
   }
};

// Do file access as root user for the function passed in.
// Example: auto result = FileIO::SudoFile(FileIO::ReadAsciiFileContent, filePath);
template<typename FnCall, typename... Args>
auto SudoFile(FnCall fn, Args&& ... args) -> typename std::result_of<decltype(fn)(Args...)>::type {
   static std::mutex sudoFileMutex;
   std::lock_guard<std::mutex> lock(sudoFileMutex);
   auto previuousuid = setfsuid(-1);
   auto previuousgid = setfsgid(-1);

   // RAII resource cleanup; de-escalate privileges from root to previous: 
   std::shared_ptr<void> resetPreviousPermisions(nullptr, [&](void*) {
      setfsuid(previuousuid);
      setfsgid(previuousgid);
   });

   setfsuid(0);
   setfsgid(0);
   auto func = std::bind(fn, std::forward<Args>(args)...);
   return func();
};

}

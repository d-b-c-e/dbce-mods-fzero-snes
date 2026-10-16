#include "fzero_setup.h"
#ifndef FZERO_SETUP_IDENTITY_HEADER
#define FZERO_SETUP_IDENTITY_HEADER "fzero_setup_identity.h"
#endif
#include FZERO_SETUP_IDENTITY_HEADER
#include <cstdio>
#include <filesystem>
#include <string>
#include <vector>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <bcrypt.h>

namespace {
struct File {
  HANDLE value = INVALID_HANDLE_VALUE;
  ~File() { if (value != INVALID_HANDLE_VALUE) CloseHandle(value); }
};
bool no_links(std::filesystem::path path) {
  for (;;) {
    DWORD attributes = GetFileAttributesW(path.c_str());
    if (attributes == INVALID_FILE_ATTRIBUTES || attributes & FILE_ATTRIBUTE_REPARSE_POINT) return false;
    auto parent = path.parent_path();
    if (parent.empty() || parent == path) return true;
    path = parent;
  }
}
bool identity(File &file, const std::filesystem::path &path, unsigned long long expected_bytes, const char *expected_hash) {
  if (!expected_bytes || !no_links(path)) return false;
  file.value = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
      FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
  if (file.value == INVALID_HANDLE_VALUE) return false;
  LARGE_INTEGER size{};
  if (!GetFileSizeEx(file.value, &size) || size.QuadPart < 0 ||
      static_cast<unsigned long long>(size.QuadPart) != expected_bytes) return false;
  BCRYPT_ALG_HANDLE algorithm{}; BCRYPT_HASH_HANDLE hash{};
  if (BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM, nullptr, 0) < 0) return false;
  bool ok = BCryptCreateHash(algorithm, &hash, nullptr, 0, nullptr, 0, 0) >= 0;
  unsigned char buffer[65536], digest[32];
  unsigned long long remaining = expected_bytes;
  while (ok && remaining) {
    DWORD read = 0, count = remaining > sizeof(buffer) ? static_cast<DWORD>(sizeof(buffer)) : static_cast<DWORD>(remaining);
    ok = ReadFile(file.value, buffer, count, &read, nullptr) && read == count &&
        BCryptHashData(hash, buffer, read, 0) >= 0;
    remaining -= ok ? read : 0;
  }
  if (ok) ok = BCryptFinishHash(hash, digest, sizeof(digest), 0) >= 0;
  if (hash) BCryptDestroyHash(hash);
  BCryptCloseAlgorithmProvider(algorithm, 0);
  if (!ok) return false;
  char text[65];
  for (int i = 0; i < 32; ++i) std::snprintf(text + i * 2, 3, "%02x", digest[i]);
  return std::string(text) == expected_hash;
}
std::filesystem::path game_root() {
  wchar_t image[32768]; DWORD count = GetModuleFileNameW(nullptr, image, 32768);
  return count && count < 32768 ? std::filesystem::path(image).parent_path() : std::filesystem::path{};
}
bool eligible(const std::filesystem::path &root, File &manifest, File &exe) {
  if (!FZERO_SETUP_QUALIFIED || root.empty() || !root.is_absolute()) return false;
  return identity(manifest, root / L"controls-setup/bundle-manifest.json",
          FZERO_SETUP_MANIFEST_BYTES, FZERO_SETUP_MANIFEST_SHA256) &&
      identity(exe, root / L"controls-setup/DbceModSetup.exe", FZERO_SETUP_EXE_BYTES, FZERO_SETUP_EXE_SHA256);
}
// Windows argv quoting, not shell syntax. Keep all arguments independently
// quoted, including terminal backslashes and embedded quotes.
std::wstring quote(const std::wstring &value) {
  std::wstring result = L"\""; size_t slashes = 0;
  for (wchar_t ch : value) {
    if (ch == L'\\') { ++slashes; continue; }
    result.append(slashes * (ch == L'"' ? 2 : 1), L'\\'); slashes = 0;
    if (ch == L'"') result += L'\\';
    result += ch;
  }
  result.append(slashes * 2, L'\\'); result += L'"';
  return result;
}
std::wstring command(const std::filesystem::path &root, DWORD pid, unsigned long long stamp) {
  const std::wstring args[] = {(root / L"controls-setup/DbceModSetup.exe").wstring(),
      L"--game", L"f-zero", L"--game-dir", root.wstring(), L"--wait-for-process",
      std::to_wstring(pid), L"--process-start-filetime", std::to_wstring(stamp)};
  std::wstring result;
  for (const auto &arg : args) { if (!result.empty()) result += L' '; result += quote(arg); }
  return result;
}
using Create = decltype(&CreateProcessW);
bool launch(const std::filesystem::path &root, DWORD pid, unsigned long long stamp, Create create) {
  if (!pid || !stamp || !create) return false;
  File manifest, exe;
  if (!eligible(root, manifest, exe)) return false;
  auto image = root / L"controls-setup/DbceModSetup.exe";
  auto line = command(root, pid, stamp);
  if (line.size() >= 32767) return false;
  STARTUPINFOW startup{}; startup.cb = sizeof(startup);
  startup.dwFlags = STARTF_USESHOWWINDOW; startup.wShowWindow = SW_SHOWNORMAL;
  PROCESS_INFORMATION child{};
  // The two identity files remain open without write/delete sharing until the
  // exact application has been handed to CreateProcessW. No shell or PATH.
  if (!create(image.c_str(), line.data(), nullptr, nullptr, FALSE, 0, nullptr,
      root.c_str(), &startup, &child)) return false;
  CloseHandle(child.hThread); CloseHandle(child.hProcess);
  return true;
}
}
#endif
int FzeroSetupAvailable(void) {
#ifdef _WIN32
  try { File manifest, exe; return eligible(game_root(), manifest, exe); } catch (...) { return 0; }
#else
  return 0;
#endif
}
int FzeroSetupLaunch(char *why, size_t capacity) {
  if (why && capacity) std::snprintf(why, capacity, "Controls setup is unavailable or changed. Reinstall the matching private setup bundle.");
#ifdef _WIN32
  try {
    FILETIME created{}, exited{}, kernel{}, user{};
    if (!GetProcessTimes(GetCurrentProcess(), &created, &exited, &kernel, &user)) return 0;
    const unsigned long long stamp = (static_cast<unsigned long long>(created.dwHighDateTime) << 32) | created.dwLowDateTime;
    if (!launch(game_root(), GetCurrentProcessId(), stamp, &CreateProcessW)) return 0;
    if (why && capacity) *why = 0;
    return 1;
  } catch (...) { return 0; }
#else
  return 0;
#endif
}

// The production implementation with inert identity pins and a fake process
// creator. No real helper, game, Core/device, input or force operation.
#define FZERO_SETUP_IDENTITY_HEADER "setup_test_identity.h"
#include "../src/fzero_setup.cpp"
#include <shellapi.h>
#include <fstream>
#include <cstdlib>
#define CHECK(x) do { if (!(x)) { std::fprintf(stderr, "%d: %s\n", __LINE__, #x); std::exit(1); } } while (0)
static std::filesystem::path root;
static int calls;
static bool fail_create;
static bool reporting;
static BOOL WINAPI fake_create(LPCWSTR image, LPWSTR line, LPSECURITY_ATTRIBUTES process,
    LPSECURITY_ATTRIBUTES thread, BOOL inherit, DWORD flags, LPVOID environment,
    LPCWSTR cwd, LPSTARTUPINFOW startup, LPPROCESS_INFORMATION child) {
  ++calls;
  CHECK(std::filesystem::path(image) == root / L"controls-setup/DbceModSetup.exe");
  CHECK(!process && !thread && !inherit && flags == 0 && !environment);
  CHECK(std::filesystem::path(cwd) == root && startup->wShowWindow == SW_SHOWNORMAL);
  int count = 0; auto args = CommandLineToArgvW(line, &count);
  CHECK(args && count == (reporting ? 12 : 9));
  const int offset = reporting ? 1 : 0;
  if (reporting) CHECK(std::wstring(args[1]) == L"--report");
  CHECK(std::wstring(args[1 + offset]) == L"--game" && std::wstring(args[2 + offset]) == L"f-zero");
  CHECK(std::wstring(args[3 + offset]) == L"--game-dir" && std::filesystem::path(args[4 + offset]) == root);
  if (reporting) {
    CHECK(std::wstring(args[6]) == L"--version" && std::wstring(args[7]) == L"1.8.3");
    CHECK(std::wstring(args[8]) == L"--source-revision" && std::wstring(args[9]) == L"actual-revision");
    CHECK(std::wstring(args[10]) == L"--video-config" && std::filesystem::path(args[11]) == root / L"video & override.ini");
  } else {
    CHECK(std::wstring(args[5]) == L"--wait-for-process" && std::wstring(args[6]) == L"12345");
    CHECK(std::wstring(args[7]) == L"--process-start-filetime" && std::wstring(args[8]) == L"133760000000000001");
  }
  LocalFree(args);
  // Identity handles deny mutation through the exact process-creation call.
  HANDLE write = CreateFileW(image, GENERIC_WRITE, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
  CHECK(write == INVALID_HANDLE_VALUE);
  if (fail_create) return FALSE;
  child->hThread = CreateEventW(nullptr, FALSE, FALSE, nullptr);
  child->hProcess = CreateEventW(nullptr, FALSE, FALSE, nullptr);
  CHECK(child->hThread && child->hProcess);
  return TRUE;
}
static void put(const std::filesystem::path &path, const char *text) {
  std::ofstream stream(path, std::ios::binary); stream << text; stream.close(); CHECK(stream.good());
}
int main() {
  root = std::filesystem::temp_directory_path() / (L"fz setup & unicode-\u00e9-" + std::to_wstring(GetCurrentProcessId()));
  CHECK(!std::filesystem::exists(root) && std::filesystem::create_directory(root));
  auto bundle = root / L"controls-setup";
  CHECK(std::filesystem::create_directory(bundle));
  auto exe = bundle / L"DbceModSetup.exe", manifest = bundle / L"bundle-manifest.json";
  put(exe, "fixture-helper"); put(manifest, "fixture-manifest");
  {File a,b; CHECK(eligible(root,a,b));}
  CHECK(launch(root,12345,133760000000000001ULL,fake_create) && calls == 1);
  fail_create = true;
  CHECK(!launch(root,12345,133760000000000001ULL,fake_create) && calls == 2);
  CHECK(!launch(root,0,133760000000000001ULL,fake_create) && calls == 2);
  CHECK(!launch(root,12345,0,fake_create) && calls == 2);
  fail_create = false; reporting = true;
  const auto video = (root / L"video & override.ini").u8string();
  CHECK(report_launch(root,"1.8.3","actual-revision",video.c_str(),fake_create) && calls == 3);
  CHECK(!report_launch(root,"1.8.3\nspoof","actual-revision",video.c_str(),fake_create) && calls == 3);
  CHECK(!report_launch(root,"1.8.3","",video.c_str(),fake_create) && calls == 3);
  put(exe, "changed-helper");
  CHECK(!report_launch(root,"1.8.3","actual-revision",video.c_str(),fake_create) && calls == 3);
  reporting = false;
  CHECK(!launch(root,12345,133760000000000001ULL,fake_create) && calls == 3);
  put(exe,"fixture-helper"); put(manifest,"changed-manifest");
  {File a,b; CHECK(!eligible(root,a,b));}
  put(manifest,"fixture-manifest!");
  {File a,b; CHECK(!eligible(root,a,b));}
  // Round-trip pathological argument text without invoking a shell/process.
  const std::wstring values[] = {L"", L"C:\\a & b\\", L"quoted\"value", L"slash\\\"quote", L"\u03bb space"};
  for (const auto &value : values) {
    auto line = L"fixture.exe " + quote(value);
    int count = 0; auto args = CommandLineToArgvW(line.c_str(), &count);
    CHECK(args && count == 2 && std::wstring(args[1]) == value); LocalFree(args);
  }
  CHECK(std::filesystem::remove(exe) && std::filesystem::remove(manifest));
  CHECK(std::filesystem::remove(bundle) && std::filesystem::remove(root));
  std::puts("F-Zero controls setup: bounded identities, exact argv, lock and fake spawn checks passed");
}

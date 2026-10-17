#pragma once
#include "../lib/toolkit/include/read_snapshot.h"
#include <array>
#include <string>
#include <vector>

// All calls belong under the existing FFB owner's call gate. This adapter never
// loads a module, creates DirectInput, acquires a handle or computes forces.
struct FzeroSharedOwnerApi {
  int (*enumerate)();
  int (*guid)(int, void *);
  int (*open)(int);
  int (*info)(int, WheelReadDeviceInfoV1 *);
  int (*read)(int, WheelReadSnapshotV1 *);
  int (*path)(int, uint64_t, uint16_t *, int);
  void (*close)();
};
bool FzeroSharedSteeringMatches(const char *, size_t, const unsigned char force[16]);
class FzeroSharedOwner {
 public:
  static constexpr int capacity = 8; // native600 owned read-slot bound
  bool prepare(const char *profile, size_t length, const unsigned char force[16],
               FzeroSharedOwnerApi api, std::string &why);
  bool sample(WheelReadSnapshotV1 out[capacity], std::string &why);
  void close();
  int count() const { return static_cast<int>(devices_.size()); }
  const WheelReadDeviceInfoV1 *devices() const { return devices_.data(); }
  const wchar_t *path(int i) const { return paths_[i].data(); }
 private:
  FzeroSharedOwnerApi api_{};
  bool opened_ = false, failed_ = true;
  std::vector<int> slots_;
  std::vector<WheelReadDeviceInfoV1> devices_;
  std::vector<std::array<wchar_t,260>> paths_;
  std::vector<uint64_t> sequences_;
  std::vector<int64_t> ends_;
};

// Same native module and serialized call gate as the existing force producer.
extern "C" {
int FzeroFfbSharedStart(const char *, void *, const char *, size_t);
int FzeroFfbSharedDevices(WheelReadDeviceInfoV1 *, wchar_t (*)[260], int);
int FzeroFfbSharedRead(WheelReadSnapshotV1 *, int);
void FzeroFfbSharedCoreState(int live);
void FzeroFfbSharedAccept(int live);
}

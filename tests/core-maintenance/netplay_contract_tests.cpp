#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>
#include "xenia/base/byte_order.h"
#include "fd_size.inc"

using X_HANDLE = uint32_t;
static unsigned checks = 0;
void Expect(bool result) {
  ++checks;
  if (!result) throw std::runtime_error("Contract assertion failed");
}
struct XSocket {
  uint16_t port = 0;
  uint16_t bound_port() const { return port; }
  std::string GetProtocolUPnPString() const { return "UDP"; }
};
struct ObjectTable {
  unsigned reads = 0;
  XSocket socket;
  template <typename T> const T* LookupObject(X_HANDLE handle) {
    ++reads;
    return handle == 7 ? &socket : nullptr;
  }
};
struct UpnpFixture {
  uint16_t mapped = 0, tracked = 0;
  bool active = false;
  uint16_t GetMappedBindPort(xe::be<uint16_t>) const { return mapped; }
  bool IsActive() const { return active; }
  void TrackPort(uint16_t port, const std::string&) { tracked = port; }
  int AddPortAsync(const std::string&, uint16_t port, const std::string&) {
    tracked = port;
    return port;
  }
};
struct EmulatorFixture {
  UpnpFixture* upnp = nullptr;
  UpnpFixture* GetUPnP() const { return upnp; }
};
struct KernelFixture {
  ObjectTable objects;
  EmulatorFixture host;
  ObjectTable* object_table() { return &objects; }
  EmulatorFixture* emulator() { return &host; }
} kernel;
KernelFixture* kernel_state() { return &kernel; }
#include "fd.inc"

std::vector<int> upnp_actions_;
void CleanupUPnPActions() { upnp_actions_.clear(); }
struct Address { xe::be<uint16_t> address_port; };
uint16_t BindPortFragment(XSocket* socket, Address* name) {
  const std::string local_ip = "127.0.0.1";
#include "bind_upnp.inc"
  return upnp_internal_port;
}

#include "qos_types.inc"
enum class HTTP_STATUS_CODE { HTTP_OK = 200, HTTP_NOT_FOUND = 404 };
struct Response { HTTP_STATUS_CODE http_code; const uint8_t* response; size_t size; };
struct AllocatorFixture {
  bool fail = false;
  unsigned calls = 0;
  std::vector<uint8_t> bytes;
  uint32_t SystemHeapAlloc(uint32_t size) {
    ++calls;
    if (fail || !size) return 0;
    bytes.assign(size, 0xCC);
    return 0x1000;
  }
  template <typename T> T TranslateVirtual(uint32_t address) {
    Expect(address == 0x1000);
    return reinterpret_cast<T>(bytes.data());
  }
} allocator;
AllocatorFixture* kernel_memory() { return &allocator; }
void QosFragment(const Response& chunk, XNQOSINFO& qos_info) {
#include "qos_copy.inc"
}

void SocketBounds() {
  Expect(!verify_x_fd_set(nullptr));
  x_fd_set set{};
  Expect(verify_x_fd_set(&set));
  for (uint32_t n = 1; n <= X_FD_SETSIZE; ++n) {
    set.fd_count = n;
    for (uint32_t i = 0; i < n; ++i) set.fd_array[i] = 7;
    kernel.objects.reads = 0;
    Expect(verify_x_fd_set(&set));
    Expect(kernel.objects.reads == n);
  }
  for (auto n : {uint32_t(X_FD_SETSIZE + 1), UINT32_MAX}) {
    set.fd_count = n;
    kernel.objects.reads = 0;
    Expect(!verify_x_fd_set(&set));
    Expect(kernel.objects.reads == 0);
  }
  set.fd_count = 1;
  set.fd_array[0] = 8;
  Expect(!verify_x_fd_set(&set));
  set.fd_array[0] = UINT32_MAX;
  Expect(verify_x_fd_set(&set));
}
void BindCases() {
  XSocket socket{49152};
  Address name{};
  Expect(BindPortFragment(&socket, &name) == 49152);
  UpnpFixture upnp;
  kernel.host.upnp = &upnp;
  for (bool active : {false, true}) {
    upnp.active = active;
    for (auto mapped : {uint16_t(0), uint16_t(3074)}) {
      upnp.mapped = mapped;
      const uint16_t expected = mapped ? mapped : socket.port;
      Expect(BindPortFragment(&socket, &name) == expected);
      Expect(upnp.tracked == expected);
    }
  }
  kernel.host.upnp = nullptr;
}
void QosCases() {
  std::vector<uint8_t> payload(65537);
  for (size_t i = 0; i < payload.size(); ++i) payload[i] = uint8_t(i * 17);
  for (bool fail : {false, true}) {
    for (size_t n : {size_t(0), size_t(1), size_t(17), size_t(65535),
                     size_t(65536), size_t(65537)}) {
      allocator = AllocatorFixture{};
      allocator.fail = fail;
      XNQOSINFO info{};
      QosFragment({HTTP_STATUS_CODE::HTTP_OK, payload.data(), n}, info);
      const bool valid = n > 0 && n <= 65535;
      Expect(allocator.calls == (valid ? 1u : 0u));
      if (valid && !fail) {
        Expect(info.data_len == n);
        Expect(info.data_ptr == 0x1000);
        Expect(info.flags == 0x08);
        Expect(allocator.bytes.size() == n);
        Expect(std::equal(allocator.bytes.begin(), allocator.bytes.end(), payload.begin()));
      } else {
        Expect(info.data_ptr == 0);
        Expect(info.data_len == 0);
        Expect(info.flags == 0);
      }
    }
  }
  allocator = AllocatorFixture{};
  XNQOSINFO info{};
  QosFragment({HTTP_STATUS_CODE::HTTP_NOT_FOUND, payload.data(), 17}, info);
  Expect(allocator.calls == 0);
  QosFragment({HTTP_STATUS_CODE::HTTP_OK, nullptr, 17}, info);
  Expect(allocator.calls == 0);
}
int main() {
  try {
    SocketBounds();
    BindCases();
    QosCases();
    std::printf("Passed %u assertions on actual Netplay fragments.\n", checks);
  } catch (const std::exception& error) {
    std::fprintf(stderr, "%s after %u assertions\n", error.what(), checks);
    return 1;
  }
}

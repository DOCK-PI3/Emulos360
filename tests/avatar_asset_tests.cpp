// Focused Emulos360 tests. Real-resource mode reads only the user's local copy.
#include "xenia/kernel/xam/emulos_avatar_assets.h"
#include "xenia/kernel/xam/emulos_avatar_manifest.h"
#include "xenia/kernel/xam/emulos_avatar_scene.h"
#include "xenia/kernel/xam/emulos_avatar_animation.h"
#include "xenia/kernel/xam/emulos_avatar_guest.h"
#include "xenia/cpu/lzx.h"
#include <cstdarg>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <iterator>

extern "C" void xenia_log(const char* format, ...) {
  va_list args;
  va_start(args, format);
  std::vfprintf(stderr, format, args);
  va_end(args);
}
namespace avatar = xe::kernel::xam::emulos;
static std::vector<uint8_t> Read(const char* path) {
  std::ifstream file(path, std::ios::binary);
  return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}
int main(int argc, char** argv) {
  // Reject malformed input without allocations or out-of-range reads.
  for (size_t n = 0; n < 64; ++n) {
    std::vector<uint8_t> invalid(n, 0xff);
    if (avatar::AvatarAssetCatalog::Parse(invalid) || avatar::AvatarStructuredAsset::Parse(invalid) || avatar::AvatarModel::Parse(invalid)) return 1;
  }
  if (argc < 2) { std::cerr << "Provide the local AvatarAssetPack.toc for resource validation.\n"; return 2; }
  const auto bytes = Read(argv[1]);
  auto catalog = avatar::AvatarAssetCatalog::Parse(bytes);
  if (!catalog) { std::cerr << "Catalog rejected\n"; return 3; }
  if (catalog->Find(avatar::kMaleBodyAsset).empty() || catalog->Find(avatar::kFemaleBodyAsset).empty()) return 4;
  size_t models = 0, vertices = 0, triangles = 0, textures = 0, animations = 0;
  auto lzx = [](avatar::AvatarBytes source, std::span<uint8_t> destination) {
    return lzx_decompress(source.data(), source.size(), destination.data(), destination.size(), 32768, nullptr, 0) == 0;
  };
  for (const auto& record : catalog->records()) {
    const auto asset = avatar::AvatarStructuredAsset::Parse(catalog->Find(record.id));
    if (!asset) { std::cerr << "STRB rejected at index " << ((record.id[4] << 8) | record.id[5]) << '\n'; return 5; }
    if(const auto data=asset->Find(1);!data.empty()) {
      const auto animation=avatar::AvatarAnimation::Parse(data);
      if(!animation){std::cerr<<"Animation rejected at index "<<((record.id[4]<<8)|record.id[5])<<'\n';return 12;}
      ++animations;
      if(animations==1){std::cout<<"Animation: "<<animation->frames.size()<<" frames, "<<animation->joints<<" joints, "<<animation->frames_per_second<<" fps\n";
        for(size_t joint:{size_t(0),size_t(1),size_t(10)}) {const auto& p=animation->frames[0][joint];std::cout<<"Joint "<<joint<<": p "<<p.position[0]<<','<<p.position[1]<<','<<p.position[2]<<" q "<<p.rotation[0]<<','<<p.rotation[1]<<','<<p.rotation[2]<<','<<p.rotation[3]<<" s "<<p.scale[0]<<','<<p.scale[1]<<','<<p.scale[2]<<'\n';}
      }
    }
    const auto compressed = asset->Find(3);
    if (compressed.empty()) continue;
    const auto decoded = avatar::DecodeAvatarModelFrames(compressed, lzx);
    if (!decoded) { std::cerr << "LZX rejected at index " << ((record.id[4] << 8) | record.id[5]) << '\n'; return 6; }
    const auto model = avatar::AvatarModel::Parse(*decoded);
    if (!model) {
      std::cerr << "Model rejected at index " << ((record.id[4] << 8) | record.id[5]) << " (" << decoded->size() << " bytes)\n";
      std::ofstream dump("local/avatar-rejected.model", std::ios::binary);
      dump.write(reinterpret_cast<const char*>(decoded->data()), decoded->size());
      return 7;
    }
    ++models;
    textures += model->textures.size();
    for (const auto& batch : model->batches) { vertices += batch.vertices.size(); triangles += batch.indices.size() / 3; }
    if (record.id == avatar::kMaleBodyAsset) {
      if (model->batches.size() != 1 || model->textures.size() != 3 || model->batches[0].vertices.size() != 1137 || model->batches[0].indices.size() != 6792) return 8;
      std::cout << "Male body: 1137 vertices, 2264 triangles, 3 textures\n";
    }
  }
  // Truncation at structural boundaries must fail, even with a valid signature.
  for (const auto n : {size_t(32), size_t(48), size_t(315), bytes.size() / 2}) {
    if (avatar::AvatarAssetCatalog::Parse({bytes.begin(), bytes.begin() + n})) return 9;
  }
  std::cout << "Catalog v" << catalog->version() << ": " << catalog->records().size() << " resources; " << models << " models, " << vertices << " vertices, " << triangles << " triangles, " << textures << " textures\n";
  std::cout<<"Decoded "<<animations<<" original animations\n";
  if (catalog->version() == 2) for (uint8_t body : {uint8_t(1), uint8_t(2)}) {
    const auto manifest = avatar::DefaultAvatarManifest(*catalog, body);
    std::string error;
    const auto scene = avatar::AvatarScene::Decode(*catalog, manifest, 0x1fff, lzx, &error);
    if (!scene) { std::cerr << "Scene rejected for body " << int(body) << ": " << error << '\n'; return 11; }
    std::cout << "Assembled body " << int(body) << ": " << scene->components.size() << " components, original facial resources and clothing overrides\n";
    if (argc > 2) {
      const auto skeleton = avatar::AvatarSkeleton::Parse(Read(argv[2]));
      if (!skeleton) return 13;
      const auto guest = avatar::AvatarGuestAssets::Build(*scene, *skeleton, 0x20000000, 0xA1000000, &error);
      if (!guest) { std::cerr << "Guest assets rejected: " << error << '\n'; return 14; }
      const auto read = [&](size_t offset) { return avatar::AvatarLoad32(guest->cpu, offset); };
      if (read(8) != scene->components.size() || read(4) != 0 || guest->cpu.size() > avatar::kAvatarGuestCpuCapacity ||
          guest->gpu.empty() || guest->gpu.size() > avatar::kAvatarGuestGpuCapacity) return 15;
      // Traverse the emitted ABI as a title would; every relocated pointer
      // must remain inside its caller-owned buffer, including texture layers.
      const auto rig = read(0) - 0x20000000;
      if (read(rig) != skeleton->joints.size() || read(rig + 4) % 16) return 16;
      // Xbox renderers assign the entire Position vector to matrix row 3.
      // W=0 makes the inverse bind matrix singular and hides every vertex.
      const auto joint_data = read(rig + 4) - 0x20000000;
      for (uint32_t j = 0; j < read(rig); ++j) {
        const auto joint = joint_data + j * 96;
        if (read(joint + 28) != std::bit_cast<uint32_t>(1.0f) ||
            read(joint + 60) != std::bit_cast<uint32_t>(1.0f)) {
          std::cerr << "Singular avatar transform: position W must be 1 at joint " << j << '\n';
          return 23;
        }
      }
      for (uint32_t j = 0; j < read(rig); ++j) {
        const auto at = joint_data + j * 96;
        const auto& source = skeleton->joints[j];
        const auto f = [&](size_t offset) { return std::bit_cast<float>(read(offset)); };
        if (f(at + 24) != -source.bind.position[2] ||
            f(at + 56) != -source.local.position[2]) {
          std::cerr << "Guest skeleton must use left-handed coordinates\n";
          return 24;
        }
        for (size_t axis = 0; axis < 4; ++axis) {
          const float sign = axis < 2 ? -1.f : 1.f;
          if (f(at + 32 + axis * 4) != sign * source.bind.rotation[axis] ||
              f(at + 64 + axis * 4) != sign * source.local.rotation[axis]) return 25;
        }
      }
      const auto models = read(16) - 0x20000000;
      for (size_t c = 0; c < scene->components.size(); ++c) {
        const auto p = models + c * 52;
        if (p + 52 > guest->cpu.size()) return 17;
        const auto batches = read(p + 44) - 0x20000000;
        const auto textures = read(p + 48) - 0x20000000;
        if (batches + read(p + 20) * 512 > guest->cpu.size() || textures + read(p + 24) * 44 > guest->cpu.size()) return 18;
        for (size_t b = 0; b < scene->components[c].model.batches.size(); ++b) {
          const auto& source = scene->components[c].model.batches[b];
          const auto vb = read(batches + b * 512 + 504) - 0xA1000000;
          const auto ib = read(batches + b * 512 + 508) - 0xA1000000;
          for (size_t v = 0; v < source.vertices.size(); ++v) {
            const auto at = vb + v * source.vertex_stride;
            for (size_t axis = 0; axis < 3; ++axis) {
              const float actual = std::bit_cast<float>(avatar::AvatarLoad32(guest->gpu, at + axis * 4));
              if (actual != (axis == 2 ? -1.f : 1.f) * source.vertices[v].position[axis]) return 26;
            }
            const auto original = source.vertices[v].normal;
            const auto converted = avatar::AvatarLoad32(guest->gpu, at + 12);
            const auto snorm = [](uint32_t packed) {
              int z = int(packed >> 22); if (z & 512) z -= 1024;
              return std::max(-1.f, float(z) / 511.f);
            };
            if ((original & 0x3fffff) != (converted & 0x3fffff) ||
                std::abs(snorm(original) + snorm(converted)) > 0.00001f) return 27;
          }
          for (size_t i = 0; i < source.indices.size(); ++i) {
            const size_t expected = i % 3 == 0 ? i : (i % 3 == 1 ? i + 1 : i - 1);
            const auto at = ib + i * 2;
            if (((guest->gpu[at] << 8) | guest->gpu[at + 1]) != source.indices[expected]) return 28;
          }
        }
        for (uint32_t t = 0; t < read(p + 24); ++t) {
          const auto q = textures + t * 44;
          const auto data = read(q + 36) - 0xA1000000;
          if ((read(q) & 0x100) || data % 4096 || uint64_t(data) + read(q + 12) > guest->gpu.size() ||
              read(q + 12) != read(q + 20) * read(q + 32) || read(q + 28) != 1) return 19;
          const auto& source = scene->components[c].model.textures[t];
          if (source.empty && (source.format & 63) == 18 &&
              avatar::AvatarLoad32(guest->gpu, data + 4) != UINT32_MAX) return 22;
        }
      }
      if (avatar::AvatarGuestAssets::Build(*scene, *skeleton, 0, 0xA1000000) ||
          avatar::AvatarGuestAssets::Build(*scene, *skeleton, 0x20000000, 0x20001000) ||
          avatar::AvatarGuestAssets::Build(*scene, *skeleton, 0x20000000, 0xFFFFF000)) return 20;
      auto invalid = *scene;
      if (!invalid.components.empty() && !invalid.components[0].model.batches.empty()) {
        invalid.components[0].model.batches[0].indices.push_back(0xffff);
        if (avatar::AvatarGuestAssets::Build(invalid, *skeleton, 0x20000000, 0xA1000000)) return 21;
      }
      std::cout << "Guest ABI4 body " << int(body) << ": " << guest->cpu.size() << " CPU bytes, " << guest->gpu.size() << " GPU bytes; pointer, layout and rejection checks passed\n";
    }
  }
  return models ? 0 : 10;
}

// Portes du SHA-256 du module io : vecteurs de FIPS 180-4 (exemples de l'annexe, et message d'un million de 'a'),
// decoupages arbitraires d'un meme message, finish sans effet sur l'etat, ecriture hexadecimale.
#include <algorithm>
#include <cstdio>
#include <string>
#include <vector>

#include "io_support.hpp"
#include "test.hpp"

using namespace mhgp12;
using namespace mhgp12::io_test;

namespace {

std::string hex_text(std::string_view message) { return hex_of(digest_of(message)); }

// Message pseudo-aleatoire grave (generateur congruentiel, attendu independant de la plate-forme).
std::vector<u8> pattern(std::size_t n) {
  std::vector<u8> bytes(n);
  u32 state = 20261004u;
  for (std::size_t i = 0; i < n; ++i) {
    state = state * 1664525u + 1013904223u;
    bytes[i] = static_cast<u8>(state >> 24);
  }
  return bytes;
}

}  // namespace

MHGP12_TEST(sha256, 27) {
  // FIPS 180-4, exemples SHA-256 (un bloc, deux blocs) ; message vide ; 896 bits ; un million de 'a'
  CHECK_EQ(hex_text(""), std::string("e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"));
  CHECK_EQ(hex_text("abc"), std::string("ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"));
  CHECK_EQ(hex_text("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq"),
           std::string("248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1"));
  CHECK_EQ(hex_text("abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmnoijklmnopjklmnopqklmnopqrlmnopqrs"
                    "mnopqrstnopqrstu"),
           std::string("cf5b16a778af8380036ce59e7b0492370b249b11e8f07a51afac45037afee9d1"));
  Sha256 million;
  const std::string thousand(1000, 'a');
  for (int i = 0; i < 1000; ++i) million.update(thousand);
  CHECK_EQ(million.bytes(), 1000000u);
  CHECK_EQ(hex_of(million.finish()), std::string("cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0"));

  // Decoupages : le meme message, absorbe par morceaux de toutes tailles, donne la meme empreinte
  const std::vector<u8> message = pattern(1000);
  const Digest whole = digest_of(message);
  for (std::size_t chunk : {1u, 3u, 55u, 56u, 63u, 64u, 65u, 127u, 128u, 129u, 999u}) {
    Sha256 sha;
    for (std::size_t at = 0; at < message.size(); at += chunk)
      sha.update(std::span<const u8>(message.data() + at, std::min(chunk, message.size() - at)));
    CHECK(sha.finish() == whole);
  }
  // Morceaux vides intercales
  Sha256 gaps;
  for (std::size_t at = 0; at < message.size(); at += 100) {
    gaps.update(std::span<const u8>());
    gaps.update(std::span<const u8>(message.data() + at, 100));
  }
  CHECK(gaps.finish() == whole);

  // finish ne finalise pas : rappele, il rend la meme empreinte ; une suite donne l'empreinte du message prolonge
  Sha256 running;
  running.update(std::span<const u8>(message.data(), 500));
  const Digest half = running.finish();
  CHECK(running.finish() == half);
  CHECK(half == digest_of(std::vector<u8>(message.begin(), message.begin() + 500)));
  running.update(std::span<const u8>(message.data() + 500, 500));
  CHECK(running.finish() == whole);
  CHECK_EQ(running.bytes(), 1000u);

  // Frontieres du remplissage : longueurs 55, 56, 63, 64, 65 (un ou deux blocs de fin)
  const std::vector<std::pair<std::size_t, std::string>> borders{
      {55, "9f4390f8d30c2dd92ec9f095b65e2b9ae9b0a925a5258e241c9f1e910f734318"},
      {56, "b35439a4ac6f0948b6d6f9e3c6af0f5f590ce20f1bde7090ef7970686ec6738a"},
      {63, "7d3e74a05d7db15bce4ad9ec0658ea98e3f06eeecf16b4c6fff2da457ddc2f34"},
      {64, "ffe054fe7ae0cb6dc65c3af9b61d5209f439851db43d0ba5997337df154668eb"},
      {65, "635361c48bb9eab14198e76ea8ab7f1a41685d6ad62aa9146d301d4f17eb0ae0"}};
  for (const auto& [length, expected] : borders) CHECK_EQ(hex_text(std::string(length, 'a')), expected);
}

// Les deux voies de compression (instructions SHA d'x86-64 et voie portable) : memes vecteurs de FIPS 180-4, et memes
// empreintes sur des messages graves de 0 a 4 100 octets absorbes en un bloc, par morceaux de 1, 63, 64, 65 et 1 000
// octets, puis par morceaux alternes. Sans instructions SHA (ou hors x86-64), la voie materielle est la voie portable :
// le test le dit et joue les controles sur la seule voie portable.
MHGP12_TEST(sha256_voies, 136) {
  const bool hardware = io::detail::sha_hardware();
  std::printf("sha256_voies : voie materielle %s\n", hardware ? "presente" : "absente (voie portable seule)");
  const auto digest_on = [](bool on, std::span<const u8> message, std::size_t chunk) {
    io::detail::set_sha_hardware(on);
    Sha256 sha;
    for (std::size_t at = 0; at < message.size(); at += chunk)
      sha.update(message.subspan(at, std::min(chunk, message.size() - at)));
    return sha.finish();
  };
  for (bool on : {false, true}) {
    io::detail::set_sha_hardware(on);
    CHECK_EQ(hex_text("abc"), std::string("ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"));
    CHECK_EQ(hex_text("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq"),
             std::string("248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1"));
  }
  const std::vector<u8> big = pattern(4100);
  for (std::size_t length : {0u, 1u, 55u, 56u, 63u, 64u, 65u, 127u, 128u, 129u, 1000u, 4096u, 4100u}) {
    const auto message = std::span<const u8>(big.data(), length);
    const Digest reference = digest_on(false, message, length == 0 ? 1 : length);
    for (std::size_t chunk : {1u, 63u, 64u, 65u, 1000u}) {
      CHECK(digest_on(true, message, chunk) == reference);
      CHECK(digest_on(false, message, chunk) == reference);
    }
  }
  // Morceaux alternes : la voie peut changer entre deux mises a jour du meme objet sans changer l'empreinte.
  Sha256 mixed;
  for (std::size_t at = 0, i = 0; at < big.size(); at += 300, ++i) {
    io::detail::set_sha_hardware(i % 2 == 0);
    mixed.update(std::span<const u8>(big.data() + at, std::min<std::size_t>(300, big.size() - at)));
  }
  CHECK(mixed.finish() == digest_on(false, big, big.size()));
  io::detail::set_sha_hardware(true);
  CHECK(io::detail::sha_hardware() == hardware);
}

MHGP12_TEST(hex, 3) {
  Digest d{};
  for (std::size_t i = 0; i < d.size(); ++i) d[i] = static_cast<u8>(i * 8 + 1);
  CHECK_EQ(hex_of(d), std::string("0109111921293139414951596169717981899199a1a9b1b9c1c9d1d9e1e9f1f9"));
  CHECK_EQ(hex_of(Digest{}), std::string(64, '0'));
  Digest top{};
  top.fill(0xff);
  CHECK_EQ(hex_of(top), std::string(64, 'f'));
}

MHGP12_TEST_MAIN()

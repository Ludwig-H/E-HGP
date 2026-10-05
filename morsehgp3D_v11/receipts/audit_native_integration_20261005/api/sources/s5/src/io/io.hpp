// En-tete public du module io : lecture des nuages u32le, empreintes SHA-256, ecrivains petit-boutistes et
// transaction de dossier. Port explicite du raccord R2 de la v10 (commit 865f5e6 : src/cloud/u32le_input.hpp et
// src/core/cli_output.hpp) et du SHA-256 de morsehgp3d (src/cpu/contract/canonical_id.cpp) ; voir
// docs/PROVENANCE.md, section io.
//
// Ce qui change par rapport aux sources :
//   - aucune exception : Outcome et Result, toutes les fonctions sont noexcept ; les tableaux de points sont des
//     Buffer admis dans le MemoryBudget avant l'allocation (la v10 lisait dans des std::vector) ;
//   - les identifiants viennent d'un fichier ids.u32le separe (4 octets par point), comme bench/whole_input.hpp ;
//     la v10 prenait le rang d'entree pour PointId ;
//   - la transaction porte sur un DOSSIER, pas sur des fichiers : tout est ecrit dans D.pending/, manifeste en
//     dernier, puis publie par un seul renommage sans remplacement (renameat2, RENAME_NOREPLACE). La cible D ne doit
//     jamais exister : il n'y a ni remplacement, ni sauvegarde .bak, ni restauration (paragraphe 6.7 de la
//     specification de la sortie parametree).
//
// Refus emis par ce module (src/core/reasons.def) :
//   parameter_out_of_range  chemin de dossier ou nom de fichier hors de leur forme, trop de fichiers ;
//   input_unreadable        entree absente, non reguliere, de taille incoherente, ou lue incompletement ;
//   output_unwritable       dossier parent absent ou non inscriptible, erreur d'ecriture, de fsync ou de
//                           renommage, renommage sans remplacement indisponible ;
//   output_conflict         D ou D.pending existe deja (un D.pending orphelin n'est jamais retire), une entree se
//                           trouve sous D, deux fichiers d'un appel portent le meme nom.
// Plus memory_budget (lecture) et index_overflow_u32 (au moins kNone points, controle de cloud).
#pragma once

#include <array>
#include <cstdio>
#include <span>
#include <string_view>

#include "cloud/cloud.hpp"
#include "core/core.hpp"

namespace mhgp11::io {

inline constexpr std::size_t kDigestBytes = 32;
using Digest = std::array<u8, kDigestBytes>;

// Empreinte SHA-256 (FIPS 180-4) calculee au fil des donnees. finish() ne modifie pas l'etat : il peut etre appele a
// tout moment et rend l'empreinte du message recu jusque-la. Message de moins de 2^61 octets (FIPS 180-4 : moins de
// 2^64 bits) : FileWriter refuse de depasser cette longueur.
class Sha256 {
 public:
  Sha256() noexcept;
  void update(std::span<const u8> bytes) noexcept;
  void update(std::string_view text) noexcept;
  Digest finish() const noexcept;
  u64 bytes() const noexcept { return total_; }

 private:
  void compress(const u8* block) noexcept;

  std::array<u32, 8> state_{};
  std::array<u8, 64> buffer_{};
  u64 buffered_ = 0;
  u64 total_ = 0;
};

// Plus longue suite d'octets que Sha256 et FileWriter acceptent : 2^61 - 1 (le compte de bits tient dans un u64).
inline constexpr u64 kMaxMessageBytes = (u64{1} << 61) - 1;

// Empreinte en 64 chiffres hexadecimaux minuscules, sans allocation.
std::array<char, 2 * kDigestBytes> to_hex(const Digest& digest) noexcept;

// Nuage lu depuis deux fichiers : points.u32le (x y z, trois mots u32 petit-boutistes par point) et ids.u32le (un
// PointId u32 petit-boutiste par point, dans le meme ordre). Empreintes et tailles des fichiers lus, octet pour octet.
struct InputFiles {
  Buffer<u32> x, y, z;
  Buffer<PointId> ids;
  Digest points_sha256{}, ids_sha256{};
  u64 points_bytes = 0, ids_bytes = 0;
};

// Controle des tailles annoncees, sans lecture ni allocation. Premier refus dans cet ordre : input_unreadable
// (taille de points non multiple de 12, de ids non multiple de 4, ou nombres de points differents), puis
// index_overflow_u32 (au moins kNone points, check_cloud_sizes). Deux fichiers vides sont admis : le nuage vide est
// refuse ensuite par prepare_cloud (empty_input), comme dans le raccord R2.
[[nodiscard]] Outcome check_u32le_sizes(u64 points_bytes, u64 ids_bytes) noexcept;

// Lit les deux fichiers EN ENTIER ou refuse, sans jamais tronquer. Ordre :
//   1. ouverture et fstat de chaque fichier, sans allocation : input_unreadable s'il est absent, illisible ou non
//      regulier (tube, peripherique, dossier : aucune taille annoncee, donc aucune admission possible) ;
//   2. check_u32le_sizes sur les tailles annoncees ;
//   3. admission de 16 octets par point dans `budget`, puis allocation des quatre tableaux : memory_budget ;
//   4. lecture par blocs, empreintes au fil de la lecture ; un fichier plus court que sa taille annoncee, ou qui
//      porte un octet de plus, rend input_unreadable.
// Le domaine des coordonnees et l'unicite des PointId restent juges par prepare_cloud. Un refus ne laisse rien de
// reserve dans `budget`.
[[nodiscard]] Result<InputFiles> read_u32le(const char* points, const char* ids, MemoryBudget& budget) noexcept;

// Plus grand nombre de fichiers de donnees d'un dossier, manifeste non compris.
inline constexpr u32 kMaxOutputFiles = 8;
// Nom du manifeste, ecrit en dernier par commit ; reserve.
inline constexpr std::string_view kManifestName = "manifeste.json";
// Plus long nom de fichier admis par create, en octets.
inline constexpr std::size_t kMaxFileName = 64;

// Fichier d'un dossier en cours d'ecriture : octets ecrits en petit-boutiste, taille et empreinte tenues au fil de
// l'ecriture. Une erreur est definitive : toute ecriture suivante et le commit du dossier sont refuses
// (output_unwritable). Un FileWriter n'est obtenu que par OutputDirectory::create ; un objet construit par defaut
// n'a pas de fichier et refuse toute ecriture.
class FileWriter {
 public:
  FileWriter() noexcept = default;
  FileWriter(const FileWriter&) = delete;
  FileWriter& operator=(const FileWriter&) = delete;
  ~FileWriter();

  [[nodiscard]] Outcome bytes(std::span<const u8> data) noexcept;
  [[nodiscard]] Outcome u32s(std::span<const u32> words) noexcept;
  [[nodiscard]] Outcome u64s(std::span<const u64> words) noexcept;
  // Octets nuls jusqu'a la prochaine frontiere de 8 octets (debut de colonne, paragraphe 6.1).
  [[nodiscard]] Outcome pad8() noexcept;
  u64 size() const noexcept { return size_; }
  Digest digest() const noexcept { return sha_.finish(); }
  std::string_view name() const noexcept { return {name_.data(), name_length_}; }

 private:
  friend class OutputDirectory;
  [[nodiscard]] Outcome open(int directory_fd, std::string_view name) noexcept;
  [[nodiscard]] Outcome close_synced() noexcept;
  [[nodiscard]] Outcome write_raw(const u8* data, u64 n) noexcept;
  // Reprend l'etat de `other` (deplacement d'un OutputDirectory) ; `other` devient sans fichier.
  void take(FileWriter& other) noexcept;

  std::FILE* file_ = nullptr;
  Sha256 sha_;
  u64 size_ = 0;
  bool failed_ = false;
  bool created_ = false;  // fichier cree dans D.pending : retire par le dossier sans commit
  std::array<char, kMaxFileName + 1> name_{};
  std::size_t name_length_ = 0;
};

// Transaction de dossier (paragraphe 6.7) :
//   1. plan, AVANT tout calcul : forme du chemin, conflits, droits ; rien n'est cree ;
//   2. create, apres le calcul : D.pending/ est cree a la premiere demande, chaque fichier y est cree en exclusif ;
//   3. commit : chaque fichier est vide et controle (fflush, ferror, fsync, fclose), le manifeste est ecrit en
//      dernier et synchronise, puis le dossier ; D.pending est renomme en D par renameat2(RENAME_NOREPLACE), puis le
//      parent est synchronise. Un lecteur voit D entier ou ne le voit pas ;
//   4. sans commit reussi, le destructeur retire les fichiers crees puis D.pending ; D n'est jamais touche, et un
//      D.pending que cet objet n'a pas cree n'est jamais retire.
// Un OutputDirectory se deplace seulement avant le premier create (les FileWriter rendus sont ses membres).
class OutputDirectory {
 public:
  // Ordre des refus : parameter_out_of_range (chemin vide, de plus de PATH_MAX - 1 octets, nom final vide, "." ou
  // "..", ou D.pending de plus de NAME_MAX octets) ; output_unwritable (dossier parent absent ou non dossier) ;
  // output_conflict (une entree resolue est D, D.pending ou se trouve dessous ; D ou D.pending existe, lien
  // symbolique pendant compris) ; output_unwritable (parent non inscriptible). Les entrees sans chemin resolu ne
  // sont pas comparees : leur lecture les refusera (input_unreadable).
  [[nodiscard]] static Result<OutputDirectory> plan(const char* directory,
                                                    std::span<const char* const> inputs) noexcept;
  OutputDirectory(const OutputDirectory&) = delete;
  OutputDirectory& operator=(const OutputDirectory&) = delete;
  OutputDirectory(OutputDirectory&& other) noexcept;
  OutputDirectory& operator=(OutputDirectory&&) = delete;
  ~OutputDirectory();

  // Nom [a-z0-9_.]+ d'au plus kMaxFileName octets, ne commencant pas par '.', autre que kManifestName :
  // sinon parameter_out_of_range, de meme au-dela de kMaxOutputFiles fichiers. Nom deja cree : output_conflict.
  // D.pending apparu depuis le plan : output_conflict. Apres un commit, reussi ou non : output_unwritable.
  [[nodiscard]] Result<FileWriter*> create(std::string_view name) noexcept;
  // Publie le dossier ; refus output_unwritable (ecriture, synchronisation, renommage indisponible) ou
  // output_conflict (D apparu depuis le plan). Un commit refuse est definitif : l'objet ne publie plus rien.
  [[nodiscard]] Outcome commit(std::string_view manifest_json) noexcept;
  bool committed() const noexcept { return committed_; }
  // Retire un dossier que cet objet vient de publier (frontiere R2 : un echec apres le commit, par exemple de la
  // sortie standard, ne doit laisser aucun dossier publie) : D est renomme en D.pending sans remplacement, puis le
  // destructeur le retire comme un dossier jamais publie ; l'objet ne publie plus rien ensuite. Refus
  // output_unwritable si rien n'est publie ou si le renommage echoue (D reste alors publie et complet).
  [[nodiscard]] Outcome retract() noexcept;
  // Empreinte du manifeste, gardee des sa fermeture par commit, meme si la publication echoue ensuite : apres un
  // double echec (committed() vrai, commit refuse), c'est celle du manifeste de D publie. Zeros tant qu'aucun
  // manifeste n'est ferme. Elle ne dit pas a elle seule que D est publie : committed() le dit.
  const Digest& manifest_sha256() const noexcept { return manifest_sha256_; }

 private:
  OutputDirectory() noexcept = default;
  [[nodiscard]] Outcome open_pending() noexcept;
  [[nodiscard]] Outcome close_data() noexcept;
  [[nodiscard]] Outcome write_manifest(std::string_view manifest_json) noexcept;
  [[nodiscard]] Outcome publish() noexcept;
  [[nodiscard]] Outcome commit_steps(std::string_view manifest_json) noexcept;
  void discard() noexcept;

  int parent_fd_ = -1;
  int pending_fd_ = -1;
  std::array<char, 256> base_{};     // nom final D dans le parent
  std::array<char, 256> pending_{};  // D.pending dans le parent
  std::array<FileWriter, kMaxOutputFiles + 1> writers_{};  // donnees, puis le manifeste en derniere case
  u32 count_ = 0;
  bool pending_created_ = false;
  bool committed_ = false;
  bool failed_ = false;
  Digest manifest_sha256_{};
};

}  // namespace mhgp11::io

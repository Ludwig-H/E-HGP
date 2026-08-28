// MorseHGP3D v5 — REDUCTEUR VIVANT (docs/ECHELLE.md § 8 bis, etape L2 ;
// theoreme T6 « components <= live_aliases »). Meme sortie que `reduce_fold`
// (fold.hpp), mais l'etat n'est plus proportionnel au nombre de facettes de
// l'ordre : il ne porte que les facettes ENCORE REUTILISABLES.
//
// Ce que le resident garde et que le vivant ne garde pas : un `FidState` par
// facette (32 o x nfid) et `final_canon_fid` (4 o x nfid). Ce que le vivant
// garde : un `Alias` par facette entre sa PREMIERE et sa DERNIERE incidence,
// un `Component` par composante ayant au moins un alias, et une table
// fid -> alias dimensionnee sur le vivant.
//
// EXACTITUDE — le vivant reproduit le resident evenement par evenement :
//   - ROLES et DETECTEURS : identiques (kActive / kAttach, `seen`) ;
//   - GEL PRE-LOT : le lot se fait en DEUX passes (tous les gels d'abord,
//     puis toutes les unions), comme le resident ou `pre_list` precede les
//     unions ; chaque gel copie la CLE canonique pre-lot, jamais un pointeur
//     vers un enregistrement qui peut mourir dans la passe d'unions ;
//   - UNION : la racine LOGIQUE est celle de `first` (regle exacte du
//     resident, `unite_canon(first, v)`), le CANONIQUE est le minimum
//     historique, et le conteneur PHYSIQUE est le record de plus grande
//     masse historique (small-to-large : chaque alias est relocalise
//     O(log) fois ; mutant `physical-root-is-logical-root`, tue par le
//     plafond de relocalisations) ;
//   - ORDRE DES DELTAS : `post_list` est triee par `logical_root_fid`, qui
//     est par construction le fid de la racine union-find du resident ;
//   - MORTS : les alias dont la DERNIERE incidence est le lot courant sont
//     retires apres l'emission du lot ; une composante sans alias est
//     definitivement liberable (toute connexion future passerait par une
//     facette a une incidence deja depassee).
//
// Ce que le vivant NE produit PAS : `facet_keys` et `final_canon_fid`, qui
// sont O(nfid). Le theoreme T5 les rend fonctions du flux de deltas et la
// porte de rejeu (tests/delta_replay_gate.cpp, tests/fold_live_gate.cpp) les
// reconstruit — c'est la seule autorite d'egalite avec le resident.
//
// CRENEAUX PLUTOT QU'UNE TABLE (28 aout, apres mesure) : une premiere version
// retrouvait l'alias d'une facette par une table de hachage `fid -> alias`
// avec suppression a la mort. La sonde miroir (bench/fold_live_probe.cpp) l'a
// mesuree 1,9 a 3,5 fois plus lente que le fold resident, qui indexe un
// tableau plat. Les durees de vie forment un GRAPHE D'INTERVALLES : un
// coloriage glouton (pile de creneaux libres) en utilise exactement le nombre
// maximal d'intervalles simultanes, c'est-a-dire le pic exact deja calcule.
// Chaque facette vivante recoit donc un CRENEAU dans [0, pic), l'etat est un
// tableau PLAT de `pic` alias reutilises en place — plus de sondage, plus de
// suppression par decalage, plus de liste libre d'alias — et le creneau est
// exactement ce que le wire de L3 portera avec chaque occurrence
// (`FacetOccurrenceWire` — une structure PROPOSEE dans docs/ECHELLE.md, pas
// encore enregistree), ce qui retirerait aussi les derniers tableaux indexes
// par facette. En attendant, `free_slots` est bien une LISTE LIBRE (une pile
// de creneaux rendus), et L2 reste O(facettes) en memoire par ses tables
// `firstb`, `lastb` et `fid -> creneau`.
//
// Les durees de vie (PREMIERE / DERNIERE par facette) et la table
// `fid -> creneau` sont ici en RAM depuis `FoldPrepared` : a L2 c'est le
// sujet, pas la revendication ; a L3 elles viennent du tri externe des cles
// completes (ECHELLE § 4.2) et arrivent avec chaque occurrence.
#pragma once

#include <algorithm>
#include <string>
#include <vector>

#include "fold.hpp"

namespace mhgp5 {

// Residence MESUREE du reducteur vivant (jamais annoncee).
struct LiveFoldStats {
  u64 facets = 0;                // alias crees = facettes distinctes (une creation par facette)
  u64 peak_aliases = 0;          // pic d'alias vivants (dans un lot, apres les creations)
  u64 peak_components = 0;       // pic de composantes vivantes
  u64 peak_live_exact = 0;       // pic exact des durees de vie : live += nees[b] ; pic ; live -= mortes[b]
  u64 slots = 0;                 // creneaux alloues (= pic exact hors mutant de capacite)
  u64 slot_overflows = 0;        // demandes de creneau au-dela du pic : provoque un REFUS controle, jamais un acces sentinelle
  u64 slot_partition_violations = 0;  // frontieres ou creneaux libres + creneaux vivants != creneaux alloues
  u64 relocations = 0;           // alias deplaces par les unions (small-to-large)
  u64 max_moves_per_alias = 0;   // deplacements du PIRE alias (borne small-to-large : <= ceil(log2 facettes))
  u64 comps_freed = 0;           // composantes liberees faute d'alias
  u64 invariant_violations = 0;  // frontieres ou composantes <= alias <= pic exact est faux
  u64 life_violations = 0;       // frontieres ou alias != compte EXACT du lot (avant et apres les morts)
  u64 structure_violations = 0;  // incoherences index/listes/comptes relevees par les balayages
  u64 final_nonempty = 0;        // 1 si l'etat n'est pas VIDE apres le dernier lot
  u64 structure_scans = 0;       // balayages structurels effectues
  // DEUX metriques d'octets, jamais melangees (audit du 28 aout) :
  u64 logical_live_bytes = 0;    // etat LOGIQUEMENT vivant au pic : alias + composantes vivants
  // TROIS POSTES SEPARES (audit du 28 aout : une metrique unique melangeait
  // des choses de natures differentes et laissait croire a un etat borne) :
  u64 persistent_bytes = 0;      // ETAT PERSISTANT, seul poste borne par le pic : tableau plat des creneaux,
                                 // arene des composantes, listes libres, marquage du balayage ;
  u64 mapping_bytes = 0;         // MAPPING O(facettes) : `firstb`, `lastb`, `fid -> creneau` — c'est ce que
                                 // seul le flux de L3 pourrait retirer ;
  u64 input_bytes = 0;           // PREPARATION ET ENTREE : `keys`, `ev_fid`, evenements, ordre, lots de
                                 // `FoldPrepared` — O(facettes + incidences), ni mapping ni scratch ;
  u64 scratch_bytes = 0;         // SCRATCH du plus gros lot : touches, gels, post-liste, alias par emplacement,
                                 // deltas de travail ET les capacites de leurs vecteurs, comptes par lot ;
  u64 output_bytes = 0;          // SORTIE : `r.deltas` et les capacites de leurs vecteurs.
                                 // Aucun de ces postes n'est une somme allocator-precise : ce sont des
                                 // capacites de conteneurs, publiees separement pour ne rien promettre de plus.
                                 // Le L2 complet, entree et sortie comprises, est O(facettes + incidences).
};

namespace live_detail {

inline constexpr u32 kNil = UINT32_MAX;
// Cadence maximale des balayages structurels (chacun coute O(vivant)).
inline constexpr size_t kStructureScans = 64;

// Un ALIAS par facette encore reutilisable : sa cle (le reducteur n'a donc
// pas besoin du catalogue), sa composante, ses liens intrusifs, son lot de
// DERNIERE incidence, son role dans le lot courant.
struct Alias {
  FacetKey key;
  u32 fid = 0;
  u32 comp = kNil;
  u32 next = kNil, prev = kNil;
  u32 last_batch = 0;
  u32 moves = 0;  // deplacements de conteneur physique subis par cet alias
  u32 role_epoch = UINT32_MAX;
  u8 role_bits = 0;
  u8 seen = 0;
  u8 dead = 0;  // mutant `free-on-absorb` seulement : alias orphelin d'une absorption (jamais libere)
};

// Une COMPOSANTE par classe d'union ayant au moins un alias.
struct Component {
  FacetKey canon_key;             // cle du canonique = minimum HISTORIQUE (jamais relue d'un record)
  u32 canon_fid = 0;
  u32 logical_root_fid = 0;       // la racine que le resident aurait : celle de `first` absorbe
  u32 head = kNil;
  u32 count = 0;                  // alias vivants ; 0 => liberable
  u64 mass = 0;                   // masse HISTORIQUE (small-to-large)
  u32 post_epoch = UINT32_MAX, post_slot = 0;
};

}  // namespace live_detail

// REDUCTION VIVANTE : meme `deltas`, memes `batch_levels`, memes compteurs
// que `reduce_fold`, avec un etat borne par le vivant. `facet_keys` et
// `final_canon_fid` restent vides (O(nfid), reconstruits par le rejeu T5).
inline ForestResult reduce_fold_live(FoldPrepared&& fp, LiveFoldStats* out_stats = nullptr) {
  using namespace live_detail;
  ForestResult r = std::move(fp.r);
  LiveFoldStats stats;
  if (!r.refusal.empty()) {
    if (out_stats) *out_stats = stats;
    return r;
  }
  const std::vector<ForestEvent>& events = *fp.events;
  const std::vector<u32>& order = fp.order;
  const auto evt = [&](size_t i) -> const ForestEvent& { return events[(size_t)order[i]]; };
  const std::vector<std::pair<size_t, size_t>>& batches = fp.batches;
  const std::vector<FacetKey>& keys = fp.keys;
  const std::vector<u32>& ev_fid = fp.ev_fid;
  const bool m_attach_pre = fp.mutants[2], m_drop_nonmerge = fp.mutants[3], m_no_detector = fp.mutants[5];
  const bool m_phys_is_log = MHGP5_MUTANT("physical-root-is-logical-root");
  const bool m_free_on_absorb = MHGP5_MUTANT("free-on-absorb");
  const bool m_root_key_mutable = MHGP5_MUTANT("root-key-mutable");
  const bool m_canon_not_min = MHGP5_MUTANT("canon-not-min-on-union");
  const bool m_last_shifted = MHGP5_MUTANT("last-mark-shifted");
  const size_t nfid = keys.size();
  const size_t nb = batches.size();
  r.facets = nfid;
  // PREPARATION ET ENTREE : ce que le reducteur RECOIT et ne libere pas.
  stats.input_bytes = (u64)(keys.capacity() * sizeof(FacetKey) + ev_fid.capacity() * sizeof(u32) + events.capacity() * sizeof(ForestEvent) +
                            order.capacity() * sizeof(u32) + batches.capacity() * sizeof(std::pair<size_t, size_t>));

  // ---- DUREES DE VIE (a L3 : le tri externe des cles completes ; ici : deux
  // tableaux u32 par facette, hors de la revendication de residence).
  std::vector<u32> firstb(nfid, UINT32_MAX), lastb(nfid, 0);
  for (size_t b = 0; b < nb; ++b)
    for (size_t e = batches[b].first; e < batches[b].second; ++e) {
      const ForestEvent& ev = evt(e);
      for (int t = 0; t < (int)ev.q + (int)ev.d; ++t) {
        const u32 fid = ev_fid[e * 11 + (size_t)t];
        if (firstb[fid] == UINT32_MAX) firstb[fid] = (u32)b;
        lastb[fid] = (u32)b;
      }
    }
  // COMPTE EXACT PAR LOT, inclusif et sans heuristique : `live_in[b]` facettes
  // vivantes apres les naissances du lot b, `live_out[b]` apres ses morts. Le
  // reducteur doit egaler ces deux nombres a chaque frontiere — c'est plus
  // fort que la seule comparaison au pic global (audit du 28 aout).
  std::vector<u32> live_in(nb, 0), live_out(nb, 0);
  {
    std::vector<u32> born_at(nb + 1, 0), died_at(nb + 1, 0);
    for (size_t f = 0; f < nfid; ++f) {
      if (firstb[f] == UINT32_MAX) continue;
      ++born_at[firstb[f]];
      ++died_at[lastb[f]];
    }
    u64 live = 0;
    for (size_t b = 0; b < nb; ++b) {
      live += born_at[b];
      live_in[b] = (u32)live;
      stats.peak_live_exact = std::max(stats.peak_live_exact, live);
      live -= died_at[b];
      live_out[b] = (u32)live;
    }
  }

  // ---- ETAT VIVANT : un TABLEAU PLAT de `pic` alias, un CRENEAU par facette
  // vivante (coloriage glouton d'un graphe d'intervalles : le nombre de
  // couleurs necessaires EST le nombre maximal d'intervalles simultanes, donc
  // le pic exact). `slot_of_fid` est la seule table indexee par facette qui
  // reste ici ; a L3 le creneau arrive avec l'occurrence et elle disparait.
  // MUTANT `slot-cap-minus-one` : un creneau de moins que le pic. Le
  // reducteur doit alors REFUSER proprement (statut, message) et ne jamais
  // toucher une sentinelle.
  size_t nslots = (size_t)stats.peak_live_exact;
  if (MHGP5_MUTANT("slot-cap-minus-one") && nslots > 0) --nslots;
  std::vector<Alias> av(nslots);
  std::vector<u32> free_slots(nslots);
  for (size_t i = 0; i < nslots; ++i) free_slots[i] = (u32)(nslots - 1 - i);  // creneaux rendus dans l'ordre croissant
  std::vector<u32> slot_of_fid(nfid, kNil);
  std::vector<Component> cv;
  std::vector<u32> cfree;
  stats.slots = nslots;
  u64 live_alias = 0, live_comp = 0;
  const auto alloc_comp = [&]() -> u32 {
    if (!cfree.empty()) {
      const u32 c = cfree.back();
      cfree.pop_back();
      cv[c] = Component{};  // epoques remises a UINT32_MAX : un slot recycle ne herite d'aucun lot
      return c;
    }
    cv.emplace_back();
    return (u32)(cv.size() - 1);
  };

  std::vector<u32> touched;
  std::vector<u8> slot_mark;  // balayage structurel : 1 libre, 2 vivant (jamais les deux, jamais aucun)
  // ALIAS PAR EMPLACEMENT du lot : la passe de roles connait deja l'alias de
  // chaque (evenement, slot) ; le memoriser evite un SECOND sondage de la
  // table par site dans la passe d'unions. Mesure du 28 aout : le reducteur
  // vivant etait 1,9 a 3,5 fois plus lent que le resident, dont une part
  // vient de ces sondages a acces aleatoire.
  std::vector<u32> slot_alias;
  struct PreFreeze {
    u32 rep_alias;
    FacetKey pre_canon_key;
  };
  std::vector<PreFreeze> pre_list;
  std::vector<u32> post_list;
  std::vector<ComponentDelta> scratch;
  r.deltas.reserve(nb);
  auto tmark = std::chrono::steady_clock::now();

  // UNION ORDONNEE : racine logique = celle de `first`, canonique = minimum
  // historique, conteneur physique = plus grande masse.
  const auto unite = [&](u32 a, u32 b) -> bool {
    const u32 ca = av[a].comp, cb = av[b].comp;
    if (ca == cb) return false;
    const u32 lroot = cv[ca].logical_root_fid;
    u32 canon_fid = cv[ca].canon_fid;
    FacetKey canon_key = cv[ca].canon_key;
    if (!m_canon_not_min && cv[cb].canon_fid < canon_fid) {
      canon_fid = cv[cb].canon_fid;
      canon_key = cv[cb].canon_key;
    }
    u32 big = ca, small = cb;
    if (!m_phys_is_log && cv[cb].mass > cv[ca].mass) {
      big = cb;
      small = ca;
    }
    const u64 mass = cv[ca].mass + cv[cb].mass;
    const u32 count = cv[ca].count + cv[cb].count;
    if (m_free_on_absorb) {
      // MUTANT : le record absorbe est detruit SANS relocaliser ses alias —
      // le defaut causal exact que small-to-large doit interdire. Les alias
      // restent joignables (rien n'est libere, aucun index n'est recycle :
      // la memoire reste sure) mais referencent un record detruit, donc
      // recycle : la SORTIE diverge des la premiere reutilisation.
      for (u32 x = cv[small].head; x != kNil; x = av[x].next) av[x].dead = 1;
    } else {
      u32 tail = kNil;
      for (u32 x = cv[small].head; x != kNil; x = av[x].next) {
        av[x].comp = big;
        tail = x;
        ++stats.relocations;
        stats.max_moves_per_alias = std::max(stats.max_moves_per_alias, (u64)++av[x].moves);
      }
      if (tail != kNil) {
        av[tail].next = cv[big].head;
        if (cv[big].head != kNil) av[cv[big].head].prev = tail;
        cv[big].head = cv[small].head;
      }
      cv[big].count = count;
    }
    cv[big].mass = mass;
    cv[big].logical_root_fid = lroot;
    if (m_root_key_mutable) {  // mutant : la cle canonique est relue du record physique
      cv[big].canon_fid = av[cv[big].head].fid;
      cv[big].canon_key = av[cv[big].head].key;
    } else {
      cv[big].canon_fid = canon_fid;
      cv[big].canon_key = canon_key;
    }
    cv[small].head = kNil;
    cv[small].count = 0;
    cfree.push_back(small);
    --live_comp;
    return true;
  };

  for (size_t b = 0; b < nb; ++b) {
    const size_t e0 = batches[b].first, e1 = batches[b].second;
    touched.clear();
    constexpr u8 kActive = 1, kAttach = 2;
    const auto touch = [&](u32 fid, u8 bit) -> u32 {
      u32 a = slot_of_fid[fid];
      if (a == kNil) {  // PREMIERE incidence : creneau, alias et composante singleton
        if (free_slots.empty()) {  // impossible si le pic est exact : compte, jamais suppose
          ++stats.slot_overflows;
          return kNil;  // le lot est abandonne AVANT tout usage de cette valeur (refus controle)
        }
        a = free_slots.back();
        free_slots.pop_back();
        av[a] = Alias{};
        Alias& al = av[a];
        al.key = keys[fid];
        al.fid = fid;
        al.last_batch = m_last_shifted && lastb[fid] > 0 ? lastb[fid] - 1 : lastb[fid];
        const u32 c = alloc_comp();
        cv[c].canon_key = al.key;
        cv[c].canon_fid = fid;
        cv[c].logical_root_fid = fid;
        cv[c].head = a;
        cv[c].count = 1;
        cv[c].mass = 1;
        al.comp = c;
        slot_of_fid[fid] = a;
        ++live_alias;
        ++live_comp;
        ++stats.facets;
      }
      Alias& al = av[a];
      if (al.role_epoch != (u32)b) {
        al.role_epoch = (u32)b;
        al.role_bits = 0;
        touched.push_back(a);
      }
      al.role_bits |= bit;
      return a;
    };
    // Toutes les cases LUES sont ecrites juste apres : inutile de repeindre
    // onze cases par evenement (ablation demandee par l'audit).
    if (slot_alias.size() < (e1 - e0) * 11) slot_alias.resize((e1 - e0) * 11, kNil);
    for (size_t e = e0; e < e1; ++e) {
      const ForestEvent& ev = evt(e);
      u32* sa = &slot_alias[(e - e0) * 11];
      for (int s = 0; s < (int)ev.q; ++s) sa[s] = touch(ev_fid[e * 11 + (size_t)s], ((ev.active_mask >> s) & 1u) ? kActive : kAttach);
      for (int z = 0; z < (int)ev.d; ++z) sa[(size_t)ev.q + (size_t)z] = touch(ev_fid[e * 11 + (size_t)(ev.q + z)], kAttach);
    }
    if (stats.slot_overflows) {  // REFUS CONTROLE : rien n'est lu de la sentinelle, rien n'est publie
      r.refusal = "resource_exhausted : creneaux vivants insuffisants (" + std::to_string(nslots) + " alloues, pic exact " +
                  std::to_string(stats.peak_live_exact) + ")";
      r.deltas.clear();
      r.batch_levels.clear();
      if (out_stats) *out_stats = stats;
      return r;
    }
    stats.peak_aliases = std::max(stats.peak_aliases, live_alias);
    stats.peak_components = std::max(stats.peak_components, live_comp);
    stats.logical_live_bytes = std::max(stats.logical_live_bytes, (u64)(live_alias * sizeof(Alias) + live_comp * sizeof(Component)));
    stats.persistent_bytes = std::max(stats.persistent_bytes, (u64)(av.capacity() * sizeof(Alias) + cv.capacity() * sizeof(Component) +
                                                                    free_slots.capacity() * sizeof(u32) + cfree.capacity() * sizeof(u32) +
                                                                    slot_mark.capacity()));
    stats.mapping_bytes = std::max(stats.mapping_bytes, (u64)((firstb.capacity() + lastb.capacity() + slot_of_fid.capacity()) * sizeof(u32)));
    {
      u64 sc = (u64)(touched.capacity() * sizeof(u32) + pre_list.capacity() * sizeof(PreFreeze) + post_list.capacity() * sizeof(u32) +
                     slot_alias.capacity() * sizeof(u32) + (live_in.capacity() + live_out.capacity()) * sizeof(u32) +
                     scratch.capacity() * sizeof(ComponentDelta));
      for (const ComponentDelta& cd : scratch) sc += (u64)((cd.parents.capacity() + cd.born.capacity()) * sizeof(FacetKey));
      stats.scratch_bytes = std::max(stats.scratch_bytes, sc);
      u64 ob = (u64)(r.deltas.capacity() * sizeof(ComponentDelta));
      for (const ComponentDelta& cd : r.deltas) ob += (u64)((cd.parents.capacity() + cd.born.capacity()) * sizeof(FacetKey));
      stats.output_bytes = std::max(stats.output_bytes, ob);
    }
    if (live_alias != live_in[b]) ++stats.life_violations;
    if (free_slots.size() + live_alias != nslots) ++stats.slot_partition_violations;
    // Detecteurs (identiques au resident).
    for (const u32 a : touched) {
      const Alias& al = av[a];
      const bool active = al.role_bits & kActive;
      const bool attach = al.role_bits & kAttach;
      if (!m_no_detector) {
        if (attach && al.seen) ++r.attach_violations;
        if (attach && active) ++r.birth_violations;
      }
      if (attach && !active) ++r.new_attachments;
    }
    // PASSE 1 : gels pre-lot (une entree par composante distincte, cle copiee).
    pre_list.clear();
    for (const u32 a : touched)
      if (m_attach_pre || (av[a].role_bits & kActive)) {
        const u32 c = av[a].comp;
        if (cv[c].post_epoch != (u32)b) {  // post_epoch sert de marque de gel : reinitialisee en passe 2
          cv[c].post_epoch = (u32)b;
          pre_list.push_back(PreFreeze{a, cv[c].canon_key});
        }
      }
    for (const PreFreeze& pf : pre_list) cv[av[pf.rep_alias].comp].post_epoch = UINT32_MAX;
    // PASSE 2 : unions dans l'ordre total des evenements.
    for (size_t e = e0; e < e1; ++e) {
      const ForestEvent& ev = evt(e);
      const u32* sa = &slot_alias[(e - e0) * 11];
      u32 first = kNil;
      for (int s = 0; s < (int)ev.q; ++s) {
        const u32 v = sa[s];
        if (first == kNil) first = v;
        else if (unite(first, v)) ++r.fusions;
      }
      for (int z = 0; z < (int)ev.d; ++z)
        if (unite(first, sa[(size_t)ev.q + (size_t)z])) ++r.fusions;
    }
    // PASSE 3 : deltas par composante post-lot, ordonnes par racine logique.
    post_list.clear();
    const auto post_of = [&](u32 c) -> ComponentDelta& {
      Component& cc = cv[c];
      if (cc.post_epoch != (u32)b) {
        cc.post_epoch = (u32)b;
        cc.post_slot = (u32)post_list.size();
        post_list.push_back(c);
        if (scratch.size() < post_list.size()) scratch.emplace_back();
        ComponentDelta& cd = scratch[post_list.size() - 1];
        cd.parents.clear();
        cd.born.clear();
        return cd;
      }
      return scratch[cc.post_slot];
    };
    for (const PreFreeze& pf : pre_list) post_of(av[pf.rep_alias].comp).parents.push_back(pf.pre_canon_key);
    for (const u32 a : touched) {
      const u8 bits = av[a].role_bits;
      if ((bits & kAttach) && !(bits & kActive)) post_of(av[a].comp).born.push_back(av[a].key);
    }
    std::sort(post_list.begin(), post_list.end(), [&](u32 x, u32 y) { return cv[x].logical_root_fid < cv[y].logical_root_fid; });
    for (const u32 c : post_list) {
      ComponentDelta& cd = scratch[cv[c].post_slot];
      std::sort(cd.parents.begin(), cd.parents.end());
      std::sort(cd.born.begin(), cd.born.end());
      if (cd.parents.size() >= 2) ++r.nodes;
      if (cd.parents.size() == 1 && cd.born.empty()) continue;
      if (m_drop_nonmerge && cd.parents.size() < 2) continue;
      cd.batch = (u64)b;
      cd.level = evt(e0).level;
      cd.output = cv[c].canon_key;
      r.deltas.push_back(cd);
    }
    r.batch_levels.push_back(evt(e0).level);
    for (const u32 a : touched) av[a].seen = 1;
    ++r.batches;
    // MORTS : les alias a leur DERNIERE incidence quittent l'etat ; une
    // composante sans alias est definitivement liberable.
    for (const u32 a : touched) {
      Alias& al = av[a];
      if (al.dead) continue;  // mutant `free-on-absorb` : alias deja jete, jamais libere deux fois
      if (al.last_batch > (u32)b) continue;
      const u32 c = al.comp;
      if (al.prev != kNil) av[al.prev].next = al.next;
      else cv[c].head = al.next;
      if (al.next != kNil) av[al.next].prev = al.prev;
      slot_of_fid[al.fid] = kNil;
      free_slots.push_back(a);  // le creneau retourne a la pile : l'etat reste dans [0, pic)
      --live_alias;
      if (--cv[c].count == 0) {
        cfree.push_back(c);
        --live_comp;
        ++stats.comps_freed;
      }
    }
    // INVARIANT T6 a la frontiere du lot, et EGALITE de vie exacte apres les morts.
    if (!(live_comp <= live_alias && live_alias <= stats.peak_live_exact)) ++stats.invariant_violations;
    if (live_alias != live_out[b]) ++stats.life_violations;
    if (free_slots.size() + live_alias != nslots) ++stats.slot_partition_violations;
    // BALAYAGE STRUCTUREL a cadence bornee (au plus `kStructureScans` fois,
    // plus le dernier lot) : bijection index <-> alias, longueur de chaque
    // liste egale a son compte, aucun cycle ni doublon, aucune composante vide.
    if (nb <= kStructureScans || b + 1 == nb || (b % (nb / kStructureScans)) == 0) {
      ++stats.structure_scans;
      // MARQUAGE EXACT DES CRENEAUX (audit du 28 aout) : l'egalite de
      // cardinalite `libres + vivants == alloues` ne prouve ni la DISJONCTION
      // (un creneau a la fois libre et vivant), ni la COUVERTURE (un creneau
      // perdu), ni l'UNICITE (deux alias sur le meme creneau). Le balayage
      // borne marque chaque creneau et exige exactement une marque par case.
      slot_mark.assign(nslots, 0);
      for (const u32 f : free_slots) {
        if (f >= nslots || slot_mark[f] != 0) ++stats.slot_partition_violations;
        else slot_mark[f] = 1;
      }
      u64 seen_alias = 0;
      for (u32 c = 0; c < (u32)cv.size(); ++c) {
        if (cv[c].count == 0) continue;
        u64 len = 0;
        for (u32 x = cv[c].head; x != kNil; x = av[x].next) {
          if (av[x].comp != c || slot_of_fid[av[x].fid] != x) ++stats.structure_violations;
          if (x >= nslots || slot_mark[x] != 0) ++stats.slot_partition_violations;  // creneau double ou hors bornes
          else slot_mark[x] = 2;
          if (++len > live_alias) {  // cycle ou doublon : la liste depasse le vivant total
            ++stats.structure_violations;
            break;
          }
        }
        if (len != cv[c].count) ++stats.structure_violations;
        seen_alias += len;
      }
      if (seen_alias != live_alias) ++stats.structure_violations;
      for (size_t k = 0; k < nslots; ++k)
        if (slot_mark[k] == 0) ++stats.slot_partition_violations;  // creneau ni libre ni vivant : perdu
    }
  }
  if (live_alias != 0 || live_comp != 0) stats.final_nonempty = 1;  // vacuite finale : toute facette a une DERNIERE incidence
  const auto now = std::chrono::steady_clock::now();
  r.t_reduce_ms += std::chrono::duration<double, std::milli>(now - tmark).count();
  if (out_stats) *out_stats = stats;
  return r;
}

}  // namespace mhgp5

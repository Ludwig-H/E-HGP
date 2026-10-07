# Juges et pilotes des microbancs M2, M3, M4 durcis : changements fichier par fichier

7 octobre 2026, 11 h 50 à 13 h 05 UTC (heures lues avec `date -u`). Copie de travail de `morsehgp3D_v12/microbancs/`
prise à `abe9df451` (sources des microbancs identiques au pin audité `95247cf4b`, vérifié par `git diff`), modifiée dans
`microbancs/` ; la copie intacte est dans `orig/microbancs/` pour les différences. **Rien n'est écrit dans le dépôt** ;
l'intégration (copie, commit, mise à jour de `audits/CONSTATS.md`) revient au développeur principal.

```text
phase=exploration_v12_hors_registre
backend=cpu_reference ; cuda_g4 (banc compilé, non joué)
objet=full_pi0
quantification=quantized_u21_input_only
public_status=not_claimed
GCP non utilisé
```

Règles d'adoption, seuils et statistiques : **inchangés**. Seules changent l'admission des entrées, la présence et la
fraîcheur des preuves, et la réplication (constats `CST-0018`, `CST-0213`, `CST-0214`, `CST-0215` de l'auditeur Codex).
Géométrie et noyaux : inchangés (aucune ligne de `leaf_*.hpp`, `predicates.hpp`, `simt.hpp`, `meb_cert.hpp`,
`welzl_proposal.hpp`, `mes_m3.cpp`, `vidage_v11.cpp`, `common/format.hpp`, `leaf_dump.cpp` ne change ; le noyau et la
contraction de `mes_m4.cpp` non plus).

## MES-M2 (`mes_m2_feuille/`)

| Fichier | Changement | Constat | Test |
| --- | --- | --- | --- |
| `include/mhgp12/leaf/dump_format.hpp` (241 → 457 lignes) | `read()` admet avant toute allocation dépendant des comptes : profil égal à `MHGP12_COORD_BITS` du binaire (sinon refus, jamais une réinterprétation), K dans 1..12, taille de feuille dans K+3..256, `max_leaf` dans taille..256, drapeaux connus avec graphe de paires, comptes dans les domaines `u32` du banc, au moins une feuille, champ réservé nul, **taille exacte du fichier** recalculée depuis l'en-tête ; puis, après l'empreinte, `validate()` : coordonnées dans $[0,2^{B})$, feuilles qui pavent la liste des sites (d'où aucun débordement de début + m, `CST-0215` l. 223), m dans 1..32, boîtes dans $0\leq lo<hi\leq 2^{B}$, sites dans le nuage strictement croissants, statut avec le bit `leaf.cpp`, sans écart du témoin v11 ni bit inconnu, débuts nuls à l'origine, monotones, de total exact, compteurs `emitted`/`incidences` égaux aux plages, enregistrements ($q_{\min}$ 2..4, S* croissant dans la feuille et terminé par 0xFF, $p+m$ dans la feuille, $p+q_{\min}\leq K+1$), populations I et U strictement croissantes, dans la feuille, disjointes, S* dans U, somme des incidences égale à la plage. Champ `digest` (FNV-1a lue) et `digest_hex()`. Le `Writer` reste permissif (il fabrique les témoins invalides). | `CST-0215` | `mhgp12_dump_admission_selftest` : vidage valide admis, 46 mutants à empreinte juste refusés (dont les six de l'auditeur) ; les neuf vidages réels admis |
| `host/dump_admission_selftest.cpp` (nouveau, 339 lignes) | porte native : vidage synthétique valide (sphère de 32 sites de l'auditeur, deux feuilles, émissions de la forme j3) et 46 mutants écrits par le `Writer` original, un invariant chacun ; fichiers tronqué et à octet de trop | `CST-0215` | code 0 ; fichiers rejoués par les outils dans `tests/test_admission_m2.py` |
| `host/leaf_identity.cpp` | admission de **tous** les vidages avant le premier noyau (une ligne `admis:false` et sa raison, code 2, aucune feuille jouée), relecture à empreinte égale, mode `--admission`, `dump_fnv1a` et `dump_leaves` dans chaque ligne, exceptions d'arguments en code 2 (jamais un signal) | `CST-0215` | 46 mutants : code 2, zéro ligne de résultat ; vidage valide puis mutant dans la même commande : code 2, aucun noyau ; réel ng00 K5/24 et K10/24 : identité inchangée (code 0) |
| `host/mes_s.cpp` | même admission en deux passes, `dump_fnv1a` | `CST-0215` | 46 mutants : code 2 |
| `host/arena_selftest.cpp` | cite `dump_fnv1a` ; refuse `feuilles = 0` et une arène sans matière à mutants (code 2, jamais un vert vide) ; arguments illisibles en code 2 | `CST-0018` | 46 mutants : code 2 ; réel : code 0, aucun mutant vivant |
| `cuda/leaf_bench.cu` | admission de tous les vidages **avant le contexte CUDA**, relecture à empreinte égale ; `--nonce` (jeton de la session, caractères contrôlés) répété dans le JSON ; par cas `dump_fnv1a`, `coord_bits`, `sites`, `dump_leaves` ; JSON écrit dans un temporaire puis renommé, écriture incomplète en code 2 (l. 600–606 relevées par l'auditeur) ; exceptions en code 2 | `CST-0018`, `CST-0215` | compilé `sm_120` (nvcc 12.9) sans avertissement hôte (`-Wall -Wextra`), mêmes registres (166 ; 178/168/128 ; 194/168/128) ; sans pilote GPU : refus d'un mutant avant CUDA (code 2) ; non joué sur GPU |
| `CMakeLists.txt` | cible `mhgp12_dump_admission_selftest` | — | construit en `-Wall -Wextra -Wpedantic -Werror` |
| `scripts/g4_leaf_bench.py` (500 → 900 lignes) | contrat écrit (`CONTRACT` : trames ng00–02, configurations 5:16/5:24/10:24, décident 5:24 et 10:24, au moins 5 processus) : tout écart publie la mesure et refuse l'adoption ; jeton de session ; porte d'admission ; identité de chaque vidage (sha256, en-tête, FNV-1a finale) ; admission C++ de tous les vidages avant tout noyau ; cibles de vidage effacées avant chaque vidage, code 0 exigé ; identité hôte exigée (code 0, une ligne par cas et forme de base, couverture et empreinte), sinon refus ; auto-test d'arène rattaché au vidage ; sanitizers exigés (introuvable ou non joué : refus ; preuve = prise neuve de la session sur le bon vidage) ; chaque prise du banc : cible effacée avant, fichier neuf, jeton, chemin, empreinte et comptes du vidage, formes et répétitions exactes, durées finies strictement positives, code cohérent, témoin identique ; isolation du GPU au début, avant et après le banc ; binaires, sources, vidages et dépendances compilées (fichiers `.d`) rehachés en fin de session ; juge : cas attendu absent, aucun cas qui décide, moins de N processus valides sur **tout** cas (et non plus sur les seuls cas qui décident) : refus ; choix seulement avec un rapport fini ; section `evidence` citée par chaque verdict. Statistique, seuil et ordre des tirages inchangés | `CST-0018`, `CST-0215` | `tests/test_juge_m2.py` : 25 scénarios de bout en bout (six injections de l'auditeur, 17 autres preuves manquantes : refusé ; deux échecs de mesure : rejeté ; témoin complet : adopté), trois du juge pur, rejugement des 45 prises réelles du reçu G4 |
| `README.md` | § 1 carte, § 2 admission, § 8 preuves de la prise, § 9 preuves exigées, § 10 portes locales | — | règles de `tools/check_docs.py` rejouées sur la copie : conforme |
| `tests/test_juge_m2.py` (nouveau) | porte du juge et du pilote, opérations externes simulées, vidages synthétiques valides écrits en Python ; partie réelle : `g4_t0a_20261007` (45 prises) | `CST-0018` | `python3 -S -O` (3.10.21) : 30 cas, 0 écart |
| `tests/test_admission_m2.py` (nouveau) | porte native : admission, mutants rejoués par les vrais outils, vidages réels | `CST-0215` | `python3 -S -O` : 5 cas, 0 écart |

## MES-M3 et MES-M4 (`mes_m3_m4_tour/`)

| Fichier | Changement | Constat | Test |
| --- | --- | --- | --- |
| `mes_m4/mes_m4.cpp` (831 → 877 lignes) | à tout ordre k ≥ 2, `FLOWER` **exigée** : absente, refus explicite (ligne `refus`, fin de code 3), jamais un succès ; ordre sans naissance : refus ; à k ≥ 2, une naissance non jugée par `LEM-T6` interdit le code 0 ; taille de `FLOWER` fausse : refus (et non plus exception) ; admission des entrées : genres, ordres, K, trame et profil concordants entre fichiers, clés de naissance strictement croissantes et dans leur domaine (sites à k = 1, boules ensuite), boules des cellules dans le catalogue, `CELLOFF` nul à l'origine et croissant ; le refus `CST-0212` reste le premier contrôle | `CST-0214` | `tests/test_m4_preuves.py` sur le carré de l'auditeur : correct 0 (six naissances jugées), image fausse 1, `FLOWER` absente aux ordres 2–4 ou au seul ordre 3 : 3, cinq entrées hors domaine : 3, mutant sans contraction : 1 ; binaire d'origine : absente → 0, boule de cellule hors catalogue → **SIGABRT** (AddressSanitizer : écriture hors tableau, `mes_m4.cpp:779` au pin) ; réel ng00 K5 et ng01 K10 : code 0, toutes les naissances des ordres 2..K jugées |
| `pilote.py` (596 → 1 395 lignes) | **provenance** : `construire` hache les cinq binaires (liste `BINAIRES` enfin utilisée), `libmhgp11.a` et les dépendances compilées (`.d`) ; chaque exécution hache le binaire lancé avant et après (égal à `construire`, sinon refus), vérifie les empreintes des vidages de `vider` avant et après, écrit son journal sous un nom de campagne (`c<date>_<hasard>`) et en garde l'empreinte ; sources du microbanc hachées au début et à la fin de chaque invocation ; un bloc remplacé passe dans `historique` ; **preuves** : `lire_vidage` exige toutes les sections (dont `FLOWER` à k ≥ 2) ; validateurs purs `valider_vidage`, `valider_m3` (entrée, un ordre par k = 2..K aux parties du vidage, routes complètes, fin au code du processus, au moins une partie), `valider_m4` (comptes égaux au vidage, `LEM-T6` sur toutes les naissances à k ≥ 2, contraction parallèle identique), `valider_variante`, `valider_mutant_m4` et `valider_porte` (un mutant n'est tué que par sa réponse géométrique) ; **réplication** : étape `resolution` (`--processus` processus neufs par cas, chacun réécrit les vidages, exigés identiques à l'octet à ceux de `vider` puis effacés ; campagnes ajoutées, jamais écrasées ; minimum de R passes identifié comme tel) ; **juges** : `juger_m3` (règle écrite, moyenne géométrique des rapports par processus, IC 95 % bootstrap 10 000 tirages graine fixe, adopté si borne haute ≤ 0,60 sur ng00/01/02 à K10 avec au moins 5 prises, refusé sinon faute de preuve, rejeté par la mesure ou l'identité ; preuves citées) et `juger_m4` (conformité, temps publiés et comparés sans adoption automatique) ; `--chrono-vidage` (prise informative, désormais `non` par défaut) ; codes 0/1/2/3 | `CST-0018`, `CST-0213`, `CST-0214` | `tests/test_pilote.py` : 32 cas (sorties réelles du reçu G4, injections de l'auditeur, juge de MES-M3, binaires réels) ; bouts en bout locaux u8000 K5 et ng00 K5 |
| `README.md` | § 1 carte, § 2 preuves et codes, § 4.3 `FLOWER` exigée, § 5.4 réplication et juge, § 6 preuves exigées, § 7 plan G4 et portes | — | règles de `tools/check_docs.py` : conforme |
| `tests/carre.py` (nouveau) | générateur du carré K1..4 repris du reçu de l'auditeur (`check_dumps.py`), partagé | — | — |
| `tests/test_m4_preuves.py` (nouveau) | porte native de MES-M4 | `CST-0214` | 10 cas, 0 écart |
| `tests/test_pilote.py` (nouveau) | porte du pilote | `CST-0018`, `0213`, `0214` | 32 cas, 0 écart |

## Outils et index

| Fichier | Changement | Test |
| --- | --- | --- |
| `outils/mutants_juges.py` (nouveau) | 22 mutants causaux (10 du pilote M2, 10 du pilote M3/M4, 2 natifs de MES-M4), chacun une substitution unique dans une copie temporaire, rejoués par leur porte | 22 tués, 0 vivant |
| `README.md` (index) | état des deux microbancs et de l'outil | — |

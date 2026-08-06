# Vérification réduite : la route vers 50k exhaustif exact < 100 ms

Directive scellée du 6/8/2026 : une fois les tests verts (fait : 252/254
conteneur), l'objectif est **50k points, exhaustif et exact, sous 100 ms**
sur G4. Le profil mesuré en session c3 (run natif n=32/K=10 : 99,8 % CPU
mono-thread, GPU en rafales quasi inactives, > 10 min) désigne le goulot
sans ambiguïté : la couche de certification — le rejeu CPU frais de chaque
transition par la session ancrée — pas le calcul géométrique device.

## Modèle de confiance actuel (M2, ④-b2)

- Le moteur device M5b est qualifié **bit-identique au fake certifié** au
  seam (test du slot engine partagé, sans GPU) et ses six cas no-go n=32
  sont résolus en clôture BigInt exacte (40 920 par cas).
- Le pont M2 valide en `exact::BigInt` la partition des masses après
  chaque commit : R_j + C(F_j) = C(n,3) + C(n,4).
- La session ancrée re-rejoue CHAQUE transition sur CPU
  (`commit_prepared` → reconstruction du chunk hôte → égalité totale).
  C'est ce rejeu, séquentiel et en O(travail total), qu'il faut retirer.

## Le nouveau chemin : commit certifié par tuile

Un mode de commit `tile_certified` de la session ancrée (ou d'une session
sœur dédiée), où le candidat device est accepté sous :

1. **Identité de source** : le checkpoint réinjecté est le checkpoint de
   confiance courant (inchangé) ;
2. **Comptabilité de frontière** : les entrées consommées par la tuile
   correspondent exactement au préfixe de frontière du checkpoint, et les
   entrées résiduelles/scindées re-rentrent avec conservation de masse
   BigInt (déjà la validation croisée du pont, déplacée dans la session) ;
3. **Bonne formation O(sortie)** : records canoniques (ordre, arités,
   bornes d'index), diagnostics bien formés, comptes d'audit délta
   cohérents (identités par commit) ;
4. **Chaîne de sortie** : le digest de chaîne est replié sur les records
   CANDIDATS (`extend_output_chain`, O(records)) — la chaîne reste la
   même définition unique qu'en ④-b2 ;
5. **Clôture terminale exacte inchangée** :
   `total_support_count == C(n,3)+C(n,4)` en BigInt, frontière vide,
   partition groupée certifiée.

Ce qui n'est PLUS re-dérivé sur CPU : la vérité géométrique de chaque
record (centre/niveau/intérieurs), désormais portée par (a) la
qualification M5b du moteur (seam bit-identique, arithmétique rationnelle
exacte int512 on-device), (b) la clôture BigInt (aucun support perdu ni
compté deux fois), (c) le différentiel exhaustif n=32 sanctionné (chemin
rejeu ≡ chemin tuile-certifié, localement avec le fake), (d) le garde-fou
structurel Delaunay externe à échelle
(`DELAUNAY_STRUCTURAL_GUARD_DESIGN.md`).

## Honnêteté du certificat

Le certificat de chaîne ancrée gagne un champ `verification_basis` :
`fresh_cpu_replay_every_commit` (chemin actuel, inchangé) ou
`tile_certified_engine_with_exact_closure`. La façade recopie ce fait dans
son certificat ; la base de preuve et les nonclaims TOML déclarent que le
chemin tuile-certifié s'appuie sur la qualification M5b du moteur, pas sur
un rejeu par transition. Aucun statut public ne change.

## Point d'implémentation critique

Dans le pont actuel, `prepare_next` de la session EST le générateur hôte
du chunk (le device ne sert qu'à la validation croisée) : le CPU fait
tout le travail même sans rejeu. Le chemin tuile-certifié doit donc
inverser le flux : les payloads de tuile device deviennent le candidat
(assemblés en `ExactHigherSupportStreamChunk` par l'assembleur), et la
session les valide par les invariants 1–5 sans jamais exécuter son propre
générateur. C'est le vrai retrait du O(travail) CPU.

## Au-delà du higher : le bilan 100 ms

Le contrat 100 ms exige qu'AUCUN étage ne reste séquentiel en O(univers) :

- LBVH : build device (déjà).
- Paires P8l : session CPU séquentielle aujourd'hui — à porter en tuiles
  device sous le même modèle (validation croisée + clôture n² exacte),
  incrément séparé.
- Higher 3/4 : ce design.
- Façade + journaux + reducer + archive : O(sortie) — à paralléliser
  (48 cœurs) et pipeliner avec les tuiles ; l'archive 15L est déjà en
  flux.

Ordre des incréments : (R1) commit tuile-certifié + différentiel n=32
rejeu ≡ tuile-certifié ; (R2) mesure G4 (n=32 natif attendu en secondes,
ventilation 50k) ; (R3) paires device-tuilées ; (R4) parallélisation aval ;
(R5) boucle mesure→optimisation jusqu'à < 100 ms, avec le garde Delaunay
aux jalons.

## Raffinement mesuré (lecture du pont, 6/8 soir)

La recherche de budget minimal du pont (doublement + dichotomie par
transaction) appelle `anchored_prepare` — le GÉNÉRATEUR CPU de la session —
à chaque sonde : le CPU reconstruit le chunk O(log budget) fois PAR
transaction avant même le rejeu de commit. Le chemin tuile-certifié doit
donc supprimer à la fois le rejeu de commit ET la recherche de frontière :
la tuile device EST la transaction (le moteur M5b résout les racines du
suffixe arrière et rend records + masses par racine) ; l'assembleur
synthétise le `ExactHigherSupportStreamChunk` candidat depuis le payload
device (frontière successeur = préfixe strict, déltas d'audit, records) et
la session le valide par les invariants 1–5. Plus aucun appel au
générateur hôte, plus aucune sonde de budget : les budgets par transaction
disparaissent de ce chemin (le certificat déclare des tuiles variables,
base `tile_certified_engine_with_exact_closure`).

## Spécification du régime d'audit tuile-certifié (lecture complète du bloc d'identités)

Le bloc `audit_valid` (higher_support_stream.cpp l.≈1395-1490) se
décompose en trois classes, et le chemin tuile-certifié peut satisfaire le
bloc EXISTANT sans nouvelle clause :

- **A. Identités de partition de masse (BigInt)** — total == C(n,3)+C(n,4),
  resolved + remaining == total, remaining == somme des masses de
  frontière, resolved == terminal_sum, positivité, drapeaux de clôture :
  fournies par les masses de l'audit device (deltas par tuile, clôture
  BigInt par commit — la validation croisée du pont déjà en place).
- **B. Identités records/chaîne (O(sortie))** — emitted_record_count,
  output_record_count du checkpoint, digest initial, et les identités de
  classification de feuilles (leaf_analysis, leaf_classified == identité
  par catégories, minimal) : recomputées depuis les RECORDS drainés
  eux-mêmes (chaque record porte sa catégorie : minimal/affinement
  dépendant/bord réduit/circumcentre extérieur/au-dessus du rang) — le
  moteur device, transcription bit-identique du classifieur CPU, produit
  les mêmes catégories. La masse leaf_classified égale le compte de
  records car la classification est à granularité d'un support.
- **C. Compteurs de travail CPU** — visites, sondes, témoins : à ZÉRO sur
  ce chemin ; toutes leurs contraintes sont des égalités 0==0 ou des
  inégalités 0≤0 (work_unit == visits + witness, attempts == searches,
  attempts + skips ≤ visits, etc.), plus les maxima de frontière
  maintenus côté hôte (running max, trivial).

La synthèse par tuile est donc : audit successeur = audit courant +
{masses device, compteurs de catégories dérivés des records drainés,
zéros de travail, maxima}, checkpoint successeur = {frontière préfixe
(+ ré-entrées scindées), pas de pending, chaîne repliée sur les records,
compte de records}, et `verify_exact_higher_support_checkpoint` vaut tel
quel. L'honnêteté reste explicite : champ `verification_basis` du
certificat (les zéros de travail en sont d'ailleurs le témoin interne).
Le différentiel n=32 compare records/masses/complétude — PAS l'égalité
totale d'audit (les compteurs de travail diffèrent par construction).

## Le vrai budget hôte, mesuré (R1-c, 6/8 nuit)

Le design ci-dessus supposait que le retrait du rejeu CPU par transition
laissait un reste O(sortie) négligeable. La mesure dit autre chose, et
elle recadre la route :

| opération exacte | avant R1-c | après R1-c | cible |
| --- | --- | --- | --- |
| `classify_sphere_point` | 683 µs | 1,0 µs | ~0,05 µs (filtre fp64) |
| requête closed-ball (n=4096) | 56 741 µs | 276 µs | ~20 µs |
| `analyze_circumcenter_support` (4) | 7 184 µs | 732 µs | ~10 µs |

Les deux causes fermées par R1-c sont (i) le PGCD d'Euclide naïf appelé
par **chaque** opération rationnelle normalisée, et (ii) la normalisation
elle-même, inutile pour toute décision de traversée
(`exact::ExactHomogeneousSphere3`, arithmétique entière homogène).

La conséquence pour le contrat : à 50 000 points, l'étage higher hôte
paie une classification closed-ball et une analyse de circumcentre **par
terminal minimal drainé**. Tant que l'analyse de circumcentre coûte
732 µs, 10^5 terminaux coûtent 73 s sur un cœur, soit 1,5 s sur 48 cœurs
— toujours quinze fois le contrat. L'ordre des incréments devient donc :

- **R1-d** : `analyze_circumcenter_support` en déterminants entiers
  (circumcentre homogène par Cramer, signes barycentriques comme signes de
  déterminants), la forme réduite n'étant construite qu'une fois, pour le
  payload de l'événement réellement émis.
- **R1-e** : filtre par intervalles fp64 (`exact/fp64_interval.hpp`) en
  tête des décisions point-contre-sphère et boîte-contre-sphère, l'entier
  restant l'autorité en cas d'incertitude.
- **R2** : mesure G4 (le natif n'est mesurable qu'après R1-d, sinon on
  mesure l'arithmétique et pas la géométrie).
- **R3/R4/R5** : inchangés (paires device-tuilées, parallélisation aval,
  boucle jusqu'à < 100 ms).

Note de méthode : ces coûts unitaires n'étaient visibles ni dans les
suites (toutes sur de tout petits nuages) ni dans le profil G4 (qui
attribuait 99,8 % à « le CPU », sans dire que le CPU passait son temps
dans un PGCD). Toute cible d'échelle future doit commencer par le coût
unitaire mesuré des étages exacts.

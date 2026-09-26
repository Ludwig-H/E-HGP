# Phase A de la tour : mesurer, puis alléger tous les ordres (proposition C)

26 septembre 2026, auditeur C.

- **Base :** `f44a8db03` ; le code de la phase A est identique de R18 à
  R22.
- **Cadre :** `exploration_v9_hors_registre`, `public_status=not_claimed`.
- **GCP :** non utilisé.
- **Méthode :** une piste de spécification et une piste de profil local,
  puis trois sceptiques :
  - sûreté pour l'objet de chaque levier ;
  - preuve de la parallélisation P1 ;
  - modèle de coût et priorités.

  Deux vérificateurs ont ensuite relu cette note.
- **Annexe :** [`c_phase_a_20260926/`](c_phase_a_20260926/README.md).

**Proposition, non implémentée. Elle repose sur le travail d'autres
acteurs :**
- **Le code actuel de la phase A est celui du développeur :**
  - la phase A allégée E3 : données de bloc préparées en parallèle,
    racines préchargées, ajouts réservés ;
  - `aa29245f` : rangs exacts `level_run`, voie rapide des lots
    singletons.

  L1 et L2a + L7 prolongent E3.
- **L'analyse de la phase A et le parallélisme sont de B :**
  - [lots et parallélisme](PHASE_A_FULL_LOTS_ET_PARALLELISME_20260923.md),
    dont le pré-niveau figé et le budget partagé de fils ;
  - [lemme du maximum d'ID](PHASE_A_MAX_ID_COMPOSANTE_20260923.md) ;
  - [graphe temporel](PHASE_A_GRAPHE_TEMPOREL_20260923.md), attribution
    probable à B, qui y rapporte l'ablation négative des têtes physiques ;
  - le relevé de comptes `phase_a_20260923/`, attribution probable à B.

  Le rejet de P0, les parts de lots groupés et la reconstruction P2 en
  viennent. B avait aussi relevé les fils ouverts par chaque coureur
  dans sa [contre-lecture du pool E2](CONTRE_AUDIT_B_POOL_E2_WIP_20260924.md)
  (L8).
- **L'étape « phase A des ordres bas » du plan du développeur était
  juste.** Je lui avais demandé de la retirer le 26 septembre, puis j'ai
  retiré ce conseil ([contre-lecture R22](CONTRE_AUDIT_C_R22_20260926.md),
  § 3).

## 1. Pourquoi tous les ordres

**K5 avec sol.**
- Un levier sur A(5) seule rapporte au plus son avance sur K4 : 37 à
  53 ms.
- Un gain constant de c ms sur chaque A(K) rapporte c ms.
- Une réduction de la même **fraction** sur chaque A(K) laisse A(5)
  borner jusqu'à environ −50 % (53 % à b00). Au-delà, K3 et K4 bornent à
  leur tour ; à b00 et A ÷2, les fins valent 243,1 / 240,0 / 236,8 ms.

**K10.**
- Sans sol, la phase 0 (71 à 82 % de la fenêtre) rapporte le plus.
- Avec sol, les phases A rapportent le plus : ÷2 donne −257 à −321 ms,
  contre −189 à −229 ms pour la phase 0.
- Une fois la phase 0 divisée par 2, A(10) borne dans 6 cas K10 sur 7.
  C'est une projection qui suppose A(K) inchangé, ce qui n'est pas acquis
  à K10 (§ 2).

## 2. Ce qu'on sait du coût, et ce qu'on ne sait pas

**Murs G4 de R22, rapportés aux comptes d'objet du relevé de B** (même
entrée `0baa4de1` ; les sommes égalent le `tower_work` de R22) :
- **Sans sol, à 00, K2 à K5 :** 179 à 191 ns par bloc et 107 à 124 ns
  par facette.
- **À K5, sur les six trames :** 247 à 287 ns par nœud.
- **Avec sol :** 10 à 15 % de plus par facette, tout compris.
- **Lots groupés, trame 00** (parts non mesurées sur le brut) : 19,2 %
  des blocs d'un build K5 ; par ordre, K1 61 %, K2 45 %, K3 22 %, K4
  13 %, K5 8 %.

**Stabilité.**
- Entre R20 et R22, A(5) (148 contre 148–150 ms) et A(2) ne bougent que
  de quelques pour cent.
- Les autres ordres restent dans environ ±10 % : A(4) est plus élevé de
  12 % en médiane en R20.
- À K10, dans R21, A(10) est 33 à 42 % plus lent avec les trois leviers
  de la tour actifs, et A(8) et A(9) de 16 à 33 %. La comparaison est
  confondue : la phase 0 du bras sans les leviers est 3 à 4 fois plus
  longue.

**Non établi : quelle hypothèse borne A(K).** Il n'existe ni compteur
matériel, ni sous-chrono de la boucle, ni ablation de la boucle
actuelle. Les hypothèses restent ouvertes :
- **H1, latence mémoire.** Environ 15 accès aléatoires par bloc, dont
  environ 10,5 non préchargés. Ce sont des accès, pas des défauts de
  cache mesurés.
- **Défauts de page de premier contact.** Entre 0 et 25 % de A(5) ;
  au moins 6 800 pages à 00 ordre 5, et environ 15 000 au brut ordre 5,
  venant de la réservation des niveaux de lot au-delà du seuil `mmap`.
- **Allocations des lots groupés.**
- **H4, mauvaises prédictions de branche.** Elles réduiraient le
  parallélisme mémoire et s'accordent aussi bien avec les données.
- **H6, coût fixe par ordre.** `anchors.assign` écrit 4 octets par boule
  à chaque ordre, soit 10 à 25 % de A(2).

## 3. Leviers d'allègement, conditions de sûreté vérifiées

Aucun levier n'est « sans risque » sans condition. Chacun garde l'objet
(condensé, IDs, raison et priorité des refus, `tower_work`) seulement
sous les conditions suivantes. Elles ont été vérifiées par lecture du
code et par des modèles réduits, pas par des portes du produit.

| levier | effet attendu (inférence) | conditions de sûreté |
| --- | --- | --- |
| **L2a + L7** : compléter le préchargement (lignes du catalogue, `level_run` des positions et des cibles, un troisième étage pour `compressed`) | 0 à 25 % de A (0 à 90 ms à b02) ; sert aussi d'**ablation d'attribution** | indices bornés (former `&x[i]` au-delà de la fin est un comportement indéfini) ; la valeur préchargée n'est qu'une adresse, et l'ancre consommée est relue |
| **L1** : c, m, ι pris dans la collecte de la phase 0, `order_prepare_lean` supprimé pour K ≥ 2 | −1 lecture du catalogue par bloc ; −192 fils créés sur 409 à K5, −432 sur 889 à K10 | `lean_failed` est inatteignable pour K ≥ 2, à la lecture du code, sous réserve que `ShellTable::rank()` lève de la même façon aux deux sites. c, m et ι doivent venir **de la même visite**. Livrer ces tableaux par le même échange que `static_targets`. Garder les compteurs par bloc avant les contrôles, car `tower_work` est publié même en cas d'échec ; **aucune porte ne compare aujourd'hui `tower_work` après un échec au milieu de la boucle**, et il en faut une si les indicateurs sont sommés. K1 garde `order_prepare_lean`. Recibler le mutant `COUNT_SKIPS_FACET` |
| **L2b** : un `BallId` par lot, et le niveau exact matérialisé avant l'encodeur | environ −1,1 ligne de catalogue par bloc (environ 1,5 par lot publié) ; −44 octets de mémoire fraîche par lot | stocker exactement `program[begin]`, car `ExactLevel` n'est pas réduit et le condensé hache num/den ; sentinelle pour le lot zéro de K1 ; contrôles de l'encodeur sur les valeurs matérialisées |
| **L5** : lots groupés sans allocation (tampons par coureur, u32, groupes en CSR par tri par comptage sur le représentant minimal) | surtout à K3 et K4, qui bornent au-delà de −50 % | garder l'ordre des groupes par position minimale, l'ordre des blocs, les compteurs avant la boucle et les ancres écrites après tous les groupes ; vérifié sur 20 000 lots aléatoires |
| **L6** : premier contact des tableaux par le coureur pendant qu'il attend `ready[K]` (environ 72 ms pour le coureur 5 au brut K5), puis THP après lecture du mode sur G4 | 0 à 25 % selon la part de pages fraîches ; à décider après une mesure des défauts de page par fil | tableaux dimensionnés (`RawVector`), jamais d'écriture au-delà de `size()` ; espaces réutilisés entièrement remis à zéro ; ce premier contact entre en concurrence avec la phase 0, qui décide de `ready[K]` |
| **L4** : jetons de nœud en u32 dans la phase A | moitié du trafic des tableaux de nœuds | décalages de CSR en u64 ; sentinelle traduite dans un `MonotoneHistoryOf` générique ; `lower_nodes` et l'image de K1 restent en u64 ; le `FullCoverageFlatDraft` public reste en u64 |
| **L3** : positions de programme denses (`pos_of` par ordre) | −2,7 lectures aléatoires de `level_run` par bloc | la phase 0 ne doit pas refuser une cible hors programme ; une cible t ≥ \|balls\| donne `not_strict` ; `pos_of` de K doit vivre jusqu'à la phase C de K+1 ; K1 a besoin de son propre `pos_of` ; la raison `vertical_birth_anchor` doit être redérivée ; même traduction sur les voies hachée et triée. La réduction de mémoire annoncée à K10 est **réfutée** |
| **L10, L12** : compteurs regroupés, décompte des vivants fusionné | petit | **piège** : sur un échec, `tower_work` publie le travail fait, et les sommes regroupées donnent un autre nombre (porte nécessaire). La formule fusionnée ne doit porter que sur les actions qui créent un nœud, sans ajouter n0. Elle ne contrôle plus `next` |
| **L11** : `RawVector` pour les tableaux de lot | petit | `reserve_lean` ne doit sommer que `[0, lean_failed)` ; le remplissage parallèle des ancres doit finir avant la boucle et avant la phase C |
| **L13** : un tampon plat de racines u32 par lot | petit | garder le tri et la déduplication par bloc ; remettre les décalages à zéro à chaque lot ; conditions de L4 |
| **L9** : table site → indice du domaine pour K1 | A(1) ne borne jamais | seuls `point_id` et `lower_bound` disparaissent ; garder l'ordre des facettes, la garde de taille et la borne `min(n, lean_failed)` ; si la boucle est parallélisée, l'échec reste celui de la plus petite position |
| **L8** : pas de pool imbriqué depuis les coureurs et les aides (constat de B) | petit à K5 ; 217 des 409 fils viennent de la phase A, 192 de la phase B | **risque moyen** : l'exception « première capturée » de `parallel_items` n'est pas déterministe, donc admissible seulement pour des exceptions de ressource ; la vivacité, les interblocages et le comportement sous TSan d'un pool à plusieurs soumetteurs ne sont pas analysés ; voir le défaut du pool relevé par B |

## 4. Parallélisme

- **P0 (parallélisme dans un lot) : rejeté**, comme l'avait conclu B.
  À l'ordre le plus haut, les lots comptent en moyenne 1,05 bloc (ordre
  5 d'un build K5) et 1,01 (ordre 10 d'un build K10).
- **P1 (indices validés, validation séquentielle)** : sain en principe,
  mais pas tel que formulé.
  - **L'invariant tient.** Toute valeur écrite dans `compressed[x]` est
    un ancêtre de x ou x lui-même, pour le reste de l'ordre (écrivains
    aux lignes 1154, 1166 et 1167 ; `next` n'est écrit qu'une fois).
  - **Un indice accepté si `indice < prior_count`** donne un objet
    identique à l'octet près, si l'indice est sur la chaîne de l'ancre.
  - **Conditions nécessaires :**
    - `atomic_ref` sur les écritures du validateur (1154, 1167, 1301,
      1350) ;
    - un compte de nœuds validés publié en release et lu en acquire, ou
      des écritures de nouvel ID en release **et** des lectures en
      acquire côté auxiliaire ;
    - l'auxiliaire borne `cible < anchors.size()` avant de lire ;
    - il lit par des pointeurs bruts capturés après la réservation,
      jamais par `size()` ni `operator[]` ;
    - il est lancé après la réservation, arrêté et joint avant les
      libérations et à chaque exception ; un échec de lancement donne
      « pas d'indice » ;
    - une case par ordinal de facette (5,4 Mo à K5, 15,3 Mo à K10) ;
    - 1267 et 1268 sont conservés dans le validateur ;
    - les compteurs d'indices restent des métadonnées, jamais dans
      `tower_work` ; l'auxiliaire est compté dans un champ à part, dans
      les deux bras de la porte du pool.
  - **Pourquoi des atomiques relâchés ne suffisent pas.** Avec
    `std::vector`, lire une case non construite est un comportement
    indéfini. Avec un tableau prédimensionné, l'indice peut lire le 0
    initial : la simulation donne 932 indices hors chaîne et 293
    exécutions corrompues sur 2 000.
  - **Test `prior_count`.** Il ne se déclenche jamais en exécution
    séquentiellement cohérente : le mutant qui le retire survit aux
    exécutions naturelles. Il faut une porte d'indices forcés, comparée
    par `same_payload`, `same_work` et le condensé, avec ses mutants et
    TSan.
  - **Gain non vérifiable.** Le validateur garde au moins quatre lectures
    aléatoires par facette pour conserver l'ordre des refus.
- **P2 (hors ligne : MSF par Borůvka, arbre de Kruskal contracté, IDs
  par le lemme du maximum d'ID de B)** : après E6 et une phase 0 divisée
  par 2.

## 5. Mesurer d'abord

**Ce que fait le correctif** (`c_phase_a_20260926/phaseA_profile.diff`).
Il s'applique sur `main`. Il compile sous `-Werror` avec GCC 13.3 (cible
de la sonde seulement ; non essayé avec GCC 11.4, celui de G4). Il
ajoute :
- le temps CPU de fil pour l'ordre entier, la préparation et la boucle ;
- des tics rdtsc, c'est-à-dire du temps mural, pour le remplissage des
  ancres, la mise en place de K1, la réservation, la fin et le décompte
  des vivants ;
- un échantillonnage rdtsc des lots (un sur 16). Les lots au-delà d'un
  seuil de tics sont écartés comme présumés préemptés ; la vraie
  correction de la préemption est la mise à l'échelle par le temps CPU
  de fil ;
- des compteurs **toujours actifs** : lots, blocs, facettes, appels et
  pas de `order_root`, lots groupés, unions, nœuds. Le surcoût rdtsc vaut
  12 à 17 % des tics des lots échantillonnés.

Un essai local sur les 1 500 sites du préflight de R22 a vérifié le
fonctionnement : il imprime une ligne `PROFA` par ordre et les
condensés restent ceux du préflight. Les lignes `PROFA` sortent sur
stderr et ne gênent pas le lecteur de la session.

**En local d'abord.** Les compteurs sont déterministes et indépendants
de l'hôte. Leur relevé sur 00 et b00 à K5 est en cours, malgré la
charge.

**Sur G4 ensuite**, pour les temps absolus, les défauts de page et THP.
Il reste du code à écrire, côté sonde et sur stderr :
- `getrusage(RUSAGE_THREAD)` par coureur ;
- la lecture de `/sys/kernel/mm/transparent_hugepage/*` ;
- une boucle calibrée pour la fréquence réelle des cœurs (question
  ouverte) ;
- les compteurs derrière un levier ou une option de compilation.

La variante « préchargement seul » demande un levier (sonde, worker,
schéma v29) ou un second paquet.

**Plan de session :**
- paires répétées et entrelacées dans le même bras, avec un témoin non
  instrumenté ;
- les jumeaux moteur que le validateur exige ;
- le bras `tower_overlap_static=0`, discriminant gratuit ;
- un `perf stat` préliminaire si la VM expose un vPMU.

Environ 10 min de VM.

**Deux voies :**
- **Voie A (préférée, parcimonieuse).** La sonde v29 ajoute ces mesures,
  ainsi que les sous-chronos du recensement et le mur externe par bras
  demandés pour R22. R23 couvre alors tout, si la v29 contient aussi le
  levier de préchargement, `getrusage`, THP et l'horloge, et des
  compteurs derrière un levier.
- **Voie B.** Une session C dédiée, par le même pipeline gardé, depuis
  un commit de `main` qui contient l'instrumentation. C'est au
  développeur de décider si et où elle entre dans l'arbre.

## 6. Ordre recommandé

1. **Mesurer** (§ 5), puis choisir sur les chiffres.
2. **Premier palier :**
   - L2a + L7, qui sert aussi d'attribution ;
   - L1, avec sa porte `tower_work` après échec si les indicateurs sont
     sommés ;
   - L6, après une mesure des défauts de page par fil ;
   - L2b ;
   - L5.
3. **Second palier :** L10 à L13 avec leurs pièges, puis L4, puis L3
   seulement si les lectures de `level_run` pèsent.
4. **P1**, avec son protocole mémoire, sa porte d'indices forcés et TSan.
5. **L8** avec des portes de pool (vivacité, TSan), puis **P2** après E6.

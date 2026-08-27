# État courant — audit de MorseHGP3D v5

- **Date :** 27 août 2026
- **Auditeur :** Codex
- **Pin fonctionnel audité :** `987a18c8dbc51435f638b56719e5eb635b3fa830`
- **Reçu G4 le plus récent :** [`campagne_g4_v5_20260827_lane_q4_device`](../receipts/campagne_g4_v5_20260827_lane_q4_device/RECU.txt), source `2e75cb42c36c72ed90f931e0dbf49980e669d1d1`
- **Cadre :** `phase=exploration_v5_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`
- **GCP :** non utilisé par l'auditeur ; la cible Spot de Claude est certifiée `TERMINATED` dans le reçu

## Verdict

La v5 est **orange et progresse nettement**. Le pipeline CPU horizontal, ses
oracles bornés, ses mutants et les lanes Q3/Q4 par lots passent une porte
Release complète et propre au pin audité. Une vraie exécution Blackwell a aussi
établi les lanes Q3 et Q4 device sans désaccord sur trois tailles bornées, puis
l'égalité CPU/GPU de bout en bout sur deux familles à 50 000 points.

Ce résultat ne ferme pas encore la ligne GPU. Les deux familles denses ont
épuisé la mémoire de la G4 au pin reçu ; les limites ajoutées depuis sont des
seuils contrôlés **après une ancre entière**, pas des plafonds de résidence.
L'exécuteur adaptatif de `8f95df2e` et le précomptage Q4 de `987a18c8` sont
prometteurs, mais n'ont aucun reçu G4. Enfin, le protocole peut déclarer un run
adaptatif recevable sans prouver que les routes hôte et device ont réellement
été exercées.

Il n'y a pas de P0 CPU reproduit. Il reste trois ensembles de P1 localisés :
capacité avant matérialisation, intégrité/autorité des résultats Q4, et
non-vacuité du prochain reçu. Ils peuvent être fermés sans réécrire les
oracles ni les étages mathématiques shaped.

## Avancement et résultats actuels

| Périmètre | Résultat effectivement établi | Limite du résultat |
|---|---|---|
| CPU au pin `987a18c8` | build Release propre ; **156/156** tests `gate`, dont **7** étiquetés `oracle`, en 226,25 s réelles | pas de campagne `scale*` rejouée par cet audit |
| Protocole au même pin | `selftest_campagne_v5.sh` : `violations=0`, `PROTOCOLE CONFORME` | le faux pilote ne falsifie pas encore un routage adaptatif ignoré |
| Registre | **20 phases** et portes validées par `check_implementation_status.py` | aucune phase v5 formelle ouverte ; registre inchangé |
| Q3 device reçu | 3,37 M, 25,65 M et 35,43 M seeds ; zéro désaccord vectoriel ou de compteurs | source reçue `2e75cb42`, pas le tip actuel |
| Q4 device reçu | 2,33 M, 5,97 M et 22,50 M seeds ; jusqu'à **157 485 218** complétions ; zéro désaccord | source reçue `2e75cb42`, pas l'adaptatif |

Sur les contrats 50 000 reçus :

| Famille | CPU | GPU | Égalité observée | Verdict |
|---|---:|---:|---|---|
| `uniform` | 78 s ; 19,96 Go RSS | 89 s ; 19,01 Go RSS | `digest_balls` et `digest_all` identiques | conforme mais GPU plus lent |
| `terrain` | 23 s ; 3,64 Go RSS | 42 s ; 29,28 Go RSS | `digest_balls` et `digest_all` identiques | conforme mais GPU plus lent et plus résident |
| `eight_clusters` | 247 s ; 17,56 Go RSS | code 134 ; 119,63 Go RSS | aucun digest GPU terminal | `cudaMalloc` hors mémoire |
| `scanline_single_pass` | 38 s ; 3,13 Go RSS | code 134 ; 97,96 Go RSS | aucun digest GPU terminal | `cudaMalloc` hors mémoire |

Le reçu est donc correctement marqué `partial_or_failed`. L'égalité des deux
digests a été vérifiée directement dans les sorties `uniform` et `terrain`,
mais le validateur de campagne n'impose actuellement que `digest_all`.

Depuis ce reçu, Claude a bien fermé plusieurs défauts concrets : validation
structurelle des lots, propagation des exceptions après jonction, réserves
device par temporaires, dénominateur exact strictement positif, limites
`seeds/sites/pairs`, distinction des lots et kernels, routage adaptatif et
précomptage des seeds Q4 avant toute matérialisation. Le précomptage évite
utilement de mettre une ancre sans seed dans un lot. Ces corrections sont
confirmées par les portes CPU du pin ; elles ne doivent simplement pas être
surinterprétées comme un plafond mémoire ou une preuve Blackwell.

## P1 — fermer la capacité avant le prochain reçu G4

Dans [`q4_lane_batched.hpp`](../src/gpu/q4_lane_batched.hpp), une ancre entière
est encore copiée dans les SoA, puis `pairs_estimate` est incrémenté, puis les
seuils sont testés. Une seule ancre peut donc dépasser arbitrairement
`lim.sites` ou `lim.pairs`. Les casts `size_t -> u32` sont effectués pendant
la matérialisation, avant que le validateur de vue puisse refuser. Le produit
`anchor_seeds * lens_count` et l'addition au cumul ne sont pas vérifiés contre
un débordement `u64`.

L'ancien audit et [`GPU.md`](../docs/GPU.md) parlent par endroits de « borne
dure » ou de paires « corrigées ». La formulation exacte est :

> seuil post-ancre ; maximum observé inférieur au seuil plus la plus grosse
> ancre observée.

Ce n'est pas une borne produit, car la plus grosse ancre n'a elle-même aucun
plafond. Le double routage conserve en outre deux lots par ouvrier, hôte et
device, qui peuvent être résidents simultanément.

Fermeture recommandée :

1. précompter sites, seeds et paires avec multiplication/addition vérifiées,
   avant `fill_affine_sites` et avant tout cast ;
2. vider le lot courant si l'ancre suivante le ferait dépasser ;
3. tuiler une ancre isolée trop grande, en préservant l'ordre seed/lentille,
   ou rendre un `resource_exhausted` structuré avant allocation ;
4. borner aussi la somme résidente hôte/device par ouvrier et la grille CUDA ;
5. graver une fixture `ancre > cap` et des helpers de frontière sans allocation
   géante. La porte actuelle `lot <= seuil + max_ancre` ne suffit pas.

## P1 — rendre les résultats Q4 fail-closed et autoritatifs

[`validate_q4_results_view`](../src/gpu/q4_lane_batched.hpp) vérifie la taille
des verdicts, une somme de compteurs et l'ordre sommaire des émissions. Il
n'impose pas `st.emit == n_emits`, ne recalcule pas le nombre exact de
complétions admissibles et ne vérifie pas qu'un `y_site` appartient à la
lentille en étant distinct de `x`, `a` et `b`. La fixture actuellement dite
valide accepte précisément `y=x` et `y=skip_a`. Une sortie artificiellement
vide peut donc satisfaire le validateur avec des compteurs nuls.

Les vues synthétiques doivent aussi refuser un pointeur nul dès que le compte
associé est non nul, avant toute déréférence. Ajouter les mutants « émission
perdue », `y=x`, `y=skip`, `y` hors lentille et somme débordante.

À une frontière plus haute, les hooks publics `q3_override` et `q4_override`
peuvent ne rien produire tout en laissant le pipeline atteindre un statut
terminal. Il faut soit les réserver à un backend interne scellé, soit marquer
explicitement toute sortie issue d'un callback externe comme expérimentale et
non autoritative. Une fixture de callback vide doit empêcher
`complete_regular`.

## P1 preuve — empêcher un reçu adaptatif vacuement vert

Le protocole doit comparer **à la fois** `digest_balls` et `digest_all` entre
CPU et GPU. `digest_all` chaîne les forêts, pas le vecteur de boules ; une
divergence de candidats éliminée avant le fold pourrait aujourd'hui passer.
Ajouter au selftest un scénario où seul `digest_balls` diverge.

Pour les runs `contrat_gpuad_*`, le validateur n'exige que `gpu=1` et le
digest final. Il ne vérifie ni `min_sites=256`, ni `ancres_device > 0`, ni
`ancres_hote > 0`, ni lancement device, ni vidage hôte. Le faux pilote du
selftest ignore actuellement `--gpu-min-sites`, imprime `min_sites=1` et reste
vert : c'est une preuve constructive de vacuité.

Choisir une fixture qui exerce réellement les deux routes, parser leurs
compteurs exacts, puis ajouter trois scénarios négatifs : option ignorée,
tout-hôte et tout-device. Les petites portes de routage CPU et device doivent
précéder les contrats 50 000 dans la session distante.

Le plan et le code doivent aussi choisir la même politique. [`GPU.md`](../docs/GPU.md)
annonce un coût `seeds × sites`, tandis que le code route uniquement sur
`cover.size()`. Q4 connaît déjà `nseeds` et `lens_count` avant
matérialisation. Enfin, `--gpu-min-sites=-1` est converti en `SIZE_MAX` et
force silencieusement le tout-hôte ; le parseur doit refuser négatif, zéro et
overflow avec le code 2 attendu.

La campagne exige encore que les deux contrats denses tout-device réussissent
avant que son statut puisse être complet. Il faut décider explicitement si le
tout-device dense est un contrat obligatoire ou un diagnostic de ressource
séparé ; sinon une réussite adaptative ne pourra pas fermer le reçu global.

## P2 — durcissements utiles, non bloquants pour la correction CPU

- Passer les événements CUDA et chaque allocation temporaire sous RAII ; ne
  pas ignorer `cudaEventElapsedTime`, `cudaEventDestroy` ou `cudaFree`.
- Renommer le `kernel_ms` Q4 ou mesurer chaque kernel : la fenêtre actuelle
  inclut des synchronisations et travaux hôte entre les trois kernels.
- Partager les types de transfert, ou vérifier aussi standard-layout,
  trivialité, alignement et offsets ; une égalité de `sizeof` ne suffit pas.
- Ajouter un mutant Q4 réellement exécuté sur device et des planchers reçus
  pour les replis exacts cocirculaires ; le mutant device actuel porte sur le
  témoin Q3.
- Rafraîchir [`GPU.md`](../docs/GPU.md) : son tableau supérieur connaît le reçu
  Q4, mais des sections basses disent encore « en attente de G4 » et sa
  provenance promise est périmée.

## Verrous d'architecture au-delà de la lane GPU

- Le refus des positions dupliquées est une décision cohérente du profil
  actuel ; toute voie pondérée reste une phase distincte à définir et juger.
- La compression des plateaux sphériques n'est pas prouvée : le chemin permis
  reste census complet puis refus de capacité, sans troncature silencieuse.
- Les dix forêts horizontales sont livrées, mais les applications verticales
  entre ordres ne le sont pas. Ne pas appeler cet objet la « tour ».
- Le flux de payload n'a pas encore de publication globale transactionnelle :
  une sortie physique doit rester `provisional` jusqu'au statut terminal.
- La conformité v4 est une porte différentielle, pas une preuve d'exactitude.
  Garder les oracles bornés, le rejeu structurel et les autorités verticales
  causalement séparés.

## Ordre de fermeture conseillé à Claude

1. Durcir le validateur Q4 et graver les mutants de sortie.
2. Introduire le préflight vérifié et la politique ancre trop grande.
3. Aligner politique adaptative, CLI et documentation.
4. Durcir validateur et selftest de campagne sur les deux digests et la
   non-vacuité des deux routes.
5. Exécuter les petites portes device, puis seulement une nouvelle session G4
   gardée ; recevoir séparément stratégie adaptative et diagnostic tout-device.

## Reproduction de cet audit

Le pin a été exporté dans une archive propre, configuré et construit hors du
worktree partagé. Résultats exacts :

```text
cmake -S <archive-987a18c8> -B <build> -DCMAKE_BUILD_TYPE=Release : code 0
cmake --build <build> --parallel 4 : code 0
ctest --test-dir <build> --output-on-failure --parallel 4 -L gate : 156/156, 226.25 s
bash gcp-migration/selftest_campagne_v5.sh : violations=0, PROTOCOLE CONFORME
python tools/check_implementation_status.py : 20 phases validées
```

`nvcc` est absent localement : aucune compilation ou exécution CUDA nouvelle
n'est revendiquée par cet audit. Tout travail non commité du worktree partagé,
dont `src/pipeline/generate.hpp` et `.codex_fold_contract_probe.cpp`, est hors
du pin et hors verdict ; il n'a été ni modifié ni inclus par l'auditeur.

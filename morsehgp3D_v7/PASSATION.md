# État de livraison v7 — 4 septembre 2026

Cadre : `phase=exploration_v7_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.
Le chantier est sur `main`, sans branche supplémentaire. Ce document
sépare le code livré, les preuves exécutées et les contrats encore ouverts.

## Ce que contient la v7

La v6 a été lue et portée explicitement après lecture intégrale des deux
premières parties du manuscrit. Les
[octets consommés](docs/V6_SOURCE_SNAPSHOT.json) sont épinglés ; aucun
changement du worktree v6 n'est inclus dans le chantier v7.

La v7 ajoute une entrée u16 réelle avec identités conservées, une archive
transactionnelle à destination neuve, la complétion silencieuse candidate
avec descentes locales et plafonds explicites, et le fold horizontal
normalisé associé. Elle retire une copie globale de BallData au census,
utilise des permutations pour trier les gros objets, et sécurise l'admission
des threads avant leur travail. Elle ne construit ni Gamma exhaustif ni
la mosaïque de Delaunay d'ordre supérieur dans le chemin produit.

Le delta mono C ajoute un fold inline dans le mode sérialisé et le minimum
entier précalculé des paraboles du census. Le premier ne change pas les
options par défaut ; les callbacks du mode mono arrivent désormais sur le
thread appelant. Le second conserve les signes stricts du census et n'ajoute
que trois valeurs locales par requête, pas un cache global par point ou boule.

`verified_events_only` reste le payload par défaut. La route
`--complete-incidences` porte `normalized_horizontal_h0_candidate` et refuse
les dégénérescences pertinentes non prises en charge. `--require-exact`
refuse explicitement : il n'existe pas encore de produit globalement
qualifié exact derrière cette option.

## Autorités de qualification distinctes

| Snapshot ou campagne | Résultat exécuté | Portée et limite |
| --- | --- | --- |
| [Release A](receipts/release_20260904/summary.json) | 279/279 portes CPU | Sources et 31 binaires stables ; pas la qualification C |
| [Préparation B1](receipts/release_delta_20260904/summary.json) | Échec du contrôle de réutilisation avant CTest | Les variantes profil consommaient aussi l'archive ; échec conservé |
| [Delta B2](receipts/release_delta2_20260904/summary.json) | 21 portes fraîches vertes, 261 portes réutilisées | 26 binaires et dépendances inchangés ; pas 282 exécutions fraîches |
| [Construction C](receipts/mono_c_build_20260904/summary.json) | CLI Release construit, sources stables, B conservé | Construction seule, pas fermeture Release |
| [Campagne locale v6/v7 A](receipts/local_paired_20260904/summary.json) | 15 paires achevées identiques ; cinq censures dans trois autres paires | 36 tentatives à 8 threads, n=8k/16k/32k ; campagne globale `invalid`, aucun SLO |
| [Complétion locale](receipts/incidence_local_20260904/summary.json) | Six refus de domaine, zéro succès moteur | Observations achevées, pas capacité exacte à 8k/16k/32k |
| [Petite complétion](receipts/incidence_mini_20260904/summary.json) | Un succès candidat sur 200 points u16 étendus | Test borné, pas preuve globale ni débit à 50k |

Les rapports indépendants ont levé A1 (nettoyage d'archive sous panne
persistante d'allocation) et C1 (classification et enregistrement des
campagnes). Les sources et replays sont dans
[le dialogue courant](audits/DIALOGUE_COURANT.md). La preuve géométrique
[S1](audits/S1_COURANT.md) est composée conditionnellement ; les obligations
restantes portent sur les primitives, leur domaine et le graphe de calcul
effectivement compilé. Voir la
[cartographie de qualification](docs/QUALIFICATION_S1_PRIMITIVES.md).

## Contrats qui ne sont pas encore satisfaits

Le [contrat de performance](docs/CONTRAT_PERFORMANCE.md) impose d'abord le
mono-thread, puis le multi-CPU et le GPU. La cible 50k porte sur toute la
tour 1..10 sous une seconde, avec repli sur toute la tour 1..5, puis 100 ms
une fois la seconde qualifiée. La séparation WSPD s=8/10/12 est comparée
à ordre, entrée et objet inchangés. Une censure ou un refus n'est jamais
une réussite rapide. Une mesure Gabriel seule n'est pas une mesure HGP
complète.

Les paliers de plusieurs dizaines de millions sur G4 demandent encore
une architecture de résidence, des budgets RAM/VRAM et une reprise moteur
qui ne sont pas livrés. Les facettes et incidences encore matérialisées
restent un coût central, même sans mosaïque globale. L'archive atomique
n'est pas un checkpoint. La verticale, les poids du vote et le traitement
général des plateaux gardent également leurs contrats propres ouverts.

## Cloud et CI

Le [reçu GCP initial](receipts/gcp_20260904/created_then_missing_toolchain.json)
conserve un démarrage gardé G4 SPOT, l'échec avant compilation faute de
toolchain CPU et l'arrêt certifié de cette génération. Aucun benchmark
CPU/GPU n'a été produit par cette tentative. Les coupe-circuits GCE et
invité ont été vérifiés avant l'exécution. Le disque de la VM arrêtée
reste conservé ; une VM arrêtée n'implique pas un coût de stockage nul.

Le workflow v7 ne possède aucune autorité cloud et n'annonce pas de
résultat GPU. Les résultats CTest locaux, les futurs runs GitHub et les
reçus G4 sont trois autorités distinctes. Le registre officiel est inchangé.

Contrôles avant le premier commit : registre (20 phases), liste blanche
des workflows GCP et corpus documentaire historique (259 fichiers) passent.
Le corpus v7, absent de ce dernier contrôleur par défaut, est validé
explicitement via la même fonction. Le contrôle des blancs du diff initial
signale les fins de fichiers héritées et les blancs des reçus bruts ;
ces octets épinglés ne sont pas reformattés. Aucun fichier v6 ni reçu de
campagne encore ouverte n'est inclus dans cette publication de jalon.

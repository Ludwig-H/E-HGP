# Encodage FULL : natif contre première occurrence W1/W4

27 septembre 2026, base moteur `fd1a2c7ee`, audit CPU local. Cadre
`exploration_v9_hors_registre`, `cpu_reference`, `quantized_u18_input_only`,
`mode=parallel_parent_real_drafts`, `public_status=not_claimed`.
Aucun moteur modifié, aucun GPU/GCP utilisé.

## Résultat

La première occurrence **réellement multi-thread** produit exactement les
mêmes tableaux sur les vrais drafts capturés. À quatre workers, elle est
plus rapide que le natif dans les quatre observations agrégées ci-dessous.
C'est une amélioration locale encourageante, pas une qualification du
contrat de tour entière en 100 ms.

| entrée K1..5, s8 | sites | natif ms | prototype W1 ms | prototype W4 ms |
| --- | ---: | ---: | ---: | ---: |
| 08/000000 entière après masque sans sol | 39 885 | 163,973 | 168,825 | 120,793 |
| uniforme synthétique | 8 000 | 65,181 | 67,187 | 46,378 |
| uniforme synthétique | 16 000 | 147,045 | 152,448 | 109,568 |
| uniforme synthétique | 32 000 | 315,085 | 317,543 | 233,550 |

Chaque colonne est une **somme des médianes par ordre K**, trois essais
par variante et par ordre. Ce n'est ni un mur de tour, ni l'encodage
simultané de tous les ordres. Les observations sont appariées dans le même
processus, hôte partagé sans affinité CPU imposée ; W4 signifie quatre
workers, pas une réservation de quatre cœurs physiques. Les répétitions
restent très variables et aucune ne remplace sélectivement une autre.
Ne pas présenter le ratio de ces sommes comme un gain stable ou G4.

Sur LiDAR, détail des médianes natif/W1/W4 en ms :

| K | natif | W1 | W4 |
| --- | ---: | ---: | ---: |
| 1 | 3,216 | 3,162 | 2,388 |
| 2 | 10,078 | 10,213 | 6,205 |
| 3 | 33,141 | 31,778 | 24,629 |
| 4 | 49,382 | 51,778 | 36,911 |
| 5 | 68,155 | 71,894 | 50,659 |

Le très petit K1 uniforme8k régresse W4 (0,925 ms contre 0,701 natif) :
les coûts fixes des threads ne sont pas gratuits. Le prototype recrée
trois threads auxiliaires à chacune des six phases, soit 18 créations par
encodage W4 et 54 pour les trois répétitions de chaque ordre. Chaque
mesure complète publie donc 270 créations et quatre workers réellement
actifs dans la phase des occurrences de parents de chaque ordre.

## Ce qui est comparé et ce qui est payé

Port explicitement attribué de `b_full_first_real_drafts_20260927` publié
en `fd1a2c7ee`. L'accroche native, le lecteur d'entrée, la géométrie et
les copies restent inchangés. La nouvelle sonde remplace le réencodage
scalaire par les deux configurations du prototype
[parallel_parent](../b_full_parallel_parent_20260927/README.md), grain **256**.
Les trois passages sont natif/W1/W4, W4/natif/W1, W1/W4/natif : chaque
variante occupe chaque position une fois sur le même draft.

Les deux résultats expérimentaux sont chacun comparés champ à champ au
réencodage natif et à la forêt effectivement publiée par la chaîne : ordre,
banque, mots des niveaux, nœuds, parents, successeurs, contributions. Vingt
ordres et **120 paires** natif/prototype au total, plus comparaison à la
sortie effective. Les trois digests et toutes les tailles/capacités de
draft sont identiques aux captures scalaires antérieures. Pas de
compression, d'approximation ni de sous-échantillonnage supplémentaire.

Chaque chrono inclut création/jointure des threads, allocations, validation,
initialisation des vecteurs, copie des parents et dispersion. Il exclut
comparaison et destruction des résultats retournés ; les temporaires
internes sont détruits dans le constructeur et leur coût est inclus.
Les trois objets de réencodage coexistent jusqu'à la fin de la répétition,
en plus du draft copié et de la forêt publiée. Les capacités natives de
sortie ne sont donc pas le pic mémoire du processus.

Le hook ne remplace pas le constructeur de la chaîne : il appelle encore
le natif, puis copie le draft. Seuls les réencodages après retour utilisent
W1/W4. Les verticales restent celles du produit, **non reconstruites par
le prototype**. Une seule unité de traduction inclut la chaîne instrumentée,
sans lien libchain concurrent ; bibliothèque générateur et sources sont
épinglées. La banque privée du produit reste partagée, sans alias mutable
échappé ; l'ancien défaut de factory publique n'est pas silencieusement corrigé.

## Génération, mémoire et croissance

| entrée | chaîne instrumentée ms | copie des drafts, somme K ms | pic processus KiB |
| --- | ---: | ---: | ---: |
| LiDAR00 sans sol | 24 855,197 | 61,332 | 969 748 |
| uniforme8k | 5 257,500 | 26,747 | 464 784 |
| uniforme16k | 11 623,934 | 46,391 | 870 536 |
| uniforme32k | 24 984,372 | 104,111 | 1 689 372 |

Pour LiDAR : q2 1 290,272 ms, q34 21 118,437 ms, tour instrumentée
1 305,547 ms, mur externe de l'appel chaîne 25 552,497 ms. La fenêtre
d'encodage de cette chaîne est contaminée par les copies (71,833 ms) ;
ne jamais soustraire une somme de temps K à un mur parallèle. Les nouveaux
temps W4 ne doivent pas être soustraits de ce mur natif pour fabriquer
un chrono de moteur non exécuté.

LiDAR : 1 541 750 actions/nœuds, 1 541 745 parents, 897 776 contributions ;
capacités drafts 127 383 080 octets, forêts natives 195 162 040,
banque partagée 59 075 492. Le champ hérité
`prototype_workspace_requested_bytes` compte **seulement le scratch
principal** `batch[A]+first[A]`, 16A octets : somme K 24 668 000 pour
LiDAR, puis 10 070 464 / 20 828 704 / 42 564 992 pour uniforme.
Ajouter les erreurs privées de validation O(W), vecteurs d'exceptions,
d'activité et handles de chaque phase O(W), les piles des threads,
métadonnées d'allocateur et sorties. Ce champ n'est ni le workspace total
ni un pic RSS/VRAM ; les ordres sont réencodés successivement.

Initialisations des vecteurs et copie des parents restent scalaires avant
dispersion. Les grosses actions et les lots ne sont pas subdivisés en
incidences pour toutes les phases. Le travail logique est linéaire en
taille du draft, mais les retries CAS ne sont pas comptés et n'ont pas de
borne uniforme pour tout draft invalide. Sur les entrées admises, les
parents uniques ne se disputent pas la même cellule.

Les trois sorties uniformes conservent 629 404 / 1 301 794 / 2 660 312
actions, soit ×2,068 puis ×2,044 ; contributions ×2,066 puis ×2,045.
Ce diagnostic de taille ne devient pas une borne globale du générateur
ni une mesure de croissance LiDAR. Une seule trame sans sol est traitée
ici, entière après masque figé à 1 mm, 39 885 sites. Pas de nouvelle
segmentation ni de données KITTI copiées dans la v9 ; les sept partitions
sont vérifiées mais seule la trame retenue complète est chronométrée.

## Qualification et reproduction

Six commandes de qualification Release/Clang ASan/UBSan/LSan, réussies
au premier essai : quatre chaînes, dix ordres, soixante paires par binaire,
formes imbriquée et plate, pass-through avec trois digests identiques.
Ensuite quatre processus Release clos, sans répétition de la chaîne.
Tous les grands drafts ont zéro continuation ; le fallback général est
qualifié séparément par les gates structurels et TSan, pas ces mesures.
La sonde réelle elle-même n'a pas été exécutée sous TSan.

Les cinq lecteurs LIVE passent normal/−O ; `summarize.py` aussi. Ce dernier
est un lecteur post-capture, non une source compilée épinglée ; il fait les
sommes et vérifie l'identité des drafts historiques, sans remplacer les
lecteurs des reçus. Anciennes captures inchangées, sources/binaires/entrées
de la nouvelle capture hachés avant/après ; aucun transfert automatique
de qualification à la chaîne produit.

Autorité : [reçus r1](../../receipts/full_parallel_real_drafts_20260927/README.md).
Build clos : `/workspaces/E-HGP/build/v9-audit-full-parallel-real-drafts-20260927-r1`.
Ne pas reconstruire ces exécutables.

```bash
python3 -B morsehgp3D_v9/audits/b_full_parallel_real_drafts_20260927/run.py check qualification
python3 -B -O morsehgp3D_v9/audits/b_full_parallel_real_drafts_20260927/run.py check ng00
python3 -B morsehgp3D_v9/audits/b_full_parallel_real_drafts_20260927/summarize.py
```

Les prochaines mesures doivent distinguer le coût mémoire des sorties,
les créations répétées de threads et la génération des drafts. Aucun
contrat FULL/G4, s10/s12, K10, multi-séquence ni 100 ms n'est acquis ici.

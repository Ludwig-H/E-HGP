# Contre-audit courant : interfaces corrigées

29 septembre 2026. `public_status=not_claimed`. Lecture de la copie
`/workspaces/E-HGP/build/v10-fixes/entrees_cli/`, issue selon son reçu de
`0bce6cc00`. **Préintégration uniquement** : lors de la capture, le produit du
worktree courant n'a pas encore `src/cloud/u32le_input.hpp`. Aucun rebuild,
GCP, git, test massif, ni modification d'un fichier d'un autre acteur.

## Bilan utile

Les correctifs sont effectifs dans la copie : lecteur commun complet, refus
des répétitions nulles, export sans attaches protégé, bornes de feuille et de
nombre de boules. Le nouveau contrôle de la branche **entrée en flux** passe.

En revanche, le refus des options illisibles reste incomplet : **un suffixe
après un entier, ou un entier hors u32 réduit silencieusement modulo 2^32, sont
encore acceptés avec code 0**. C'est un défaut d'interprétation de la commande,
pas une inexactitude géométrique constatée sur une entrée valide.

## Périmètre de preuve

Le [reçu développeur figé](../../../receipts/audit_continu_20260929/interfaces_corrected/capture/sources/06_RECU_entrees_cli.md)
annonce 80 contrôles de frontières et porte encore `RESULTATS_A_COMPLETER`.
Il ne faut pas transformer cette annonce, ou des campagnes encore actives,
en clôture de toutes les portes. Ses 80 contrôles n'ont pas été rejoués ici.

Notre [reçu indépendant](../../../receipts/audit_continu_20260929/interfaces_corrected/capture/receipt.json)
est clos : **9 commandes, 0,092 s cumulées, zéro timeout**. Il contient commandes,
stdout/stderr complets, codes, hashes des entrées, dumps et snapshots des sources.
Le [runner](../../../receipts/audit_continu_20260929/interfaces_corrected/probe.py)
copie un binaire existant dans son répertoire temporaire, puis contrôle les
hashes avant/après ; il ne compile rien. Les 9 sources/documents relus sont
également stables avant/après et identiques aux snapshots archivés.

Binaire : `entrees_cli/build-final/mhgp10_catalogue`, Release C++20/GCC 13.3,
SHA256 `8ec42a51b2ecee2904c4cf594caa67b12a86f5666a28a8ade7d49b7f030c6e27`.
Cette tranche ne qualifie ni ASan/UBSan, ni les quatre CLI ensemble, ni leur
intégration. Les copies temporaires du binaire sont supprimées par le runner
après fermeture des sous-processus ; les artefacts de preuve restent archivés.

## Gate nouvelle 1 : options numériques consommées partiellement ou tronquées

Fixture : `(0,0,0), (8,0,0), (0,8,0), (0,0,8)`. Référence :
`--k=2 --threads=1 --leaf=8`. Chaque variante remplace une seule valeur.

| Variante | Résultat observé |
| --- | --- |
| `--k=2not_an_integer` | code 0, `status=ok`, K publié = 2 |
| `--leaf=4294967304` (= 2^32 + 8) | code 0, `status=ok` ; conversion source vers u32 = 8 |
| `--threads=4294967297` (= 2^32 + 1) | code 0, `status=ok`, threads publiés = 1 |
| `--k=not_an_integer` (témoin négatif) | code 2, `invalid_input / parameter_out_of_range` |

Les trois variantes admises produisent exactement le dump de la référence,
SHA256 `263379914af2129d3f60b8e11b0ac9604f78b36371a8b9698f49e1d57ffc5b56`.
L'égalité des dumps ne révèle pas à elle seule la valeur interne de `leaf` ;
cette interprétation est donnée par la conversion explicite dans la source.

Cause : [CLI catalogue figée](../../../receipts/audit_continu_20260929/interfaces_corrected/capture/sources/01_mhgp10_catalogue.cpp),
lignes 41–45 : `stoi`/`stoul` sans contrôle de la position finale ; puis
conversion de `unsigned long` vers `u32`/`unsigned` sans borne intermédiaire.
Le `catch` des lignes 51–52 règle les exceptions, pas ces admissions silencieuses.
La lecture de la CLI tour révèle le même motif, mais aucune généralisation
expérimentale à toutes les interfaces n'est revendiquée ici.

Correctif ciblé conseillé : parseur partagé vérifiant la consommation du
jeton entier, son signe autorisé et la borne du type cible **avant** conversion.
Conserver les trois fixtures ci-dessus, le témoin entièrement non numérique
et des valeurs valides de bord. Aucun motif de modifier le moteur géométrique.

## Gate nouvelle 2 : entrée non régulière, sans taille annoncée

Le runner transmet un vrai pipe par `/proc/self/fd/...`, propriété vérifiée
par `fstat` ; le petit contenu est entièrement écrit puis le côté écrivain
fermé avant le lancement. Aucun producteur extérieur ni processus tiers.

| Contenu du pipe | Résultat observé |
| --- | --- |
| Les quatre points complets (48 octets) | code 0 ; dump identique au fichier régulier |
| Même entrée + 1 octet | code 2, `size_mismatch`, aucun dump |
| Même entrée + 11 octets | code 2, `size_mismatch`, aucun dump |
| Même entrée + point complet `(262144,1,2)` | code 2, `coordinate_out_of_domain`, aucun dump |

Ces quatre contrôles exercent la branche sans taille de fichier connue du
[lecteur commun](../../../receipts/audit_continu_20260929/interfaces_corrected/capture/sources/00_u32le_input.hpp),
lignes 40–87, absente de la liste des 80 contrôles de fichiers réguliers du
reçu développeur. Ils confirment que le suffixe incomplet n'est plus perdu et
que le domaine u18 est vérifié sur le cinquième point complet. Pas de test de
flux géant, de faute mémoire ou de modification concurrente d'un fichier.

## Autres correctifs : lecture de source, pas nouvelle qualification globale

- **Dump sans points.** La [CLI tour figée](../../../receipts/audit_continu_20260929/interfaces_corrected/capture/sources/02_mhgp10_tower.cpp),
  ligne 194, saute les attaches quand `point_node` est vide ; nœuds, niveaux,
  parents et liens verticaux restent exportés. Le développeur a déjà une gate
  différentielle complète/sans points : aucun doublon lancé ici.
- **Répétitions.** Ligne 72, `repeat < 1` est refusé avant lecture ; c'est bien
  la cause du crash `repeat=0` qui est traitée, pas un simple contournement du dump.
- **Feuilles.** [Générateur figé](../../../receipts/audit_continu_20260929/interfaces_corrected/capture/sources/03_generator.cpp),
  lignes 636–649 : K est validé, puis `leaf_size != 0 && leaf_size < K+3` est
  refusé avant calcul. La valeur 0 garde le défaut. Cette borne pratique est
  plus forte que M ≥ K ; elle ne constitue pas une preuve de complexité générale
  sur les dégénérescences. Les nouvelles conversions CLI doivent éviter de la
  contourner ou de changer silencieusement la valeur demandée.
- **Indices u32.** `check_ball_count` (ligne 634) refuse `balls >= kNone` ;
  appel ligne 778 avant allocation/assemblage, boucle locale u32 et conversion
  de `refs.size()` ligne 818. Le total borne les comptes par fil. Les tests
  artificiels documentés sont pertinents ; aucun test matériel à 2^32 boules
  n'est nécessaire ni exécuté ici. Le lecteur borne de même le nombre de points
  d'un fichier régulier avant lecture ; un pipe est vérifié après EOF.

## Relecture et rejeu

SHA256 du reçu indépendant :
`1493b00927e7f54b57d6baf2e04acfe0e136ae137bf2117948bf4f9a68522c3f`.
Pour rejouer, fournir au runner un **nouveau** `--out` temporaire : il refuse
d'écraser une capture et refuse un binaire différent de celui épinglé. Les
chemins historiques du reçu sont conservés ; le binaire d'origine appartient
au développeur et n'a pas été modifié. Les preuves ici sont celles de cette
capture close, pas celles de compilations ou régressions encore en cours.

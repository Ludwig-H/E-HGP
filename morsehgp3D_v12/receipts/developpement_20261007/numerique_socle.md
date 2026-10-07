# Contrat numérique en repère local dans le socle (7 octobre 2026)

Livraison de l'agent du développeur, intégrée par `git apply` (82 fichiers : 64 modifiés, 18 ajoutés). Détail :
[`numerique_socle_RAPPORT.md`](numerique_socle_RAPPORT.md) (résultats, mutants, points de contrat) et
[`numerique_socle_CHANGEMENTS.md`](numerique_socle_CHANGEMENTS.md) (fichier par fichier). Spécification :
[`CONTRAT_NUMERIQUE.md`](../../docs/CONTRAT_NUMERIQUE.md).

## Ce qui est fait

Repère local certifié (coin minimal, étendue exacte, fermetures jusqu'à $2^{32}$, $s=33$) ; budgets par palier
d'étendue (`SpanBudgets<S>` : étroit $s\leq 16$, moyen $s\leq 24$, large $s\leq 33$) ; voies native, certifiée,
contrôlée et large, avec compteurs, sans rétrécissement avant contrôle, comparaisons de niveaux jusqu'à 512 bits ;
certificats liés à leur domaine $t$ ($t=s+2$ pour le recensement gardé) ; recensement gardé (`CertifiedBall`,
`GuardedSphere`) sans centre absolu ; requêtes à centre entier en `u64` natif, contrôlé puis `u128` ; test du milieu
local ; réservoir $2s+4$ ; comparaison des centres en deux temps ; clé de Morton exacte sans troncature et identité des
sites ; profil 32 admis (18 et 33 refusés, à la configuration et à la compilation) ; invariance par translation jugée à
translation près ; portes de palier à $s^{*}$ et $s^{*}+1$.

**Non retenu** : la clé de Morton sur coordonnées normalisées. Elle rendait neuf portes rouges au profil 21, parce que
l'ordre de Morton n'est pas invariant par translation et que les juges du socle lisent l'ordre canonique des sites sur
la clé absolue. Décision du développeur : garder la clé exacte sur coordonnées absolues, qui garde l'ordre de la v11 et
le différentiel des feuilles (contrat numérique § 4, contrat du catalogue § 6).

## Résultats

| Contrôle | Résultat |
| --- | --- |
| constructions Release u21, u24, u32 | 0 avertissement (`-Wall -Wextra -Wpedantic -Werror`) |
| portes rapides (agent, `ctest -LE long`), à chaque profil | 438 enregistrées, 415 sélectionnées, 414 passées, 1 sautée (sentinelle LiDAR, sans données), 0 échouée ; aucun attendu changé aux profils 21 et 24 |
| ASan et UBSan, unités `num`, `index`, `cloud`, profils 21 et 32 | 172 portes sur 172 |
| mutants | `num` 76 sur 76, `index` 18 sur 18, `cloud` 17 sur 17, `core` 7 sur 7 (ceux qui visent un fichier modifié) |
| style | `style_ok fichiers=189` |
| intégration dans le dépôt (après `07ee13ef6`), rejeu local `ctest -LE long` avec la v10 figée | profil 32 : 441 sur 441 ; profil 21 : 440 sur 441, la porte `mhgp12_reference_diff_v10` ayant expiré (300 s) sous charge, puis échoué au premier rejeu, puis passé 13 fois de suite (`CST-0024`, ouvert) ; `style_ok fichiers=213` |

## Preuves par constat (états laissés à l'auditeur)

| Constat | Portes et mutants |
| --- | --- |
| `CST-0108` (garde réservée aux boules certifiées) | `mhgp12_index_guarded_uncertified` ; mutant « recensement avant certificat » tué ; mutant « garde d'un bit trop étroite » livré comme certificat au domaine $s+1$, tué par un support certifié au domaine 21 et non 22 |
| `CST-0109` (recensement sans centre absolu) | `mhgp12_index_guarded_{fixtures,witnesses,fraction}` (boîte partielle, segment 0–1, boîtes disjointes et partielles, sites hors pavé) |
| `CST-0110` (requêtes à centre entier) | portes `mhgp12_num_local_*` des sommes de carrés, coins opposés de $[0,2^{32})^{3}$ |
| `CST-0111`, `CST-0114` (budgets mixtes, milieu local) | portes de palier `mhgp12_num_local_*` à $s^{*}$ et $s^{*}+1$ ; les budgets d'orientation $7s+14$ et $3s+10$ attendent leurs appelants (catalogue) |
| `CST-0201` (certificats et domaine) | témoin q3 de Codex ; mutant « certificat du support seul » tué |
| `CST-0202` (identité des sites) | `mhgp12_cloud_unit_identity` (clé exacte, doublons intercalés) |
| `CST-0204`, `CST-0208` (fermeture à 33 bits, réservoir) | portes de palier et de repère ; `mhgp12_core_coord_bits_33_refusal` |

GCP non utilisé pour ce travail. `public_status=not_claimed`.

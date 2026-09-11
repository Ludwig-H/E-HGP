# Banc privé du coeur MEB : profil, variantes exactes, vérification

11 septembre 2026, second auditeur, sur `99b4d3b1`. `public_status=not_claimed`.
GCP non utilisé. Aucune source active modifiée : le banc compile contre un arbre
isolé obtenu par `git archive 99b4d3b1`, jamais contre le worktree partagé.

Conclusions et chiffres dans la [note](../NOTE_CLAUDE_COEUR_MEB_20260911.md).
Ce dossier ne porte que de quoi **refaire la mesure**.

## Reproduction

```bash
PIN=$(mktemp -d)
git archive 99b4d3b1 morsehgp3D_v7 | tar -x -C "$PIN"
g++ -std=c++20 -O2 -w -I "$PIN/morsehgp3D_v7" meb_hybrid.cpp -o meb_hybrid
./meb_hybrid 20000 40000
```

Le programme construit un nuage `uniform` u16 de 20 000 points, tire 40 000
ensembles de sites en plus proches voisins avec K de 2 à 10, puis compare pour
chaque cas la sortie de `anchor_meb` à celle de la variante hybride, champ par
champ : `status`, `key`, `level`, `support_size`, `support_slots`,
`selected_shell_count`. Il rend 1 si une seule divergence apparaît.

## Ce que le banc contient

- `brute_q2` : force brute avec q=2 restreint aux paires de distance maximale et
  confinement testant d'abord les deux points extrêmes.
- `welzl_rec` : Welzl récursif dont la **base est bornée à quatre points**, ce
  qu'autorise le fait qu'une MEB en dimension trois est déterminée par au plus
  quatre points ; `trivial_meb` coûte alors au plus onze candidats.
- `run` : MEB d'abord, puis canonicalisation sur la **coquille** seule.
- `hybrid` : `brute_q2` pour K<7, `run` pour K≥7.

## Limites déclarées

Les ensembles de sites sont des plus proches voisins, substitut raisonnable des
facettes réelles, **pas** le flux exact du résolveur. Le banc mesure le noyau MEB
isolé, pas la tour : la part de ce noyau dans `tower_s` n'est pas instrumentée.
Aucun contrat 50k, 1 s ou 100 ms n'en découle. Le prototype n'a ni mutants, ni
gardes de dépassement, ni portes O2/SAN : ce n'est pas du code produit.

## Welzl réparé, après la réfutation K7

`welzl2.cpp` porte la réparation : le cas de base construit la boule passant
**par** `R` avec `q3_form` et `q4_form`, en ne gardant que `g > 0` et `det > 0`
et en retirant les filtres de minimalité de `form()`. Même compilation, puis :

```bash
./welzl2
```

Il exécute trois épreuves : la contre-fixture K7 de
`receipts_meb_boundary_20260911`, le balayage exhaustif de sa famille de dix
points sur tous les masques de taille 2 à 10, et 200 000 tirages. Il rend 1 si
une seule divergence apparaît, et compte les déclenchements du repli de sécurité.
Mesuré : 0 divergence et 0 repli sur les trois épreuves, 4,12x en temps.

## Exactitude à deux échelles

| n | `payload_digest` | `tower_s` base | `tower_s` patché | gain |
| ---: | --- | ---: | ---: | ---: |
| 8 000 | identique | 61,23 s | 45,13 s | 1,357x |
| 16 000 | identique | 139,15 s | 107,03 s | 1,300x |

Sorties brutes : `flux_reel.out`, `flux_reel_16k.out` et
`flux_reel_comptabilite_corrigee.out`. Le lecteur `verify.py` exige, pour chaque
échelle, deux digests identiques et un `tower_s` strictement décroissant. Le gain
recule avec la taille parce que la part géométrique recule.

**Faire foi pour le travail : `flux_reel_comptabilite_corrigee.out`.** Les deux
premières captures sous-comptaient le travail de la variante, faute de facturer
les formations de `boundary_ball` et les puissances de récursion et de coquille.
Leur `tower_s` et leurs digests restent valides ; seul leur rapport de supports
était faux. Après correction, à n=8000 et sous la borne à dix facettes :

| grandeur | référence | variante | rapport |
| --- | ---: | ---: | ---: |
| supports testés | 340 615 272 | 104 791 833 | **3,25x** |
| `tower_s` | 62,17 s | 45,63 s | 1,362x |

Le digest reste identique, donc la borne ramenée de douze à dix facettes
n'altère pas l'objet.

## Piège à éviter : l'export isolé doit être HORS du dépôt

`realflow_patch.py` refuse tout arbre versionné, en remontant les ancêtres à la
recherche d'un `.git`. Un export placé sous `build/` **à l'intérieur** du dépôt
est donc refusé, et la sonde prétendument patchée reste identique à la
référence : la comparaison se ferait entre la référence et elle-même, sans rien
prouver. Extraire hors du dépôt, et **vérifier que le patch a eu lieu** avant de
mesurer :

```bash
PIN=$(mktemp -d)
git archive <commit> morsehgp3D_v7 | tar -x -C "$PIN"
python3 -B realflow_patch.py "$PIN/morsehgp3D_v7/src/forest/anchor_meb.hpp"
grep -c anchor_meb_brute "$PIN/morsehgp3D_v7/src/forest/anchor_meb.hpp"
```

Le dernier contrôle doit rendre un compte strictement positif. Je suis moi-même
tombé dans ce piège : une mesure entière a été rendue nulle parce que la garde
avait refusé de patcher et que je ne l'avais pas vérifié avant de chronométrer.

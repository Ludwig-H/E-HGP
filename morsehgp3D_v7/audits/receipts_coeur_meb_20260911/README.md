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

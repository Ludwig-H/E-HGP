# Note de Claude — sweep du join postings et prévision 50 k (`forecast_only`)

Date : 10 août 2026 UTC. Auteur : Claude (développement). Cadre :
`phase=exploration_v3_hors_registre`, CPU de vérité, profil u16, famille
TRONQUÉE `smax=11` (`partial_refinement`) — **aucun chiffre de cette note n'est
un GO mémoire ni une exactitude** ; la règle des deux autorités de
[`REPONSE_CLAUDE_JOIN_POSTINGS_Q1_Q3_20260810.md`](REPONSE_CLAUDE_JOIN_POSTINGS_Q1_Q3_20260810.md)
s'applique : seul le `P_post` exact du catalogue réellement joint est
autoritatif, et le préflight du fold le calcule déjà avant toute émission.

## 1. La table mesurée (codespace 2 vCPU, un cœur, Release, `--join postings`)

Protocole : densité 1e-3, `smax=11`, `K=5`, graine 20260810, un nuage par
taille — un profil descriptif, pas une campagne répétée.

| n | G | pool | `P_post` | plus gros lot | unions | fold s | catalogue s | Mocc/s fold |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 100 | 14 954 | 121 269 | 114 337 554 | 119 104 | 71 928 | 10,0 | 4,4 | 11,4 |
| 141 | 25 042 | 205 635 | 221 895 856 | 152 537 | 120 964 | 22,1 | 8,6 | 10,0 |
| 200 | 40 007 | 329 920 | 385 553 414 | 262 072 | 193 471 | 42,7 | 17,1 | 9,0 |
| 283 | 63 692 | 529 810 | 684 186 889 | 358 031 | 308 985 | 77,6 | 30,9 | 8,8 |
| 400 | 99 942 | 833 925 | 1 168 158 097 | 568 045 | 485 364 | 275,4 | 52,5 | 4,2 |

Identités respectées et `P_post prédit == réel` à chaque ligne. Le pic
conservateur prédit croît de 44 à 297 Mo.

## 2. Lectures descriptives, sur cette fenêtre seulement

- `P_post` s'ajuste à `~ n^1.68` sur ces cinq points (résidus non publiés :
  cinq points ne choisissent pas un modèle) ;
- le débit du fold TOMBE de 11,4 à 4,2 M occurrences/s entre n=100 et n=400 —
  l'empreinte (postings + couvertures + états ×K) sort des caches ; le débit
  n'est PAS une constante, l'extrapoler est interdit ;
- `|P_x|` max continue de croître (3 915 → 5 260) ; le plus gros lot aussi.

## 3. Prévision 50 k, étiquetée `forecast_only`

En prolongeant `n^1.68` (modèle non prouvé, résidus inconnus au-delà de
n=400) : `P_post(50k) ~ 4e12` occurrences sous famille tronquée `smax=11`.
À un débit — non constant — de l'ordre de 5-10 M occ/s par cœur, le join
monolithique vaudrait des JOURS sur un cœur, des heures sur 48 cœurs, et
l'ordre de l'heure sur un device à ~1 Gocc/s. Conclusion de planification,
au sens de la règle des deux autorités : **le join 50 k par postings pleins
est un NO-GO prévisionnel dans TOUTES ses formes monolithiques** — le
chiffre autoritatif restera le `P_post` exact du catalogue 50 k réel, que le
préflight imprime avant émission et refuse au-delà du budget.

## 4. Les deux leviers structurels qui changent la masse, pas la constante

La note `q_min` de l'auditeur les contient déjà ; ils deviennent maintenant
le travail d'échelle prioritaire, AVANT tout kernel :

1. **Postings par ordre sur `Σ_k`** (théorème 2, réduction exacte sous source
   `Σ_k`-complète) : les générateurs `q > k+1` sont RETIRÉS du `DSU_k` et de
   ses postings sans perdre d'attache. À `k=1`, seuls les générateurs
   `q <= 2` participent ; à `k=2`, `q <= 3`. La masse par ordre devient
   `Σ_x C(d_x^{(k)}, 2)` avec `d_x^{(k)}` le degré RESTREINT — des ordres de
   grandeur sous `P_post` plein, à mesurer par la distribution réelle des
   `q` (en 3D, `q <= 4`).
2. **Réduction en arbre à `k=1`** (audit live 39cf76e) : la connexité d'une
   posting n'exige que `d_x - 1` arêtes, pas `C(d_x, 2)` — le poste dominant
   du bas de la tour s'effondre linéairement.

S'y ajoutent les runs bornés (intervalles triangulaires scellés, merge
vérifié) pour que le pic mémoire cesse d'être `O(P_post)`.

## 5. Ce que cette note ne fait pas

Pas de catalogue 50 k réel (le générateur tronqué `smax=11` y coûterait des
heures de codespace et son statut resterait `partial_refinement`) ; pas de
GO/NO-GO mémoire (préflight seul juge, sur le catalogue réel) ; pas de G4 —
l'audit live le dit : aucun kernel GPU n'existe à qualifier, une session
G4 n'apporterait rien avant les runs bornés et la source certifiée.

GCP non utilisé pour cette note.

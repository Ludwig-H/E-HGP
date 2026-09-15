# Oracle q3/q4 indépendant en entiers i128

15 septembre 2026, auditeur indépendant B. `phase=exploration_v8_hors_registre`,
`backend=cpu_reference`, `profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.
GCP non utilisé. Vérité terrain bornée, hors chemin produit, sans aucun
en-tête produit.

`oracle_q3q4.cpp` énumère, sur un nuage u16 lu dans un fichier texte,
toutes les présentations positives de cardinal 3 (triangles strictement
aigus) et 4 (tétraèdres non plats dont le circumcentre a des poids
barycentriques strictement positifs), compte exactement les sites
strictement intérieurs à la circumboule et sa coquille complète, retient
celles qui vérifient p + q ≤ Kmax + 1 (soit p < h_q = Kmax + 2 − q) et
signale une coquille excédant le support sans la filtrer. Prédicats
entiers : acuité par produits scalaires ; q3 par le signe de
2D·|z − a|² − 2 (z − a)·W avec D = |u × v|² et
W = |v|²(u·u − u·v)·u + |u|²(v·v − u·v)·v (au plus 2^106) ; q4 par le
signe de 2det·|z − a|² − 2 (z − a)·g avec g = 2det·(c0 − a) obtenu par
Cramer (au plus 2^89), et le bon centrage par les signes des quatre
déterminants de Cramer des poids barycentriques. Coût O(n⁴) pour q3 et
O(n⁵) pour q4 : réservé aux petits nuages (n ≤ 250 pour q3, n ≤ 60 pour
q4 en pratique).

`compare_reference.py` le confronte au catalogue rationnel exhaustif de
`reference/morsehgp3d_oracle` (`build_critical_catalog`, `fractions`) :
mêmes présentations positives retenues, et toute coquille excédentaire
d'une présentation positive figure parmi les dégénérescences de la
référence (qui n'en garde qu'une par boule). Reçu
`ORACLE_Q3Q4_CHECKS.json` : cinq fixtures (les deux contre-fixtures du
constructeur, tétraèdre régulier entier, triangle droit, cube cosphérique
avec centre) × Kmax ∈ {2, 5, 10} et 300 nuages aléatoires (n ≤ 12, boîtes
serrées) : 5 811 présentations q3 et 1 358 q4, 126 coquilles
excédentaires, 0 désaccord.

```bash
g++ -std=c++20 -O2 -Wall -Wextra morsehgp3D_v8/audits/oracle_q3q4_20260915/oracle_q3q4.cpp -o /tmp/oracle_q3q4
PYTHONDONTWRITEBYTECODE=1 python3 -B morsehgp3D_v8/audits/oracle_q3q4_20260915/compare_reference.py --binary /tmp/oracle_q3q4 --clouds 300
```

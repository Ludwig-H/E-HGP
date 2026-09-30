# Contact de la majorité uniforme : rejeu sous MM_κ selon l'univers de votes

30 septembre 2026, vers 17 h 45 UTC. Réponse du développeur au
[contre-exemple de l'auditeur continu](../../audit_continu_20260929/uniform_majority_contact_20260930/README.md).

```text
phase=exploration_v10_hors_registre
backend=cpu_reference
profile=quantized_u18_input_only
mode=interaction_auditeur
public_status=not_claimed
GCP non utilisé. Aucun moteur modifié ; aucun appel natif au-delà de l'export de la bibliothèque privée.
```

## Fixture

Sites de l'auditeur, K = 2, homothétie S = 1024 : a = (0, 0, 0), b = (10S, 0, 0), z = (9S − e, 3S, 0),
y = (0, 0, 9S), avec e ∈ {0, 1}, soit ε = e/S. Mesure : hauteur de réunion u(a, b), en rayon, divisée par S.

## Résultat (normal et `-O` identiques octet pour octet)

| Règle | e = 0 | e = 1 | Saut |
| --- | ---: | ---: | ---: |
| majorité de bande uniforme de la bibliothèque, η = 1/8 et 1/4 (témoins forts du catalogue) | 5 | 6,53801 | 1,53801 |
| MM_κ, univers du catalogue, η′ = 1, κ = 2 et 3 | 5 | 6,53801 | 1,53801 |
| MM_κ, univers des K-parties (Γ_2), η′ = 1, κ = 2 et 3 | 5 | 5 | 0 |
| MM_κ, univers des K-parties, η′ = 1/4, κ = 2 et 3 | 6,53835 | 6,53801 | −0,00034 |
| P_2 et première couverture A1 | 6,53835 | 6,53801 | −0,00034 |
| core | 9 | 9 | 0 |

Lecture :

- Le saut de l'auditeur se reproduit sur la grille : 5 → √171/2 ≈ 6,538, pour un déplacement d'une unité à
  S = 1024.
- Il vient de l'univers de votes, pas de la date. MM_κ, dont la date est continue par la marge, saute aussi
  quand il vote sur les témoins forts du catalogue.
- Sur les K-parties, la partie {a, b} reste un vote de rayon 5, 1-lipschitzien : pas de saut.
- Avec η′ = 1/4, le vote AB est presque éteint (poids ≈ 0,06). a suit alors la lignée de son plus proche voisin
  y, puis rejoint b à la fusion AYZ.

## Pièces

`contact_mmg.py` (script), `contact_normal.json` et `contact_O.json` (sorties), stderr vides, `SHA256SUMS`.
`SOURCES_PRIVEES.sha256` : empreintes des sources privées importées, lues seulement :
`build/v10-verrou-points/revision_cible/majorites_continues/mmc.py` et
`build/v10-verrou-points/fixtures_cibles/lib/regles.py`, `run_target.py`.
Commande : `python3 -B contact_mmg.py > contact_normal.json`, puis la même avec `-O`.

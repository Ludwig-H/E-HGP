# Reçu local : brouillon plat de la tour (chemin statique)

23 septembre 2026. **GCP non utilisé.** Cadre : `exploration_v9_hors_registre`,
`backend=reference_cpu`, `profile=quantized_u18_input_only`,
`public_status=not_claimed`.

## Changement mesuré

Code `f93dc165`, comparé à la phase A allégée `aa29245f`
([reçu précédent](../tower_phaseA_lean_local_20260923/README.md)).
- **Forme plate** : le chemin statique range ses lots dans des tableaux
  plats (niveaux par lot, plages CSR de parents et de contributions par
  action), remplis sans allocation par action.
- **Lecteurs** : populations, images verticales et encodage lisent cette
  forme. Le constructeur du certificat a un seul corps pour les deux formes.
- **Témoin** : le chemin séquentiel garde la forme vectorielle ; même charge
  utile et mêmes condensés (152/152 portes).

## Mesures (trame sans sol 08/000000, s = 8, W8, tour statique 8)

Hôte partagé et très chargé (charge 10 à 11 sur 8 cœurs) ; paires
entrelacées allégée / plate. Binaires : allégé `74732c6a…`, plat
`77b2d1c8…`.

| K | bras | phase A de l'ordre K (ms) | populations | images | encodage | tour (ms) | RSS (Mio) |
| ---: | --- | --- | --- | --- | --- | --- | --- |
| 5 | allégé | 502 / 514 / 550 | 93 / 93 / 89 | 155 / 146 / 150 | 133 / 94 / 112 | 1 737 / 1 698 / 1 701 | ≈ 1 020 |
| 5 | plat | **438 / 480 / 455** | 76 / 78 / 72 | 163 / 147 / 135 | 78 / 91 / 84 | **1 670 / 1 668 / 1 628** | ≈ 975 |
| 10 | allégé | 2 706 / (5 439) | 678 / (1 552) | 643 / (1 390) | 368 / (590) | 14 705 / (27 892) | ≈ 4 185 |
| 10 | plat | **1 464 / 1 840** | 492 / 522 | 580 / 595 | 277 / 290 | **11 327 / 12 507** | ≈ 3 700 |

La seconde exécution allégée à K10 (entre parenthèses) est aberrante :
tout y double sous la contention de l'hôte. La lecture repose sur la
première.

- **K5** : phase A de l'ordre K5 −10 à −13 %, populations et encodage
  environ −15 à −25 %, tour environ −4 %, RSS −5 %.
- **K10** : phase A de l'ordre K10 environ −35 %, populations −25 %,
  encodage −22 %, tour −15 à −23 %, RSS −11 %.
- **Depuis la base** `3dfedcae` (phase A et brouillon ensemble), la phase A
  de l'ordre K5 passe d'environ 790 à 455 ms en local.
- **Portée** : mesures locales bruitées ; le gain G4 reste à mesurer (R14).

## Contenu

`out/` (sorties JSON de la sonde), `SHA256SUMS`.

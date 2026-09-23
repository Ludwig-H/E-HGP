# Clipping global des centres après S2 : potentiel très limité sur le brut K5

Audit B, 23 septembre 2026, base `f3dc025cd`, statut **mesure exploratoire
CPU, non autonome sans les traces temporaires**. Aucun moteur HGP n'a été
relancé et aucun GPU/G4 n'a été utilisé. Réponse attendue : ne pas porter
la cellule unique clippée comme optimisation prioritaire ; mesurer ensuite
les **sous-cellules de centres avec gardes distincts**.

Sur la trame SemanticKITTI brute entière `08/000000`, grille isotrope
1 mm, `123 389` sites, `K=5`, `s=8`, le disque nominal d'au moins une voie
q3/q4 encore ouverte après S2 dépasse la boîte réelle du nuage pour
seulement **57 677 / 3 986 433 arêtes (1,447 %)**. Ces arêtes pèsent
**8 363 262 / 559 661 741 incidences site–cœur (1,494 %)**.
Ces incidences comprennent les deux extrémités de chaque arête. En
les retirant des deux sommes, le majorant porte sur
**8 247 908 / 551 688 875 formes chargées hors extrémités (1,495 %)**.

| Voie active après S2 | Arêtes actives | Disque hors boîte | Incidences cœur des arêtes actives | Incidences cœur hors boîte |
| --- | ---: | ---: | ---: | ---: |
| q3 | 3 502 180 | 27 868 (0,796 %) | 277 478 723 | 2 636 115 (0,950 %) |
| q4 | 3 665 711 | 54 431 (1,485 %) | 546 221 992 | 8 042 106 (1,472 %) |
| au moins une voie | 3 986 433 | **57 677 (1,447 %)** | 559 661 741 | **8 363 262 (1,494 %)** |

Les lignes q3 et q4 **se chevauchent** et ne s'additionnent pas :
24 622 arêtes, représentant 2 314 959 incidences cœur, ont les deux disques
hors boîte. Aucun disque actif n'est exactement tangent selon le test
entier. La boîte des sites de cette entrée est
`[0,158607]×[0,158284]×[0,30595]` en millimètres encodés.

## Sens exact de ce plafond

Le [lemme de redondance](../CERTIFICAT_B_REDONDANCE_CELLULE_UNIQUE_20260923.md)
établit qu'une cellule unique contenant **tout** le disque nominal de
centres d'une arête ouverte ne peut plus la fermer avec des gardes que
S2 n'aurait pas déjà crédités. Clipper cette cellule à la seule boîte
globale réelle ne crée donc une possibilité nouvelle que si le disque
d'une voie active **déborde strictement** de cette boîte. Pour toute
arête dont les disques actifs restent intérieurs, une preuve à cellule
unique ne peut éviter son cœur, quel que soit le choix de gardes.
Ainsi, sur **ce seul régime**, 1,494 % est un majorant de masse cœur
concernable, **pas** un nombre de rejets, de formes réellement évitées,
ni un gain de temps. Le poids du cover par arête n'est pas dans cette
trace et ne peut pas être extrapolé ; une voie fermée seule peut encore
laisser l'autre voie consommer le cœur ou le cover. Les segments de
rectangle entiers ne peuvent pas améliorer ce plafond par simple
regroupement d'arêtes avec la même cellule.

Ce résultat ne condamne **pas** les sous-cellules : chacune couvre
seulement une partie du disque et peut employer un jeu différent de
gardes. Il ne qualifie ni K10, ni LiDAR sans sol, ni `s=10/12`, ni une
autre séquence, ni la croissance globale ou le contrat G4. Avant
d'implémenter un certificat pré-cœur, faire le même test O(S) sur ces
régimes ; n'engager ensuite qu'un shadow de 2/4/8 cellules à coût de
recherche borné, avec repli exact et les formes **physiquement**
épargnées, pas seulement les arêtes filtrées.

## Provenance et reproduction

[`measure.py`](measure.py) lit les huit morceaux de la
[trace de préfixe paresseux](../lazy_prefix_dead_core_20260923/README.md),
sans relancer HGP. Le producteur est le commit
`c265a5dae4dd92059fc78acc0a1d7f52de9c1435` instrumenté et son
`RUN.json` fixe la commande du probe, les SHA-256 des huit parties,
de l'entrée et des IDs ; les mêmes sommes sont scellées dans le
[`RESULTS.json` versionné](../lazy_prefix_dead_core_20260923/RESULTS.json).
Chaque record `<IIIIIIII` donne les IDs bruts des extrémités, la taille
du cœur et le masque avant/après sa preuve. Les fichiers d'entrée
`<III` et d'IDs `<I` sont joints par rang, avec IDs uniques.

Le script vérifie tous les SHA, `3 986 433` records, la somme
`Σcore_sites=559 661 741` et
`Σ(core_sites−2)=551 688 875=dead_core_form_sites`. Pour chaque
arête active `ab`, il pose `D=|b−a|²`,
`qᵢ=min(aᵢ+bᵢ−2loᵢ,2hiᵢ−aᵢ−bᵢ)` et classe un disque q3 comme débordant
si un axe vérifie `3qᵢ²<D−(bᵢ−aᵢ)²`, ou q4 avec `2qᵢ²` à gauche.
Cette comparaison entière est équivalente à la projection maximale
du disque sur chaque axe ; elle n'utilise aucun arrondi. Le poids
`core_sites` est additionné une seule fois pour l'union des voies.

Rejeu tant que les archives temporaires sont disponibles :

```sh
python3 -B morsehgp3D_v9/audits/bbox_clipping_s2_20260923/measure.py \
  /tmp/mhgp9-lazy-prefix-audit-20260923/runs/full_k5/RUN.json \
  /tmp/mhgp9-v17-density-inputs-audit/s00_full_full.u32le \
  /tmp/mhgp9-v17-density-inputs-audit/s00_full_full.raw_return_ids.u32le
```

Les trois gros binaires d'entrée et les huit traces demeurent sous
`/tmp`, non versionnés. Leurs SHA et les scripts de production sont
conservés dans l'audit parent ; ce reçu compact ne prétend donc pas être
un artefact LIVE indépendant de ces données.

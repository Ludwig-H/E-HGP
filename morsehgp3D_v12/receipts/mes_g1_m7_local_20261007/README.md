# MES-G1 et MES-M7 en local : saut certifié sans census, profil de la résolution

7 octobre 2026. Mesures hors produit de la tranche T2-a ([contrat de la tour](../../docs/CONTRAT_TOUR.md) § 4.3,
§ 11), faites par un agent du développeur sur le codespace, relues et intégrées par le développeur. Comptes, cycles et
empreintes seulement : aucune coordonnée, aucun vidage. GCP non utilisé.

```text
phase=exploration_v12_hors_registre
backend=cpu_reference (microbancs hors produit, liés à la v11 gelée ac081a06f)
quantification=quantized_u21_input_only
public_status=not_claimed
```

Sources : [`microbancs/mes_g1_saut/`](../../microbancs/mes_g1_saut/README.md) (nouveau) et l'option
`--profil-resolution` de `mhgp12_vidage` ([`mes_m3_m4_tour/README.md`](../../microbancs/mes_m3_m4_tour/README.md)
§ 5.5). Bibliothèque de la v11 liée : `libmhgp11.a` `050532a9…` ; binaires : `mhgp12_mes_g1` `5cabe11c…`, son mutant
`37e7c84f…`, `mhgp12_vidage` `a3fcf6eb…`. Vidages complets de ng00, ng01, ng02 à K5 et de ng00 à K10 (hors dépôt).

## MES-G1 : comptes déterministes ([`mes_g1.md`](mes_g1.md), [`mes_g1.json`](mes_g1.json))

Part des censuses **saturés** (route 2) qu'un ensemble de candidats certifie sans census (k sites strictement
intérieurs exhibés par le prédicat exact ; un site sur la sphère ne compte jamais) :

| Cas | censuses saturés | voisins (K plus proches) | fenêtre de Morton 8 | fenêtre 16 | voisins ∪ fenêtre 16 |
| --- | ---: | ---: | ---: | ---: | ---: |
| ng00 K5 | 224 173 | 82,4 % | 79,8 % | 87,2 % | 95,4 % |
| ng01 K5 | 169 043 | 83,1 % | 80,8 % | 87,9 % | 95,6 % |
| ng02 K5 | 156 707 | 83,9 % | 81,1 % | 88,0 % | 95,7 % |
| ng00 K10 | 1 656 079 | 95,7 % | 83,6 % | 89,6 % | 98,5 % |

**Première moitié de la règle de `G-L3` tenue hors ligne** : au moins la moitié des censuses saturés évités à K5 sur
ng00–02 (83,0 % par les voisins, 456 534 sur 549 923). La seconde moitié (temps de G à un fil, borne haute de l'IC
95 % du rapport sous 1) exige un bras de résolution qui saute réellement : à jouer sur G4 ; `G-L3` n'est **pas
adopté** par ce reçu. Limite : la mesure suit les chaînes de la v11 ; quand la cible diffère (29 à 30 % des parties
certifiées à K5 par les voisins), la suite de la chaîne diffère aussi. Coût : 17 candidats et 12,6 tests exacts par
partie en moyenne à K5 par les voisins (au plus 30). Juge : 0 écart sur les quatre cas ; mutant « côté nul admis »
tué (porte et données réelles).

`LEM-HORS-CAT`, vérifié sur les seules sphères hors de $\mathrm{Cat}_K$ : **aucun contre-exemple** ; censuses complets
hors catalogue aux ordres $K-1$ et $K$ seulement, avec $(p,q_{\min})$ parmi $(K-2,4)$, $(K-1,3)$, $(K-1,4)$. Censuses
complets de sphères **du** catalogue ($S^{*}\not\subseteq F$), publiés à part : 3, 3 et 13 à K5 (ng00, ng01, ng02),
18 à K10 aux ordres 5 à 9. Sphères distinctes de route 3 : 43 054, 31 432, 34 948 à K5 et 223 592 à K10 (pour
`G-L4`).

## MES-M7 : profil local de la résolution ([`mes_m7_local.md`](mes_m7_local.md), [`mes_m7_local.json`](mes_m7_local.json))

Bras `replique_v12` (plus petite boule certifiée), un fil, compteur de cycles encadré par `lfence` (biais de 62 cycles
par section, passe 18 à 28 % plus lente que le bras non instrumenté), graines identiques au vidage, occurrences égales
aux routes. **Temps locaux** (codespace partagé) : parts indicatives, à confirmer sur G4.

| Cas | sondes | proposition + `LEM-T1` | certificat | census saturé | census complet | saut | trace stricte | reste |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| ng00 K5, ordres 2 à 5 | 41,6 % | 23,3 % | 2,3 % | 14,4 % | 6,5 % | 0,3 % | 1,3 % | 10,4 % |
| ng00 K5, ordre 5 | 34,1 % (285 ns) | 22,1 % (624 ns) | 3,4 % | 18,7 % (2,0 µs) | 11,6 % (2,8 µs) | | | |
| ng00 K10, ordres 2 à 10 | 32,9 % | 31,3 % | 3,5 % | 16,0 % | 6,9 % | 0,3 % | 1,7 % | 7,4 % |

Correction de l'estimation du contrat (§ 4.3, ajustement linéaire sur la session D) : le census pèse environ 21 % à K5
(non 35 à 48 %), et les **sondes de la table de populations** sont le premier poste (42 %), devant la proposition
(23 %). Conséquence pour l'ordre des leviers : `G-L5` (premières sondes sur l'appareil) et `G-L7` (disposition et
préchargement des sondes) d'abord, `G-L3` ensuite.

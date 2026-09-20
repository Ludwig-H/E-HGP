# Reprise du 20 septembre 2026 : audit des preuves existantes

Lecture seule effectuée sur `3e94c868`. Ce document ne constitue ni une
nouvelle exécution des moteurs, ni une recapture des qualifications historiques.
GCP non utilisé. Aucun build épinglé modifié.

## Vérifications exécutées

Les scripts de lecture existants ont été réutilisés, sans commande `run` :

```text
python -B morsehgp3D_v8/receipts/q2_front_inheritance_20260917/analyze.py <les 11 captures explicites de analysis_8ui2bes5/COMPLETION.json>
python -B -O morsehgp3D_v8/receipts/q2_front_inheritance_20260917/analyze.py <les mêmes captures>
python -B morsehgp3D_v8/receipts/q2_front_proposals_20260917/analyze.py <les 10 captures explicites de analysis_33ezki5_/COMPLETION.json>
python -B -O morsehgp3D_v8/receipts/q2_front_proposals_20260917/analyze.py <les mêmes captures>
```

Chaque analyse appelle les lecteurs stricts des captures : commandes et
résultats, logs bruts/décodés, empreintes de fermeture, géométrie appariée,
comptes et XML CTest. Les quatre commandes ont renvoyé zéro ; leurs sorties
JSON sont identiques entre normal et −O et aux `SUMMARY.json` enregistrés.
Les chemins exacts restent dans `capture_provenance` des fichiers de clôture
ci-dessus. Aucun fichier de reçu historique n'a été réécrit.

Un contrôle SHA256 supplémentaire sur les fichiers actuellement présents
a vérifié toutes les entrées des dictionnaires de clôture, sans désaccord :

| Tranche | Sources actuelles | Artefacts/builds | Entrées de l'analyse |
|---|---:|---:|---:|
| 21, héritage | 130/130 | 98/98 | 887/887 |
| 20, propositions | Non réattribuées au code actuel | 93/93 | 714/714 |
| 19, lots | Non réattribuées au code actuel | 94/94 | 218/218 |

Pour la tranche 19, les 26 lectures/analyses **enregistrées**, leurs empreintes,
codes de sortie et égalité normal/−O ont été contrôlés ; elles n'ont pas été
réexécutées. La distinction est volontaire : ses sources historiques ne sont
pas la source courante.

## Résultats réellement acquis

- Tranche 19 : résultat négatif conservé ; 172 mesures, lots plus lents que
  Coarse sur les 54 comparaisons aux tailles d'intérêt. Ne pas rouvrir cette
  piste sans hypothèse nouvelle.
- Tranche 20 : 684 mesures et 300 comparaisons appariées. La fenêtre 2K réduit
  réellement les produits transmis au census, pas seulement leur temps.
- Tranche 21 : 854 mesures et 372 comparaisons appariées. L'héritage complète
  ce gain ; les empreintes des supports restent identiques. Les journaux et
  XML authentifiés indiquent 81/81 CTests Release (124,95 s), 81/81 Clang
  ASan/UBSan (894,74 s), puis trois portes Clang TSan réussies. Ce sont des
  résultats du 17 septembre, pas des tests moteurs relancés le 20.
- La porte héritage enregistrée couvre 1 125 rejeux indépendants du front,
  711 237 paires rejetées jugées, 990 appels des cinq entrées q2 et dix mutants
  causaux, dont quatre non sûrs perdant 41 271 supports.

## Temps actuels mesurés de q2

Profil explicite `WspdFrontProposals{2, 16, true}`, Kmax=10, s=8, Pool64.
Temps mur front+census+collecte+callbacks, **sans** préparation du propriétaire
ni de l'index, sans q3/q4, catalogue ou FULL. W1 : minimum des trois captures
`scale_*` ; W4 : observation unique de `parallel_3pq0wb_6`.

| Famille | W1, 8k | W1, 16k | W1, 32k | W4, 32k |
|---|---:|---:|---:|---:|
| Uniforme | 2,110 s | 4,842 s | 10,506 s | 2,743 s |
| Terrain | 0,572 s | 1,184 s | 2,556 s | 0,666 s |
| Amas | 1,549 s | 3,788 s | 8,715 s | 2,262 s |
| Rangées | 0,228 s | 0,472 s | 0,968 s | 0,245 s |

La tranche 21 ne contient pas de mesure W4 à 8k/16k pour ce profil. Hôte
partagé, affinité libre ; ces temps ne qualifient aucun contrat G4. Le profil
historique demeure le défaut de l'API ; les nouveaux leviers sont explicites.

## Croissance : favorable sur ces mesures, pas une borne générale

Pour le même profil, les visites Z du census sont :

| Famille | 8k | 16k | 32k | Rapports aux doublements |
|---|---:|---:|---:|---|
| Uniforme | 36 685 357 | 87 534 555 | 194 637 006 | ×2,386 / ×2,224 |
| Terrain | 7 190 411 | 14 588 960 | 32 841 999 | ×2,029 / ×2,251 |
| Amas | 26 161 324 | 67 496 322 | 164 665 479 | ×2,580 / ×2,440 |
| Rangées | 5 716 373 | 11 831 297 | 24 451 379 | ×2,070 / ×2,067 |

Le nombre de produits visités croît ici de ×2,000 à ×2,435 par doublement.
Cependant la somme des facteurs Pool sur les rangées est 8 000/32 000/80 000 :
**×4 puis ×2,5**, à ne pas masquer par les compteurs précédents. Aucun
argument asymptotique général n'est acquis ; le nombre de seeds, le travail
q3/q4 et les sorties FULL restent à borner ou à rapporter comme coût de sortie.

## Limites et priorité de reprise

Pas de contradiction trouvée entre les reçus examinés et les gains annoncés.
Les trois limites structurantes restent inchangées : q3/q4 produit absents,
catalogue canonique absent, parents/plateaux FULL absents. Aucun GPU v8 ni
contrat 50k G4 n'est qualifié. Une mesure à 70k de la tranche 21 vérifie la
largeur des rangs hérités ; ce n'est pas une tour 70k.

L'option d'héritage est q2 seule ; `inherited_rejections` est un majorant,
pas le nombre exact de rejets supplémentaires. Le gain supplémentaire sur les
rangées est nul. Le faible gain restant des micro-variantes du proposeur ne
justifie pas de retarder les objets q3/q4 et le raccord FULL.

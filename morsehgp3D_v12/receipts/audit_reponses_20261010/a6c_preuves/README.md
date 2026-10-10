# A6c : provenance du critère et publication du retrait B3

10 octobre 2026. Lecture de métadonnées et de sources publiques épinglées à
`aa6338ee8`, sans moteur, compilation, payload de nuage ni appel GCP.

**A6c est intégré mais aucun nouveau chrono A6c n'est établi par ce reçu.**
Les mesures FULLN du produit R1 restent la référence descriptive adoptée.
Le retrait B3 au commit `1879eff9a` est exact : arbres `src/` et `tests/`
identiques à `8a0716e74` (tour 55, catalogue 34 mutants). Le reçu B3 publié
contient 438 fichiers, 4 770 302 octets ; ses 436 empreintes sont valides.
Ses 387 JSONL sont identiques aux JSONL de la session close, comparaison
effectuée lors de cette lecture, sans recopier l'archive privée.

Le pilote B3 conserve son SHA `6a798b31…` : la proposition de juge v2
`d9910490…` n'est pas intégrée. La publication ne crée pas les codes natifs
G ni les stderr individuels absents de la capture. « Identités établies »
dans son README doit conserver les réserves de
[l'admission indépendante](../../audit_reponses_20261008/session_t2db3_admission/README.md).
Le coût des transferts n'explique pas seul la régression ng02 : le contraste
clés/transfert échoue aussi, GM 1,009047, IC [1,006009 ; 1,011864]. Voir les
[statistiques](../../audit_reponses_20261008/t2db3_stats/README.md) ; la queue
relative à G n'est pas une mesure du travail supplémentaire de la forêt.

## Reconstruction du critère

`critere_a6c.json` annonce 40 trames mais publie **37 groupes**. La lecture
exacte de 44 journaux des deux sessions A6b reconstitue ses 594 passes de
base et ses coefficients : a = −32,5324151883119 ; b = 0,8092514473400847 ;
RMS = 2,836370286043444 ms ; LOO = 3,019935829512100 ms. Le modèle est donc
effectivement ajusté sur les **37 groupes**, sans pondération par leur
nombre de passes. « 40 trames » est une erreur de métadonnée.

Le retard ajusté est **médiane(fin noyau K) − médiane(fin G)** ; il ne s'agit
pas de la médiane des écarts appariés. Chaque ng combine 90 passes chaudes
contractuelles et 2 visites v12set. Les alias ng00/ng01 et leurs entrées
v12set sont de mêmes trames et mêmes nombres de sites mais de coordonnées
translatées différemment ([DONNEES, §8](../../../docs/DONNEES.md)) ; ne pas
les décrire comme une seule entrée identique octet pour octet. Ng02 a la
même translation. Les niveaux mathématiques sont invariants par translation.

Les 21 grandes trames ont 14 passes chacune sauf la première (12), le
premier passage de chaque processus d'identité étant écarté. Les 13 autres
groupes ont 2 passes chacun. `calibration.json` conserve seulement les
médianes temporelles, effectifs et nombres de sites ; aucun payload.

La règle d'activation à 43 900 sites a été calibrée sur les mêmes scènes
que le jugement futur. Le LOO de ce modèle ne fournit pas une validation
indépendante de la sélection du seuil ou des performances d'A6c. Une
campagne conforme pourra juger ces scènes ; une généralisation demande
des scènes supplémentaires tenues à l'écart de la calibration, notamment
autour du seuil. La règle et les seuils d'adoption doivent rester figés.

Détail d'arrondi sans conséquence géométrique : à 43 900 sites le modèle
prédit 2,99372335 ms ; « retard prédit ≥3 ms » correspond à 43 908 sites
entiers. La règle produit à 43 900 reste une règle explicite distincte.

## Rejeu léger

`python3 -S [-O] check.py --repo <repo>` vérifie l'empreinte du reçu,
le retrait B3, les 436 hashes publics, le pilote et la reconstruction OLS
depuis les agrégats épinglés. Le contrôle est indépendant du pilote A6c.

Pour rejouer aussi ces agrégats depuis les lignes temporelles originelles :
ajouter `--a6b-archive <archive results.tar.gz récupérée par a6br>` ; son SHA
est imposé dans `calibration.json`, les journaux A6b2 viennent du reçu public.
La relecture choisit seulement les JSONL temporels nécessaires, sans
extraction sur disque. Sans cette option, le contrôle ne prétend pas
réadmettre l'archive ou reconstruire les agrégats depuis les sources brutes.

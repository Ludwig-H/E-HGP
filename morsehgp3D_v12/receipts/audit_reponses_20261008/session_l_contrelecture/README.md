# Contre-lecteur L1/L2, avant admission

Aucun temps réel admis. [reader.py](reader.py) ne lance rien et ne lit aucun
contenu géométrique. Source L1 `403736300` : FULL avant `memoire_octets`, pilote
`84777f19`. Port explicite du lecteur K `d0751369` (`039b2657e`), original inchangé.
[capture.json](capture.json) épingle sources, plans et comptes déclarés repris de
[session_l_preparation](../session_l_preparation/README.md). La source L2 reste à
confirmer ; une sortie du nouveau schéma `memoire_octets` sera refusée.

Le lecteur contrôle clés exactes, u64 hors bool, JSON sans doublons/non-finis,
commandes, étiquettes uniques, séquence open/full/libération/exit. Empreinte FUL1
exigée seulement lorsque le plan la demande (sites≤1 600 000), comparée entre
passes et voies d'un même (scène,K). Aucun digest ni code manquant n'est inventé.

Durées : P+C+G+raccord+TMVR≤mur, T+M+V+R≤TMVR, tables+résolution≤G ; aucune
somme des diagnostics C imbriqués. Mémoire : 16×sites≤pic hôte≤160 Gio,
épinglé≤pic hôte, capacité appareil≤pic appareil≤88 Gio ; champs appareil nuls
sur CPU. RSS est un maximum du processus depuis son lancement, pas le budget.
CPU/RSS null restent null et font manquer un contrôle. Mur nul refusé avant log,
même si le pilote historique l'acceptait.

Refus, échecs, illisibles et non joués restent distincts ; les passes complètes
avant refus restent des diagnostics. Statistiques : première/dernière passe,
médiane et maximum des seules passes 1..P−1 ; P=1 reste froid. B1–B4 utilisent
la dernière passe, même froide : refus K5 toléré à ≥10 M, aucun refus K10 toléré,
B3 limité aux séries complètes. Rapport comparé aux bruts pour toutes les passes,
issues, empreintes, états des critères et verdict. Les phrases libres des détails
ne servent pas de preuve numérique.

Les codes sont déclarés par le rapport du pilote épinglé, ou confrontés à
`--codes` (objet tag→code). Source/binaire, provenance et arrêt sont à vérifier
séparément : `qualification_campagne=false`. Isolation vérifiée aux instantanés
avant/après seulement ; « non joué : délai » ne prouve pas le calcul du délai.

```sh
python -B morsehgp3D_v12/receipts/audit_reponses_20261008/session_l_contrelecture/test_reader.py
python -B -O morsehgp3D_v12/receipts/audit_reponses_20261008/session_l_contrelecture/test_reader.py
python -B morsehgp3D_v12/receipts/audit_reponses_20261008/session_l_contrelecture/reader.py \
  --repo /workspaces/E-HGP --session L1 --plan CHEMIN_PLAN_L1 --results DOSSIER_B
```

Normal/−O concordants : deux cohortes synthétiques, 24 corruptions, deux null,
quatre issues et un préfixe avant refus ; contre-épreuve via les seules fonctions
pures du pilote, jamais main/run/build. Le test lit les plans épinglés sous
`build/v12-data-20261007/plans`. Le lecteur hache les bruts et refuse leur mutation
pendant lecture ; code 1 si admission refusée, code 0 sans qualification de campagne.

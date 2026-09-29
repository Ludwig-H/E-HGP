# Contre-audit courant des bancs corrigés

29 septembre 2026, `cpu_reference`, hors registre, `public_status=not_claimed`.
Les deux causes initiales sont traitées sur la copie : groupe de processus
fermé au délai/signal, et décision refusée sur un plan incomplet. Un angle
mort distinct demeure : **un ARI_s de 1,25, impossible, est accepté puis
publié dans une décision complète**. Ce constat ne réfute aucun score des
lots A/C archivés ; nos valeurs sont des fixtures volontairement corrompues,
sans génération de nuages ni calcul de clustering.

## Copies relues, pas intégration implicite

Sources : `build/v10-fixes/bancs/src/morsehgp3D_v10`, base annoncée `0bce6cc00`.
Les six scripts nécessaires sont copiés dans notre
[snapshot](../../../receipts/audit_continu_20260929/bench_corrected/source/),
avec empreintes fermées avant/après leur exécution. Aucun banc ou moteur
source de l'autre acteur n'est modifié, aucune campagne lourde ou GCP lancé.

| Script corrigé | SHA256 |
| --- | --- |
| `bench/scaling/scale_run.py` | `f8d5e891a06b0549dd1de19aaad76e32cbe32e19f69b8dab3bd34cfd4ff5f6e2` |
| `bench/synthetic/decide.py` | `bab58df89c398479cb2a59791a7d23370937cfc51f5064037a76a160d5da7dd3` |
| `bench/g4/merge_sessions.py` | `2526cc3d2f4dee8347e052698dbfeccba050aee30e92934ff7dd5a7ec7ae7ca5` |

Le [reçu développeur observé](../../../receipts/audit_continu_20260929/bench_corrected/developer_observed/RECU_bancs.md)
et son [CTest terminé](../../../receipts/audit_continu_20260929/bench_corrected/developer_observed/f1_ctest_gate.txt)
portent 11/11, code 0, 2 296,49 s. Les longues portes géométriques ne sont
**pas rejouées** ici. Leurs réussites ne ferment pas les classes de mutations
absentes des juges, traitées dans le
[contre-audit distinct](../catalogue/CONTRE_AUDIT_JUGES_CORRIGES_20260929.md).

## Contrôles courts réellement exécutés

Le [harnais](../../../receipts/audit_continu_20260929/bench_corrected/record.py)
emploie seulement la fabrique de fixture dev du développeur ; ses mutations
et attentes sont définies dans notre propre script. Il ne génère aucun
point. Chaque capture a neuf commandes, soit **18 au total**, normal et
`−O`, avec mêmes résultats sémantiques ; les durées et PID diffèrent.

1. **Porte du délai relue puis rejouée sur snapshot.** Huit scénarios passent
   dans chacun des deux modes : enfant et petit-enfant réellement arrêtés
   et récoltés ; signal pendant le lancement ; conservation du JSON natif ;
   fichier des deux appels ; écart de boules refusé par code 3 ; délai CLI ;
   SIGTERM/SIGINT rendant 143/130 avec calcul fermé. C'est un rejeu indépendant
   d'une porte du développeur, pas une nouvelle porte de nettoyage écrite ici.
2. **Petit plan complet**, 32 scènes dev × trois méthodes : accepté.
3. **Une ligne manquante, doublon, scène hors plan ou ARI NaN non refusé** :
   chacun refusé par code 2, explication `REFUS`, aucun fichier de décision
   dans ces dossiers neufs.
4. **ARI_s=1,25 sur les 32 lignes `tour`, toutes non refusées** : accepté
   par `--check-only`, code 0, puis en décision complète, code 0.
   `DECISION.json` publie `mean_ari_s.tour=1.25`.
5. **NaN sur une ligne `refused=1`** : accepté conformément à la politique
   explicite de substitution du score refusé par zéro. Ce cas n'est pas
   une faille : la borne doit concerner les scores effectivement utilisés.

Les stdout/stderr, CSV, plans et décisions de fixture sont conservés dans
[normal_r2](../../../receipts/audit_continu_20260929/bench_corrected/normal_r2/receipt.json)
et [optimized_r2](../../../receipts/audit_continu_20260929/bench_corrected/optimized_r2/receipt.json).
Les refus et vérifications `--check-only` tournent sans site-packages (`−S`) ;
la décision complète utilise numpy avec un fil BLAS. Cette preuve n'est ni
une comparaison statistique tour/HDBSCAN, ni un benchmark de performance.

Le premier lancement du harnais avait omis `attribution_statement` dans
notre préenregistrement et échoué avant la publication de décision complète.
Son [snapshot et diagnostic](../../../receipts/audit_continu_20260929/bench_corrected/first_fixture_failure/README.md)
restent archivés. La correction porte uniquement sur notre fixture, suivie
de deux nouvelles captures ; aucun défaut produit n'est déduit de cet échec.

## Cause et corrections utiles

`finite_scores()` de `decide.py` teste `math.isfinite(float(...))`, pas la
borne du score. `check_run()` applique bien ce contrôle avant le calcul ;
un nombre fini mais impossible le traverse. Un ARI ne peut dépasser 1 :
dans sa formule, le nombre de paires communes est au plus la moyenne des
deux nombres de paires internes, donc le numérateur est au plus son
dénominateur positif ; les cas dégénérés ont un traitement spécial, jamais
un score de 1,25.

Ajouter une vérification du domaine des métriques utilisées, sur les lignes
non refusées, avec une politique de tolérance numérique explicitement
déclarée. Graver la fixture ci-dessus et garder le témoin `refused=1`.
Cela complète le contrôle de complétude ; cela ne demande aucun changement
géométrique, nouvelle scène test ou campagne G4. La vérification des scores
ne remplace évidemment pas leur recalcul depuis labels et vérité terrain.

Le correctif du délai reste pertinent : nouvelle session avant calcul,
`killpg` avant récolte, subreaper Linux, report des signaux de lancement.
Sa limite annoncée — SIGKILL direct du superviseur sans SIGTERM préalable —
reste distincte. L'ancienne sonde d'audit exigeait l'ancien groupe commun
et échoue désormais par construction ; ne pas compter ce changement
d'architecture comme un échec de l'arrêt. La porte relue ci-dessus contrôle
les nouveaux groupes directement.

Le checkout produit observé garde les scripts anciens ; intégration,
épingle du prochain préenregistrement et régression du binaire combiné
restent sous la responsabilité du développeur. Nos empreintes qualifient
cette copie, pas un mélange des différentes copies corrigées.

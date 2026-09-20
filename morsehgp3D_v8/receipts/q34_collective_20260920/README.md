# Qualification du filtre collectif q3/q4 — tranche26

20 septembre2026, après8d0a0f0f ; `cpu_reference`, entrée u16,
`public_status=not_claimed`. Code, preuves et [note mathématique](../../docs/Q3_Q4_CERTIFICAT_COLLECTIF_20260920.md)
propres au port constructeur. Les modèles de l'auditeur A4215dd16 ne
qualifient pas ce port et son autre pool n'est pas utilisé ici.

## Ce qui est comparé

Même arête, même nuage/index/cover et **même pool spatial** pour les
quatre options : Jung/Variance × Universal/Collective. La jumelle est
le filtre universel de la tranche25, pas le chemin sans filtre24.
Chaque résultat compare physiquement les candidats, supports, profondeurs
et coquilles avant le digest. Les sorties n'incluent pas les intérieurs
et ne constituent pas un catalogue global ni une tour FULL.

Le temps de chaque bras inclut génération, propriétaire, index, cover,
pool et exécution avec callback. Cette préparation réellement mesurée
est commune aux deux sommes : ce ne sont pas deux temps mur indépendants.
Le temps mur apparié inclut aussi les vérifications et libérations.
Une observation par configuration, sous qualification concurrente ;
aucune accélération stable n'est inférée de différences de quelques %.
La campagne scale est fixée au CPU logique0, un thread par sonde.

## Captures

| Capture | Périmètre |
|---|---|
| `smoke_r9g1vw9j` | Release : cinq gates,24 mesures |
| `smoke_pl8reky9` | Clang ASan/UBSan : cinq gates,24 mesures |
| `scale_3b2oi8i0` | Release : cinq gates,160 mesures |
| `regression_17ztzy1b` | Suite complète87 CTests Release |
| `mutants/compiled_n8onzuz3` | Trois mutants compilés, dix commandes baseline incluse |
| `mutants/differential_ctdn_04g` |20 paires contre le binaire épinglé25,40 commandes |
| `dense_forecast/forecast_0nz9r_pb` |48 configurations de filtre dense, repli non exécuté |

Toutes ces captures sont closes PASS. `readers_9li9rpmq` conserve dix
lectures/contrôles : Python normal et−O donnent les mêmes résultats,
33 corruptions du lecteur sont détectées.163 sources,65 artefacts et
234 fichiers d'entrée sont inchangés avant/après cette fermeture.
Les auxiliaires conservent leurs propres lectures et empreintes dans
leurs fichiers `READBACK.json`, `MUTANTS_READBACK.json` et
`DIFFERENTIAL_READBACK.json`, sans devenir des sources produit.

Les208 mesures principales incluent96 mesures8k/16k/32k, K5/10,
C32/64, quatre options, sur les fonds `far` et `cap` à seulement deux
faces. Le régime `adversarial`, à32/64/128/256, fait croître faces et
cover ensemble. La projection dense auxiliaire est distincte : elle
mesure tous ses filtres à8k/16k/32k mais n'exécute pas leurs familles.
Elle ne doit pas être ajoutée aux mesures complètes de candidats.

La nouvelle gate passe8549 contrôles :349 évaluations de filtre,
81 minima rationnels exacts, deux gains collectifs positivement exercés,
50 bornes resserrées et quatre numérateurs naïfs dépassant127bits.
Les racines aux bornes, groupes mixtes égaux, creux ponctuels, comptes
qui redescendent, quatre classes de masques, extrêmes u16 et coquille30
sont exercés.244 appels par arête et348 par seed, avec oracle rationnel,
complètent les contrôles de possession, exception et concurrence privée.
Les cinq gates passent aussi sous Clang ASan/UBSan ; **ce n'est pas la
suite complète87 sous sanitizers**, ni une nouvelle qualification TSan.

Les [mutants](mutants/README.md) sont tués mathématiquement, pas par une
erreur de compilation. Le différentiel retrouve tous les champs JSON
hors temps du chemin25 à budget64. Les nouvelles options restent
explicites, les chemins précédents et q2 ne changent pas de défaut.
Les erreurs de développement antérieures au gel sont décrites dans
[PREFLIGHT.md](PREFLIGHT.md), pas effacées par ces succès.

## Résultats et coût réellement payé

Adversaire256, C64 ; lectures du repli avec collecte des sorties.

| Mode | Lectures K5 | Lectures K10 | Tri du filtre K5 / K10 |
|---|---:|---:|---:|
| Jung + Universal (référence25) |15 616|33 536|0 / 0|
| Jung + Collective |11 179|23 314|17 875 /40 984|
| Variance + Universal |13 568|31 744|0 / 0|
| Variance + Collective |10 667|21 266|14 815 /37 134|

Les sorties restent14 K5 et54 K10. Avec Variance+Collective, les
comparaisons de tri du repli passent de147 678 à97 021 K5 et de316 486
à198 198 K10 ; **les tris du filtre s'ajoutent**. Ses propositions sont
8 557/11 993 et les deux racines entières coûtent chacune5588 itérations.
Le pic couplé workspace+buffers du repli vaut4640octets, hors nuage,
index, pool, callbacks, temporaires d'allocation et RSS.

Les sommes de temps des bras à cette seule configuration donnent
4,690→3,658ms K5 et9,545→7,280ms K10. Sur toute la campagne, l'option
Variance+Collective régresse aussi : ratio option/référence jusqu'à1,56
sur les petits adversaires,1,14 sur `cap`. Les grands fonds ne gagnent
aucun rejet ; les variations de temps de ces fonds ne prouvent donc
pas un gain géométrique. Le chemin ne devient pas le défaut universel.

Les lectures Variance+Collective/C64 sur le petit adversaire donnent
960/2176/4602/10667 K5 et960/2432/7419/21266 K10. Ces ratios sous×4
sur quatre petites tailles ne prouvent pas la croissance globale :
la [projection dense8k/16k/32k](dense_forecast/README.md) contredit
précisément cette généralisation.

À32k dense etC64, il reste5792 familles q4 K5 et11 195 K10 : au moins
185,344M et358,240M lectures futures pour le repli actuel **non exécuté**.
C'est28,2%/24,7% de moins que Jung+Universal, mais le dernier doublement
multiplie ce minimum par9,694/6,223. Le filtre a réellement payé
1,918M/3,730M comparaisons de petits tris en plus. La recette dense
change de couches avec n : ces ratios observés ne sont pas une preuve
asymptotique, mais le régime mesuré n'est pas sous-quadratique.

## Décision

Conserver le filtre collectif qualifié comme option. Ne pas multiplier
ces recherches par toutes les arêtes WSPD sans réduire leur résidu.
Priorité suivante : carte de minorants commune aux faces dans le plan
des centres, sans arrangement exhaustif de droites ; mesurer construction,
requêtes, cas indécis et repli, pas seulement ses rejets.

Les163 sources et les deux builds `v8_q34_collective_20260920` et
`v8_q34_collective_sanitize_20260920` sont désormais épinglés.
Le s8/10/12 ne figure pas dans cette primitive à arête fournie ;
sa comparaison revient au raccord WSPD. Générateur q3/q4 global,
catalogue, intérieurs après regroupement, FULL et contrats50k/G4/massif
restent ouverts. GCP non utilisé.

# Dialogue courant de l'auditeur indépendant B (v8)

21 septembre 2026, après **c5308651** (tranche 31 en chantier, non commise),
sur main. Canal rouvert : l'auditeur B reprend son rôle d'auditeur indépendant
après avoir été constructeur du 17 au 20 septembre (tranches 19 à 21) ; il ne
requalifie jamais son propre code de cette période. Écritures limitées à
`morsehgp3D_v8/audits/` ; l'auditeur A conserve
[DIALOGUE_COURANT.md](DIALOGUE_COURANT.md) et ses notes P0_*, l'auditeur
complémentaire `audits/morsehgp3D_v8_complementaire/`, le constructeur ses
sources, docs, reçus et [ETAT_COURANT.md](ETAT_COURANT.md).
`phase=exploration_v8_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.
GCP non utilisé.

Cadre fixé par l'utilisateur pour cette reprise : l'objet est le clustering
hiérarchique par polyèdres des niveaux K-NN (la tour HGP par niveaux) ; on
audite son **calcul** (exactitude, complétude, coût sur les régimes visés,
SemanticKITTI en tête), **pas encore sa pertinence** : aucune confrontation à
une vérité terrain n'est menée pour l'instant. L'oracle indépendant
[oracle_q3q4_20260915/](oracle_q3q4_20260915/README.md) reste prêt ; A mène
le contrat global exhaustif dans `q34_global_contract_20260921/` et B ne le
duplique pas.

## Réponses aux deux questions du journal du 21 septembre

Mesure d'appui : [front_lanes_lidar_20260921/](front_lanes_lidar_20260921/README.md),
18 exécutions (scans 0/100/200, n = 8k/16k/32k, K = 5/10, s = 8) sur les
sources c5308651 épinglées, 2 000 paires jugées exactement par voie et par
exécution, reçu rejoué en `python3` et `python3 -O`, aucun flottant.

### Question pratique : K témoins autour du pivot, ou crédits h_a/h_b par blocs ?

**La fenêtre de K témoins autour du pivot est la faiblesse dominante ; les
crédits par blocs n'apporteraient rien.** Les témoins exacts situés dans A ou
B représentent 1 à 4 % des témoins du citron, sur les deux voies et les 18
exécutions. À l'inverse, un proposeur ponctuel parfait (compte saturant des
sites universels pour les boîtes, par descente de l'index) retirerait 82 à
91 % de la masse résiduelle q3 et 75 à 89 % de la masse q4, quand la fenêtre
2K n'en retire que 36 à 56 % (q3) et 6 à 55 % (q4, 6 à 16 % à K = 5), et la
fenêtre 4K à peine plus. Élargir la fenêtre ne rattrape donc pas le plafond :
il faut chercher les témoins par **descente saturante de l'index** (les bornes
conjointes de boîte existent déjà), pas par rangs autour du pivot.

Au-delà du plafond de boîte, 88 à 97 % des paires résiduelles sont rejetables
exactement (au moins h_q sites dans leur citron), part qui croît avec n ; 40
à 67 % de leurs témoins sont hors A∪B sans être universels pour les boîtes.
Le reste de la réduction passe donc par un test **par paire**, mesuré
ci-dessous, jamais par un scan local quadratique.

### Question prioritaire : census q3 par boîtes, certificats avant les clés

**Le census par boîtes de la puissance exacte est validé, mais ce n'est pas
lui qui supprime les 780 M seeds : c'est le rejet exact de la paire avant sa
couverture.** Sur les 18 exécutions, une paire rejetable porte 2 000 à
13 000 sites de couverture et 350 à 1 950 seeds aigus (le constructeur mesure
341 par arête à 8k/K5, en accord), alors qu'une paire conservée porte 47 à
792 sites de couverture en moyenne (maximum 11 225) et 8 à 125 seeds. Rejeter
la paire d'abord divise la masse de seeds par 87 à 1 519 selon l'exécution
(tableau des coûts du reçu), et les survivantes se censurent à bon
compte même naïvement. Ordre recommandé, avant covers et seeds :

1. Par rectangle résiduel : descente saturante de l'index avec les bornes de
   boîte (le « plafond » mesuré), coût total 46 à 851 M visites de nœuds par
   exécution dans mon harnais mono-fil, soit 3 à 48 s pour les deux voies.
2. Par paire restante : descente saturante avec les boîtes singleton de a et
   b, enfant le plus proche du milieu de ab visité en premier : 43 à 148
   visites par paire rejetable q3 et 63 à 222 en q4 selon l'exécution (quatre
   à six fois moins qu'en préordre), décision identique au balayage complet
   sur les 72 000 paires jugées.
3. Couverture serrée pour q3 : |2z−a−b|² ≤ 3|b−a|² suffit (centre à moins de
   D/√3 du milieu et rayon au plus 2D/√3, D demi-longueur, donc tout intérieur
   à moins de √3·D du milieu), 1,54 fois moins de volume que 4|b−a|² ; en q4
   la couverture 4|b−a|² reste nécessaire ((1/√2 + √(3/2))·D < 2D).
4. Census par seed sur les survivantes, naïf ou par boîtes.

Sur le census par boîtes lui-même (proposition du contrat 31) : la fonction
A|z|² + B·z + C est convexe et séparable, son minimum entier par axe est
atteint au plancher ou au plafond du sommet −B_i/(2A) rabattu dans l'axe, son
maximum à une extrémité ; minimum > 0 écarte le nœud, maximum < 0 admet son
cardinal, saturation à h_q, chaque nœud compté une fois : exact. Deux
précisions : (i) séparer les deux phases, un compte saturant des intérieurs
stricts qui écarte aussi les nœuds à minimum = 0 (les contacts ne servent
qu'à l'acceptation), puis, pour les seules boules acceptées, une seconde
descente des nœuds à minimum ≤ 0 ≤ maximum pour récupérer la coquille
entière ; raffiner les égalités pour toutes les boules rejetées paierait pour
rien. (ii) Visiter d'abord le sous-arbre du centre : la saturation d'une
boule profonde se joue près du centre, l'ordre préordre gauche-droite est
quatre à six fois plus cher, comme mesuré ici pour les citrons.

Certificat de famille avant les clés : pour une paire survivante, le compte
c < h_q du citron est un plancher d'intérieurs commun à toutes ses boules
propriétaires ; chaque seed n'a plus à trouver que h_q − c intérieurs dans la
couverture serrée. Avec 8 à 125 seeds par paire survivante, un certificat
plus fin (secteurs du disque des centres dans le plan bissecteur) n'est pas
nécessaire aujourd'hui ; ne pas construire d'index par seed.

**Lemme du citron, vérification exécutable (q3)** : sur 1 800 paires
rejetables et tous leurs seeds aigus propriétaires, aucune circumboule n'a de
profondeur < h_q (0 violation). Confirmation par instances de la preuve de A,
pas une preuve ; q4 n'est pas vérifié de cette façon.

### Citron : accord avec A, une phrase à préciser

Accord avec la preuve de A par la variance (R² ≤ D²(q−1)/(2q), α3 = 3,
α4 = 2, inégalités strictes, contacts réalisables en u16). **Une phrase de sa
réponse est à préciser** : « utiliser α3 pour q4 aussi ». Si elle signifie
prendre α = 3 sur la voie q4, c'est non sûr : α4 = 2 est exact et serré.
Contre-exemple entier : tétraèdre a = (0,0,0), b = (60,0,0), c = (20,42,0),
d = (28,10,49), arête ab maximale, centre strictement intérieur
(30, 241/21, 28885/2058), r² = 5203980749/4235364 ; le site z = (28,−12,−12)
a H = 608 et Ξ = 1 036 800, donc 2H² = 739 328 ≤ Ξ < 3H² = 1 108 992 : z est
dans L_3(a,b) mais **strictement extérieur** à cette boule q4
(|z−o|² = 5222107613/4235364 > r²). Un front qui le créditerait pour q4
pourrait rejeter à tort l'arête. Le contrat 31 garde α4 = 2 ; à confirmer
dans le code de la tranche 31 avant sa commission.

## Erreurs et points durs relevés (à c5308651 et sur le chantier 31)

1. **Session G4 R2 : diagnostic non établi.** La capture
   `receipts/lidar_global_20260921/gcp_r2_gate_failure/` montre
   `mhgp8_wspd_q34_gate --selftest` sans aucune sortie pendant 350,6 s puis
   tué (exit −15) par la session, sans chien de garde par commande ni trace de
   pile ; arrêt certifié `TERMINATED`. La suspicion « rational/int == 0 sous
   ancien Boost/C++20 » est plausible mais non prouvée : avant de conclure,
   reproduire sur la chaîne de la VM avec `timeout -s ABRT` et une trace
   (`gdb -batch`), ou un chien de garde par commande dans le worker. La
   réécriture par `numerator() == 0` est équivalente pour un rationnel
   normalisé (dénominateur > 0 garanti par Boost), donc sans risque, mais elle
   ne vaut correction que si R3 passe la même porte.
2. **Chiffre sans reçu.** « 210 987 arêtes, 83,307 M incidences, environ
   10,5 s » (journal, pilote 1000/K10) n'a pas de capture dans
   `receipts/lidar_global_20260921/` : le doter d'un reçu ou le retirer.
3. **Formulation d'ETAT_COURANT** : « la contrelecture A confirme
   indépendamment le citron » désigne une preuve sur papier ; la campagne
   exécutable de A est en préparation (son essai LSan a échoué), la mienne
   ci-dessus couvre q3 par instances. Écrire « preuve indépendante de A,
   vérification exécutable q3 par B, q4 en attente ».
4. **Compteur `owner_tests`** (`wspd_q34.cpp`, deux incréments par seed aigu,
   un par test de propriétaire) : sémantique acceptable, à noter dans le
   registre pour que `owner_tests ≥ acute_seeds` ne soit pas lu comme une
   anomalie.
5. **Budgets par défaut de Local28** exercés par la porte sur deux appels
   seulement : ajouter des fixtures bornées où le budget par défaut mord
   (frontière active saturée) et où il ne mord pas, avec planchers.
6. **Archive de 28 256 326 octets** (`global_vwtz76da.tar.gz`) dans les reçus :
   sa relecture n'est pas `--check-live` et ses entrées ne sont pas dans le
   dépôt ; garder l'archive hors Git (empreinte seule dans le reçu) pour ne
   pas alourdir l'historique sans rejouabilité.
7. **`--check-live` des tranches 22 à 30** échoue sur l'arbre courant (sources
   et CMake modifiés par la tranche 31, attendu) : chaque README de reçu
   devrait nommer le commit auquel sa relecture vivante s'applique, pour que
   le lecteur sache faire `git worktree add --detach <commit>`.
8. **Rédaction** : Q3_Q4_COVERS_PARTAGES (§ couverture) parle d'« un minimum
   de distance [qui] dépasse D² » ; comparer des carrés à des carrés
   (|2z−a−b|² à 4D²) comme dans `edge_cover.hpp`.

Journal et sessions GCP : la mention « aucune VM démarrée » précède les
sessions R1 (07:17 UTC) et R2 (07:25 UTC) que l'entrée suivante déclare ;
lecture chronologique correcte, rien à corriger. Les deux captures sont
conservées et les arrêts certifiés `TERMINATED` sur leur cible ; GCP non
vérifié en direct par B (pas de `gcloud` ici).

## Entretien du dossier

- Fichiers de B : ce dialogue ; les notes datées
  [REGIME_WSPD_20260914.md](REGIME_WSPD_20260914.md),
  [PROPAGATION_TEMOINS_20260914.md](PROPAGATION_TEMOINS_20260914.md),
  [CREDITS_TERMINAUX_20260914.md](CREDITS_TERMINAUX_20260914.md),
  [BUDGET_CONTRAT_50K_20260914.md](BUDGET_CONTRAT_50K_20260914.md),
  [PORTES_ET_TESTS_20260914.md](PORTES_ET_TESTS_20260914.md),
  [VERROUS_MATHEMATIQUES_20260914.md](VERROUS_MATHEMATIQUES_20260914.md),
  [SEPARATION_20260914.md](SEPARATION_20260914.md), chacune coiffée ce jour
  d'un statut (historique ou acquis) ; les reçus immuables
  `wspd_regime_20260914/`, `propagation_temoins_20260914/`,
  `credits_terminaux_20260914/`, `chaine_q2_20260914/`,
  `separation_20260914/`, `surproposition_20260915/`,
  `plafond_proposeur_20260915/`, `oracle_q3q4_20260915/` et
  `front_lanes_lidar_20260921/`.
- Rien n'est supprimé ni déplacé : chaque ancien fichier est cité par un reçu
  immuable, une note du constructeur ou le journal (précédent
  `P0_OWNER_CHECKS.json` à ne pas répéter). Les sections antérieures de ce
  dialogue (14 au 17 septembre) sont condensées ci-dessous ; leur texte
  intégral se lit par `git show 36bef318:morsehgp3D_v8/audits/DIALOGUE_AUDITEUR_B.md`.
- Ne pas déplacer `P0_INPUT_ALIAS_CHECKS.json`, `p0_q2_census_bounds_probe.py`
  ni `p0_collective_probe.py` (épinglés ou importés ailleurs).
- Contrôles : Markdown de ce dossier validés par la fonction `validate` de
  `tools/check_docs.py` ; reçus rejoués en `python3 -O` ; aucun fichier des
  autres acteurs modifié.

## Historique condensé de l'audit B (14 au 17 septembre)

Chaque tranche q2 publiée a été confrontée à la force brute exacte sur des
sources identiques, haché par haché, aux blobs du commit indiqué ; reçus
dans `chaine_q2_20260914/`, rejouables par `git archive <commit>`.

| Tranche | Commit | Reçu | Résultat |
| --- | --- | --- | --- |
| Front + census q2, frère, ordre Complement | e3af11a7 | `CHAINE_Q2_CHECKS.json` | 86 × 8 combinaisons, 104,7 M paires, 0 désaccord |
| Census conjoint A × B | b2106c3c | `CHAINE_Q2_JOINT_CHECKS.json` | 86 × 18, 235,7 M paires, 0 désaccord, admission conjointe nulle comme prédit |
| Filtre Pool terminal | ba11e3ab | `CHAINE_Q2_POOL_CHECKS.json` | 86 × 25, 327,3 M paires, 0 désaccord ; seuil 64 = 99,94 % de la masse filtrable |
| Workers du front et du census | b268cf6f | `CHAINE_Q2_PARALLEL_CHECKS.json`, `…_BALANCE_…` | 4 348 appels, W ∈ {1,2,3,4,8}, sorties identiques ; ×4,1 à ×4,9 à W = 8 |
| Redistribution dynamique | 4e878754 | `CHAINE_Q2_DONATE_CHECKS.json` | 13 044 appels, 1,95 M dons repris, 0 blocage |
| Continuations à ancre unique | d09e2207 | `CHAINE_Q2_RESUME_CHECKS.json` | 258 624 continuations, 0 désaccord |
| Détachement intérieur | 897085f8 | `CHAINE_Q2_DETACH_CHECKS.json` | 63 120 lignées, 504 960 appels parallèles, 0 désaccord |
| Équipe persistante | beee3341 | `CHAINE_Q2_COOP_CHECKS.json`, `…_COOP_SCALE_…` | 5 304 appels, 0 désaccord, 258 exceptions propagées |
| Plages d'ancres et Pool partagé | 2741d614 | `CHAINE_Q2_RANGES_CHECKS.json` | 5 256 appels, 0 désaccord |
| Lots singleton (sources en chantier) | — | `CHAINE_Q2_BATCHED_*` | obligations des feuilles B compactées tenues |

Mesures et résultats acquis à côté : régime WSPD v4 et convention de
séparation (`wspd_regime_20260914/`), lentille réfutée et propagation
chiffrée du premier front (`propagation_temoins_20260914/`), crédits
terminaux et survivantes de Pool sur les amas (`credits_terminaux_20260914/`),
séparation s ∈ {8, 10, 12} : même objet, s = 8 confirmé
(`separation_20260914/`), bilan net de la surproposition sur copie patchée
(fenêtre 2K : temps q2 à 42 à 68 % de la référence hors rangées,
`surproposition_20260915/`), plafond de tout proposeur de témoins q2
(`plafond_proposeur_20260915/`), oracle q3/q4 i128 identique au catalogue
rationnel de `reference/` sur 315 petits nuages (`oracle_q3q4_20260915/`),
campagne adversariale sur 1bf806f0 sans défaut d'exactitude survivant.
Corrections acquittées : chiffres de la note des crédits, invariant I1,
argument des rangées (cordes 2u, témoins W3 pour u > D/√3). Les tranches 19
à 21 (8d615cfd, 8190e7ab, 3e94c868) ont été livrées par B en tant que
constructeur et relèvent des contrelectures de A et de l'auditeur
complémentaire, pas de ce canal.

# Audits courants de la v11

5 octobre 2026. Cadre : `exploration_v11_hors_registre / cpu_reference /
quantized_u21_input_only / not_claimed`. Notes maintenues en place ;
preuves détaillées dans les reçus immuables.

**Réponse avant S10 :** refuser l'appel entier si une comparaison EOM
épuise son budget ; réutiliser les preuves exactes existantes et compléter
le port de z=2. Aucun troisième oracle complet n'est demandé.
[Réponse aux trois questions du développeur](AUDIT_REPONSES_AUX_VERROUS_MOTEUR_20261002.md#réponse-aux-trois-questions-avant-s10--note-développeur-d26328fe2).

**L2b en cours : retirer le diagnostic du chemin normal.** Dans le
brouillon capturé, `compute_supports` active le hachage de tout le journal
des graines même quand aucun diagnostic n'est demandé. Ce parcours
supplémentaire est payé dans l'étage `tree`. Propager un pointeur nul
jusqu'à `build_order_full` dans ce cas, avant la mesure de la nouvelle voie.
[Constat de source et correction ciblée](../receipts/audit_l2b_wip_20261005/README.md).

**S9 : correctif du refus de tri publié en 3d47eaa93.** Les quatre fichiers
correspondent exactement à la capture relue favorablement. Le tri par
tas renvoie immédiatement l'`Outcome` refusé ; l'ordre de comparaison ne
change plus. La contre-épreuve Python contrôle l'arrêt à chaque position
de refus, les bornes et la permutation. Le refus remonte jusqu'au CLI
avant publication, en lecture du code. La qualification native G4 reste
à confirmer ; le mutant `tri_refus_ignore` accompagne la publication.
[Correction et portée](../receipts/audit_s9_sort_fix_20261005/README.md),
[défaut initial conservé](../receipts/audit_s9_wip_20261005/README.md).

**Avant de qualifier S9 sur G4 : conserver le différentiel exact sur LiDAR.**
La matrice c97776ea8 exclut les quatre portes `points_vs_python`, dont les
trois trames entières. Les succès locaux sont distincts ; le lecteur de
fichier et l'oracle borné conservés ne remplacent pas cette identité
complète à K5. Un plan ciblé avec Python épinglé est préparé et validé
localement, sans exécution G4.
[Périmètre et plan prêt à intégrer](../receipts/audit_s9_qualification_scope_20261005/README.md).

**Décision L2 : livrer L2b.** La mesure G4 appariée déclenche la règle
fixée avant la campagne : raccorder le journal des graines à la voie
concurrente de FULL pour `supports`. L'identité des fichiers entre prises
est conforme ; le complément W48 sur ng00 et ng02 passe. Le nouveau
chemin devra conserver les octets MHGP11SP et être qualifié sous TSan.
[Mesure, décision et limites](../receipts/audit_l2_decision_20261005/README.md).

**G4 A2 : la sélection ordinaire est entièrement conforme.** Sur
**b319efc84**, les profils u18/u21/u24 et poison u21 passent respectivement
**914/824/824/825 tests**, sans échec ni résultat manquant ; arrêt ciblé
certifié. Les dix portes CLI ordinaires auparavant manquantes passent.
Les tests longs non mutants et le complément supports W48 passent depuis
dans leurs sessions dédiées. La couverture sanitizer reste partielle :
c97776ea8 avait retiré les portes d'échelle et LiDAR ; **a7711b506 les
rétablit en huit lots dédiés**, encore à exécuter.
Aucun contrat 100 ms n'est acquis.
[Reçu A2 vérifié](../receipts/audit_g4_a2_20261005/README.md).
Les premières sessions interrompues et les **459 mutants u18 détectés**
gardent leur [preuve distincte](../receipts/audit_g4_sorties_20261005/README.md).

**G4 B : couverture partielle sous sanitizers.** ASan/UBSan
u24 passe 774/824 tests, TSan u21 759/824 ; les autres n'ont pas de résultat
après l'échéance globale. Aucun échec individuel terminé ; arrêt ciblé
certifié. Les portes API/IO et oracles supports passent dans les deux
configurations. Les **50 et 65 portes manquantes**, retirées par
c97776ea8, sont reprises par le découpage a7711b506 : deux lots ASan/UBSan
et six TSan. Aucune nouvelle exécution n'est encore constatée ; A2 ne
les qualifie pas sous instrumentation.
[Reçu B vérifié](../receipts/audit_g4_b_20261005/README.md).

**S8 : arithmétique exacte relue, qualification native à poursuivre.**
La tranche **53c027fe8** prépare les sommes de radicaux de la sortie
points. Aucun défaut important nouveau établi : regroupement des classes
certifié, zéro prouvé, intervalles stricts et refus explicite si la capacité
ne suffit pas. La contre-épreuve Python indépendante concorde ; A2 précède
S8 et ne la qualifie pas. S9 est publiée depuis en 3d47eaa93.
[Revue, preuve bornée et limites](../receipts/audit_s8_20261005/README.md).

**Audit général au pin 238734f1d, suivi de S3/165def5ab et S6b/9e7428995.** Aucun nouveau défaut
mathématique FULL établi ; contre-épreuve du nerf complet, des coupes et
des verticales conforme sur 65 ordres. Le point important du raccord est
la qualification de l'assemblage L1 entier. L'identité Session et la
cohérence du manifeste sont corrigées dans les sources S5 publiées.
La mesure avec journal actif et sortie complète a depuis retenu L2b.
[Verdict et priorités du développeur](AUDIT_CONTRATS_NUMERIQUES_ET_CAPACITE_20261002.md#audit-général-du-5-octobre--décisions-importantes),
[preuve et périmètre](../receipts/audit_geant_20261005/README.md).

**S5 : manifeste corrigé, aller-retour natif conforme sur G4.** La provenance est
contrôlée contre le nombre de points avant toute écriture ; les tailles
incohérentes et le budget déclaré nul sont refusés. La nouvelle porte
fait relire une publication API par le lecteur officiel et vérifie les
quatre refus sans dossier créé. La porte publication/lecteur passe en
normal et `-O` dans les quatre configurations de la session A ; la
qualification globale reste incomplète. Les 27 portes API, y compris
l'identité Session avec produit vivant, y sont toutes conformes.
[Correction et portée](../receipts/audit_corrections_s3_s5_20261005/README.md).

**Suivi des corrections.** Identité stable de Session, contrôle de fin de
vie et hook IO typé sont corrigés dans S5 ; le correctif du manifeste est
publié en **a5e4019b4**, identique à la capture relue.
S6a est publiée sur `main` en **19b2fb218**, identique aux sources déjà
relues en ee8a69f1a. S3 est publiée en **165def5ab**, avec E1/E2 sur le
catalogue étroit et D2 ; le différentiel contre l'oracle exact et le témoin
K10 sont maintenant inscrits dans ses portes. L'assemblage S6b est publié
en **9e7428995** et relu favorablement ; S7 est publiée en **966a351be**,
qualification G4 limitée au périmètre ci-dessus. Pour la future L3, K=n est refusé par `points`/`plat`
quand K≥2 ; FULL/supports le conservent.
[Contrelecture précédente](../receipts/audit_l1_followup_20261005/README.md).

**S6b : assemblage relu.** Tous les supports sont conservés, y compris
ceux sans coface. Le plafond de 24 sites est contrôlé sur l'appel entier
avant allocation ; count/fill gardent les mêmes positions et des tampons
privés par worker. Comptes et admission mémoire restent dans leurs bornes.
S7 livre désormais le fichier ; résultats G4 et périmètres ci-dessus.
[Périmètre et contre-épreuves](../receipts/audit_s6b_20261005/README.md).

**S7 : sortie complète livrée, qualification à terminer.** L'API, l'écrivain
MHGP11SP et le lecteur sont relus sans défaut important établi. Les portes
comparent le fichier à S1 et la signature d'arbre à FULL. Les permutations
et réétiquetages **W48 sur ng02 et ng00** sont désormais conformes, avec
14 appels par porte, au pin b319efc84.
[Revue et complément W48](../receipts/audit_s7_20261005/README.md).

- [Mathématiques : supports, frontières, hiérarchies et sélection](AUDIT_REPONSES_AUX_VERROUS_MOTEUR_20261002.md).
- [Moteur : qualification, performances et intégration](AUDIT_CONTRATS_NUMERIQUES_ET_CAPACITE_20261002.md).
- [Question courante du développeur sur 100 ms](QUESTION_CLAUDE_VITESSE_100MS_20261004.md).
- [Réponse courante du développeur sur les supports](REPONSE_CLAUDE_SUPPORTS_20261004.md) : demandes de l'audit général adoptées, frontière K=n tranchée pour L3.
- [Contrat courant des points](../docs/HIERARCHIE_POINTS.md), avec
  [échange du 3 octobre archivé](../receipts/audit_supports_contract_20261005/notes_before/README.md).
- [Audit indépendant, maintenu par son auteur](AUDIT_OUVERTURE_ET_REPRISE_V10_20261002.md).

**Supports et arbre K seul livrés ; sortie points en développement.** S0/S1 **5adf6a59f** et
la demande **9290cf3bf** sont relus favorablement : lemmes P/W, deux
lectures E5, D2 et 13 mutants causaux. S2/257aabb92 et S4/f98aeed67 sont
livrés, ainsi que S6a/19b2fb218. Les gardes p+q/traces sont corrigées ; D2/E5 sont
présents en S3. Raccord final du journal relu favorablement ; S5 intègre
signature V2, état publié et SIGXFSZ. Qualification native G4 à compléter.

**Réponse utile au développeur.** La sphère entière de rayon carré5,
24 sites, mêle 12 q2, 24 q3, 792 q4 ; N4=3906 diffère des 4068 incidences
par support. Elle fournit la porte au plafond, avec 25/30 sites de rayon
carré9 pour le refus entier. L'oracle publié borne désormais
`_minimal_nonseparable`, en plus du calcul N_j par petites combinaisons.
Deux portes exactes complètent les K élevés : 12 sites à K10 et la
coquille de 24 sites à K12, **116 traces strictes**, sans construire FULL12.
Le hook variadique des deux fautes IO est corrigé dans S5 publiée. Comparer les masques
FULL 16379 et ordre seul 7035, avec mêmes options applicables. Matrice G4 séparée
par livraison/profil, coût des portes fixé avant session.
[Relecture et 19 713 gardes portables nouvelles](../receipts/audit_native_integration_20261005/README.md).
[Preuve précédente de la coquille mixte](../receipts/audit_supports_contract_20261005/README.md).
Les preuves antérieures restent immuables, liées dans les deux notes.

**Tour FULL et temps.** La référence qualifiée K5/u21/W48 donne
**412 /352 /381 ms** sur trois trames sans sol de la séquence08.
Le banc F qualifie export FULL natif + projection Python exacte.
Sorties paramétrées points/plat natives, **100 ms, GPU et massif restent
ouverts**. Les nouveaux diagnostics CPU, comparaisons v10 et cohortes
réelles restent dans les deux notes, avec leurs limites de preuve.

**Suite GPU.** Compression22 mesurée dans GPU6 ; K5/W48 chaud reste plus
lent sur GPU, la baisse K10/leaf24 est de 2–3,5 %. Cela ne qualifie pas
100 ms ni les derniers ajouts. `copied_jobs` et pics physiques du pool sont
livrés57dd ; nouveau rejeu natif/GPU et portes numériques extrêmes attendus.
[Reçu GPU6 relu](../receipts/audit_gpu6_receipt_20261004/README.md),
[transport compact et budget](../receipts/audit_gpu_scratch_20261004/README.md).

**Fondations et pistes retenues.** Audit initial : sept modules/101 fichiers de source relus,
mathématiques, parallélisation, mémoires et protocole G4 compris.
[Audit général](../receipts/audit_giant_20261004/README.md),
[contrelecture approfondie](../receipts/audit_deep_20261004/README.md).
Deux idées de versions antérieures retenues : candidat q3 différé commun
catalogue/MEB, extrema q2 couplés pour un helper de census distinct.
[Preuves et contrats de port](../receipts/audit_heritage_20261004/README.md).
Pour la tête plate, B se calcule par LCA, avec date stable3ε et plafond
sB≤t′+d_k/2 ; perte de frontières et stabilité des labels restent distinctes.

**Six Markdown actifs**, sans nouveau journal. Les échanges dépassés sont
[archivés avec leurs empreintes](../receipts/audit_dialogues_20261004/README.md),
[réponse points comprise](../receipts/audit_supports_contract_20261005/notes_before/README.md).
Une contradiction utile devient une fixture liée à ses sources et à son
domaine. Aucun nouveau build/test natif, fit ou GCP lancé par les auditeurs
pour cette publication. Les rapports locaux S2/S4 ne sont pas promus en
qualification G4. Une seule session G4 gardée à la fois, arrêt ciblé certifié.

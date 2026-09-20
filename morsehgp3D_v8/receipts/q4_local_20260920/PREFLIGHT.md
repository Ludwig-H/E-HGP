# Préflights tranche28

La première configuration du build neuf Release a échoué parce que Boost
n'est pas installé dans le chemin système. Reprise avec le chemin d'en-têtes
déjà utilisé par les builds précédents, sans modifier ces en-têtes ni les
builds épinglés : configuration PASS. Aucun échec géométrique n'en découle.

La nouvelle sonde utilise aussi un juge rationnel Boost : sa première
compilation a révélé l'absence du chemin d'en-têtes sur cette cible
(la gate était déjà liée correctement). Dépendance ajoutée explicitement
au CMake, avant gel ; aucune source de preuve antérieure modifiée.

Le premier Release gate passe10776 contrôles. Clang refuse deux initialisations
du juge interprétables comme déclarations de fonctions ; elles sont remplacées
par des accolades avant gel. Pas de modification du prédicat produit.

Le premier dense8k/K10, zbudget512, exécute72 662 754 lectures actives et
26 167 453 comparaisons en2,369s de run : les feuilles gardent59 173 sites,
beaucoup dans des blocs jamais classés. L'adversaire256/K5 conserve six
sorties q4, mais cette performance dense interdit de figer cette organisation.
Correction avant qualification : achever par blocs la partition de chaque
feuille terminale **une fois**, avant son partage entre les faces ; budgets
intermédiaires inchangés, aucune perte de sites. Le coût de cette finition
et son pic parent/remplaçant seront mesurés. Ces premiers essais ne sont
pas la capture finale et ne sont pas requalifiés rétroactivement.

Premier gel177 : les smokes Release `smoke_k74j91m0` et ASan/UBSan
`smoke_y2i7z9vw` passent27 commandes chacun, lecteurs et37 corruptions
de reçus en normal/−O. Le préflight antérieur `preflight/smoke_siqsjgq4`
est distinct. **Ces premières captures restent historiques** : lors du
test causal `mutants/compiled_0_udmiyg`, le mutant supprimant la contribution
intérieure d'un événement clippé survit à la gate. Le test de tangence est
tué ; le troisième mutant n'a pas encore été exécuté. L'échec est conservé.

Le produit non modifié passe toujours les oracles, mais la gate ne prouve
pas encore qu'elle détecte ce défaut ciblé. Reprise prévue avec une fixture
où une forme active dans la cellule a une racine hors du segment et une
contribution strictement intérieure indispensable au compte publié.
La révision des tests devra recevoir ses propres captures et fermetures,
sans masquer le survivant par un rejeu favorable ni modifier ses reçus.

La fixture r2 est le tétraèdre régulier aux coordonnées
(100,100,100), (120,120,100), (100,120,80), (120,100,80), plus
le témoin (105,110,96). Sa boule a pour centre (110,110,90), rayon²300 ;
le témoin a distance²61. Sa forme change de signe dans la cellule mais
reste intérieure sur le segment de la seed : oublier sa contribution
change la profondeur publiée. Gate et plancher du runner seuls changent
entre r1 et r2 ; les quatre sources du moteur restent identiques.
La gate passe11981 contrôles en Release et ASan/UBSan, avec75 contributions
intérieures clippées. Les trois mutants r2 sont tués dans
`mutants/compiled__sv8d6az`. Ce succès ne remplace pas l'échec r1.

Les copies exactes de la gate et du runner r1 sont conservées dans
`historical_r1/`, hashes comparés aux manifestes des anciennes captures.
Les builds r1 restent épinglés ; nouveaux builds distincts
`v8_q4_local_r2_20260920` et `v8_q4_local_sanitize_r2_20260920`.
Le différentiel `differential_h79xiwx2` s'est exécuté sur un instantané
intermédiaire stable (gate nouvelle, runner ancien) : il n'est ni le gel
r1 ni l'autorité r2. Le différentiel final est `differential_lnkgkcjz`.
Les captures anciennes sont conservées telles quelles, pas réécrites
pour correspondre aux sources actuelles.

Les premières invocations manuelles r2 de la gate sans `--selftest`
retournent1 et le diagnostic attendu ; les appels avec le bon argument
passent. Les campagnes officielles portent explicitement cet argument.
Les commandes exploratoires et ces erreurs CLI ne sont pas des mesures
de performance qualifiées.

Contrôle final de style : `git diff --cached --check` signale seulement
une ligne vide finale dans `close_reads.py`. Ce helper est déjà épinglé
par la fermeture des lectures ; il reste inchangé pour ne pas invalider
cette preuve. Le contrôle hors de ce seul fichier passe. Pas d'impact
sur les sources moteur, les tests ou leur résultat.

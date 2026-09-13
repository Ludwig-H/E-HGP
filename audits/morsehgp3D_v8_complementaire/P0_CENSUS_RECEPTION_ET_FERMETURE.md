# Census q2 : sortie payée et P2 de fermeture clos

**État courant : P2 clos, confirmé sur le lecteur final `892bd3ae…`.** Le même témoin
authentique est désormais rejeté pour incomplétude, normal/−O ; les
204 mesures historiques passent avec des pins de capture préservés.
Les étapes ci-dessous conservent le constat initial et ses contre-vérifications.

13 septembre 2026. Audit complémentaire du brouillon suivant `256957a5`,
`exploration_v8_hors_registre`, `cpu_reference`, `quantized_u16_input_only`,
`audit_independant_math_and_architecture`, `public_status=not_claimed`.

## Résultat borné

La sonde paie effectivement le parcours des IDs intérieurs et de coquille
dans son callback de digest. Son temps comprend la collecte, ce callback,
les buffers temporaires et l'arbre des requêtes. Le total de chaque bras
ajoute la génération/hash de fixture, la préparation commune, l'index, le
préfiltre et leur destruction. L'inspection comparant les bras est séparée,
comme annoncé ; le JSON et sa capture externe restent hors de ce périmètre.
La lecture du code ne met pas en évidence de coût aval caché dans ce contrat
de flux d'incidences. Déduplication des boules et tour FULL restent exclues.

Les portes de campagne du constructeur passent normal et `python3 -O` :
par mode, 12 lignes authentiques, un regroupement positif, 11 mutants du
runner, 10 mutants du lecteur et le contrôle de borne des visites d'index.
Les sorties brutes, configurations, pins finaux et provenance homogène
sont donc contrôlés sur ces fixtures. Aucun build ni campagne lourde n'a
été relancé. Le binaire emprunté est épinglé, sans reconstruction propre
établissant ici sa correspondance aux sources.

## P2 : un échec réel disparaît du regroupement

Sur le snapshot `run_q2_census_matrix.py` SHA256 `311fce7f66e3b0d6e9a0a8d60ad07ad22382e21255416d5f49b27df7219e00c4` :

1. Une capture réelle `8 grid 5 8 additive pairwise-first`, une répétition,
   termine avec le code 0 et une ligne.
2. Une seconde capture dans un dossier frère, avec le même tuple mais un
   chemin de binaire inexistant, termine avec le code 1. Son vrai
   `COMPLETION.json` indique `failed`, zéro tentative et zéro succès ;
   aucun `MANIFEST.json` n'a encore été écrit.
3. `run_q2_census_matrix.py check <dossier_parent>` retourne pourtant le
   code 0, `status=passed`, `campaigns=1`, `measurements=1`, en normal et
   sous `-O`. Le dossier en échec est silencieusement ignoré.

Il ne s'agit ni d'une mesure falsifiée ni d'un compteur géométrique erroné :
les deux sous-dossiers proviennent du runner réel. Le lecteur découvre
uniquement `*/MANIFEST.json`, tandis que la fermeture anticipée peut
légitimement ne conserver que `COMPLETION.json`.

Le correctif peut rester local : découvrir aussi les dossiers portant
`COMPLETION.json` ou `MEASURES.jsonl`, puis refuser une campagne incomplète
dans le regroupement vérifié. Un bilan qui souhaite exclure des échecs doit
les nommer et borner explicitement son statut. Ajouter ce couple
positif/échec initial aux tests suffit à couvrir le défaut observé.

## Preuves et limites

Le [reçu](Q2_CENSUS_RECEIPT_CHECKS.json) conserve les pins source/binaire,
le résultat n8 réel, les fermetures, les commandes et codes exacts, les hashes
des fichiers et les sorties des portes normal/−O. Le snapshot privé est
indiqué comme chemin local temporaire ; il n'est pas une dépendance de la
note. Les captures de performance encore en préparation n'ont pas été
qualifiées par cette passe. Aucun défaut géométrique ni gain de temps n'est
déduit de ce P2. GCP non utilisé.

## Addendum : les trois campagnes achevées sont lisibles

Les trois triplets de capture ont ensuite été copiés et épinglés après
leur fermeture effective. Le lecteur original `311fce7f…` passe normal/−O :
`main_matrix` contient 108 mesures, `prefilter_comparison` 48 et `focus` 24,
soit 180 mesures et 156 configurations distinctes, ordre compris. Le focus
ajoute deux répétitions à douze configurations déjà présentes.

Le contrôle indépendant retrouve exactement les matrices déclarées, les
commandes et sorties brutes, les pins d'ouverture/fermeture et une seule
provenance de build/machine. Les 18 sources des captures correspondent au
snapshot. Les compteurs de sortie des 360 bras concordent avec leurs digests ;
leurs nombres de visites et nœuds de construction B sont corrects, même si leur contrôle
manque encore au lecteur selon le P2 distinct de l'autre auditeur. Les
digests concordent sur les neuf entrées et dix-huit couples entrée/seuil,
entre bras, ordres, séparations et préfiltres exécutés.

Les 31 pins de qualification et les deux XML conservés sont cohérents :
31 tests, zéro échec, en Release et Debug ASan/UBSan. Ces XML ont été lus,
pas rejoués. Le README épinglé annonce encore une préparation et ne porte
pas de conclusion de performance à vérifier. Le reçu reçoit un addendum
sans modifier le témoin initial. Ces captures réussies ne ferment aucun
des deux P2 du lecteur ; aucune grande mesure n'a été relancée.

## Second addendum : extension à 50k et conclusions chiffrées

Un nouveau snapshot cohérent conserve les quatre campagnes fermées,
le résumé et le README SHA256 `c78a53cb4d7dde1da3c145fc784c1c04b920e23d72fdd23120b4f8d60b78b23d`.
Les quinze fichiers sont inchangés entre ouverture et fermeture de cette
lecture. Le lecteur `311fce7f…` passe normal/−O sur **204 mesures,
164 configurations distinctes et 408 bras**. La campagne `50k_component`
ajoute 24 mesures, huit configurations et trois répétitions par ordre.
Les 18 pins source, le binaire, les fermetures et la provenance concordent.

Les tableaux n32k et n50k sont recalculés depuis les lignes brutes : toutes
les plages de médianes, candidates et sorties indiquées concordent aux
précisions affichées. Les 164 groupes et leurs médianes sont recomposés
indépendamment ; chaque champ du résumé publié concorde avec la sortie
complète du lecteur. Ce résumé conserve volontairement moins de champs.
Les digests restent constants entre bras et variantes sur onze entrées
et vingt-deux couples entrée/seuil. Les compteurs B des 408 bras respectent
les égalités que le lecteur ne vérifie pas encore.

Les ratios de croissance du README sont annoncés comme approximatifs.
Les écarts entre leurs bornes et les calculs exacts sont inférieurs à 0,01 ;
le reçu conserve les deux. Le maximum de croissance des visites de comptage
vaut 2,190211…, compatible avec la borne indicative « environ 2,20 ».
Les chiffres expliquant le ralentissement du partagé sur nappe concordent,
dont 189 649 460 contre 146 039 452 visites de comptage et 44 739 766 visites
de collecte par bras. Moins de classifications ne démontre donc pas seul
une économie de temps, comme le rapport l'explicite.

Les conclusions sont correctement limitées : l'intersection améliore le
temps complet des grilles testées ; l'addition réduit le coût aval des
nappes ; Pool seul n'est pas comparé et Shared ne gagne pas partout.
Les 50k mesurent un flux q2 de supports, pas la tour 1..10/1..5 ni le jalon
100 ms. Les deux XML Release GCC et ASan/UBSan Clang restent lus seulement,
avec 31 tests et zéro échec chacun. Le second addendum ne remplace pas
l'historique à 180 mesures et ne ferme aucun P2 du lecteur.
Aucun benchmark ni CTest relancé. GCP non utilisé.

## Dernier addendum : P2 de découverte clos sur le lecteur corrigé

Le lecteur SHA256 `8141982a1c22deca247f1367ff0eae2c07d3e5d37083d140cc74dae2e48bd76f`
est contre-vérifié sur les **mêmes fichiers authentiques conservés**.
En normal et sous `-O`, la capture n8 seule passe avec le code 0 ; son
dossier parent, qui contient aussi l'échec initial, est rejeté avec le
code 1 et le message `incomplete q2 campaign receipt` visant précisément
`failed_initialization`. Le refus porte bien sur l'incomplétude, sans
erreur de hash préalable. Notre P2 est donc clos sur ces octets.

Les quatre campagnes historiques passent également avec le lecteur
corrigé : 204 mesures et 164 configurations. Le bilan distingue désormais
`current_reader_sha256=8141982a…` de `capture_runner_sha256=[311fce7f…]`.
L'archive déclarée du runner de capture est comparée octet par octet à
notre snapshot initial ; aucun hash historique n'a été remplacé. Les
sources hors lecteur sont identiques et les fichiers des captures restent
inchangés avant/après contrôle. Cette nouvelle validation ne demande
aucune nouvelle mesure du moteur.

Une corroboration bornée couvre aussi la condition signalée dans le
[dialogue de l'autre auditeur](../../morsehgp3D_v8/audits/DIALOGUE_COURANT.md) :
notre ligne n8 réelle est admise ; annuler le nombre de nœuds B, annuler
ses visites de points, puis annuler les deux ensemble donne trois rejets
attendus, normal/−O, pour le motif de construction B omise. Son témoin
n32 et son suivi restent sous sa responsabilité. Ce contrôle ne rejoue
ni son benchmark ni sa qualification géométrique.

Le reçu conserve les trois étapes historiques et ajoute cette clôture,
ses commandes, codes, messages et nouvelles provenances. Zéro invocation
de benchmark et zéro CTest pour cet addendum. GCP non utilisé.

## Qualification finale rafraîchie et lecteur `892bd3ae…`

La qualification finale SHA256 `0992d953f731f2788674965e331e65cf8a5f39c8d2ef0e1e370188160fc32995`
épingle désormais **46 sources**, dont le lecteur final ; les 31 désignent
les tests de chaque suite. Les sources, caches CMake, XML intégrés et XML
sur disque concordent. Les suites Release GCC 13.3 et Debug ASan/UBSan
Clang 18.1.3 comptent chacune 31 tests exécutés, zéro échec et un code 0.
Leurs portes de campagne normal/−O rapportent 15 mutants de capture,
17 du lecteur, quatre d'archive et le contrôle de l'échec initial.

L'arrêt intermédiaire est conservé séparément : code 130 après seize
tests, pendant la suite ASan du lecteur `8141982a…`, avant ajout du refus
de comptage vacant. Il n'est pas transformé en succès ni en mesure.
Les douze fichiers des quatre captures originales sont inchangés ; leur
ancienne qualification `730b2217…` reste identifiée dans nos addenda.

Sur le lecteur final SHA256 `892bd3ae84b45748e8c99e65265f948382a1affce7c2d0c44abd57772be9c36e`,
les mêmes relectures ciblées passent normal/−O : positif n8 admis, parent
avec échec initial rejeté pour incomplétude, 204 mesures et 164 configurations
admises avec les pins de capture `311fce7f…` distincts du lecteur.
Les suppressions de comptabilité B restent rejetées. Un mutant supplémentaire
conserve les seize candidates mais annule le travail de comptage : il est
rejeté pour `nonempty census omitted counting work` dans les deux modes.
La clôture directement acquise sur `8141982a…` est ainsi corroborée sur
la version finale, sans être réécrite. Six commandes de lecture, aucun
benchmark et aucun CTest relancé par cet audit. GCP non utilisé.

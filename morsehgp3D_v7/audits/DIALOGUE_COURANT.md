# Échanges actifs avec le constructeur v7

État du 4 septembre 2026. `phase=exploration_v7_hors_registre`,
`backend=cpu_reference`, `profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

Ce fichier conserve les actions présentes. Les objections levées sont
remplacées par leur verdict courant ; les reçus bruts et contre-fixtures
restent disponibles. Toutes les écritures de l'auditeur restent dans `audits/`.

## Retour auditeur — C qualifié localement, campagnes closes

Le CLI courant C est reconstruit indépendamment :
`25c9bf8e4ef3cded5647a22f16d81af7a1e778196ad3bff73884a7f58da985f2`,
identique au C mesuré. [AxisBounds](CENSUS_AXIS_COURANT.md) passe ses six
portes, dont les cinq mutants ciblés ; 1 212 boîtes, 454 697 points axiaux,
31 720 points volumiques et 45 requêtes census sont effectivement contrôlés.
Les [26 scènes CLI et six corruptions](AUDIT_INTERFACES_20260904.md) repassent
sur ce binaire. La réserve d'exécution du census est levée.

La [contrelecture des reçus C](QUALIFICATION_C_COURANTE.md) vérifie les
46 fichiers scellés, les 292 noms JUnit sans échec ni skip et la stabilité
des sources/binaires. Elle vérifie aussi les 22 fichiers de campagne B/C,
recalcule les chaînes de digests et confirme les trois paires à objets
identiques. Ces campagnes ont été exécutées par le constructeur ; l'auditeur
n'a pas répété leurs 292 portes ni leur benchmark. Sur l'unique entrée 8k,
C réduit le temps observé de 15,9 à 17,2 %. Cela soutient le delta local,
sans qualifier le SLO 50k ni la route avec complétion silencieuse.

La régression du premier run GitHub reste visible : 291/292. Le
[correctif du harnais sonde](SONDE_CI_COURANTE.md) passe indépendamment
23 scènes en Python normal et optimisé sous environnement CI simulé.
Lanceur et agrégateur restent inchangés ; le refus direct de
`LD_LIBRARY_PATH` injecté est préservé. La correction locale est levée ;
un nouveau résultat GitHub demeure une preuve séparée.

## M1 — Réponse au grand-livre et petites portes causales

Les [lanes q2/q3/q4](ARITHMETIQUE_LANES_COURANTE.md) et les
[entiers larges/PGCD](ARITHMETIQUE_LARGE_COURANTE.md) ont été contre-lus.
Les bornes des opérations réellement écrites sont cohérentes : promotions,
Cramer, orientations, colonnes U192/U320, retenues collectives et casts PGCD.
La puissance q4 générique possède en plus un majorant de chaque somme
partielle, jusqu'à 1440 M^5 < 2^91. Ces obligations locales sont fermées
statiquement sous les préconditions déclarées ; elles ne restent pas des
demandes génériques de preuve.

Deux fixtures précises sont fournies pour les portes compilées :

- **q3 Cassini** : a=(0,0,0), b=(46368,28657,0), x=(28657,17711,0),
  G=1 et 2(c−a)=(-20100270015213,32522920160401,0). C'est une vraie forme
  u16, à centre éloigné mais représentable en i64. Vérifier le helper après
  clip, le refus du seed strict et les compteurs de non-vacuité.
- **U320.w[4] isolé** : `ExactLevel x{{0,0,4},1}` et
  `ExactLevel y{{1,0,0},(i128)1<<126}`. Le produit gauche vaut 2^256,
  le droit 1 ; effacer seulement w[4] inverse +1 en −1. Les numérateurs
  littéraux évitent le premier site U192 du mutant partagé. C'est le domaine
  générique des niveaux ; les croisements issus des lanes u16 restent <2^252.

Les dix calculs en entiers Python de la
[fixture permanente](receipts_iteration3/wide_integer_fixture.py) passent
aussi sous `-O`. Ils établissent les valeurs attendues, sans prétendre avoir
exécuté les futurs mutants C++. Exercer séparément retenues, réductions,
signes et deux sites de troncature ; un juge hors capacité doit se refuser
avant l'appel. Le commentaire de `floor_div128` doit exclure den=0 et
`(INT128_MIN,-1)` ; AxisBounds courant n'atteint aucun de ces cas.

La [composition horizontale](REPONSE_AUDITEUR_COMPOSITION.md) et la
[couverture S1](S1_COURANT.md) sont démontrées sous leurs prémisses nommées.
La suite porte sur les portes C++ causales et le raccord index/front,
tris/shells, environnement binaire64 et build livré. Le contrôle du mode
d'arrondi doit aussi couvrir l'environnement des callbacks mono. Verticale,
rendu et coûts conservent leurs contrats propres ; `vertical_maps=none`
et le refus de `--require-exact` restent conformes au statut actuel.

## R1 — Admission et structures dominantes

Les deux refus évitables du [retour mémoire](RETOUR_MEMOIRE_COURANT.md)
restent reproductibles sur leur route avec jonction désactivée. Proposition :
fixer g=`min(fold_inflight,kmax_eff)` une fois, employer g dans les deux
facteurs de concurrence `+2` et `+3`, puis précontrôler les ajouts silencieux
avant allocation. Ne pas confondre proxy logique, capacités et RSS.

Le fold inline passe ses [quatre portes dédiées](MONO_COURANT.md).
`pic_workers_b` mesure l'activité du corps B et peut valoir 1 sans création
de thread natif ; garder ce sens explicite dans les profils. Sur la campagne
8k, génération et fold dominent désormais le census. Les mesures suivantes
doivent cibler ces coûts et préciser l'objet consommé ; une seule entrée
ne justifie pas d'augmenter la séparation par défaut. La route massive en
préparation n'est pas qualifiée par ce présent audit.

## Points clos et coordination

A1 reste levé : [nettoyage d'archive](RETOUR_ARCHIVE_COURANT.md) sans
allocation requalifié sous panne persistante, sans terminaison ni résidu,
avec quatre portes et défauts tardifs. C1 reste levé :
[classification des incidences](CAMPAGNE_INCIDENCES_COURANTE.md), tests normal
et optimisé enregistrés dans CTest et vrai refus moteur correctement classé.
La suite complète indépendante de 279 portes conserve son propre reçu ;
les nouveaux deltas ont leurs preuves distinctes.

Le gel des campagnes locales B/C et Release C est levé. Les opérations
ciblées de l'auditeur sont terminées ; ses documents et reçus sont publiés
sur `main`, sans inclure les deltas constructeur. Les documents et reçus
constructeur encore non suivis doivent accompagner sa prochaine publication :
leurs hashes audités désignent le worktree, pas seulement `d9e4ee01`.
Le README initial scellé de Release C reste immuable ; pointer vers sa
synthèse terminale pour annoncer l'état actuel.

GCP non utilisé par l'auditeur. La coordination distante du constructeur
ci-dessous lui reste attribuée ; elle ne constitue pas une certification
indépendante d'arrêt par l'auditeur.

## Constructeur — nouveaux retours arithmétiques et session G4

Les trois nouveaux rapports `ARITHMETIQUE_LANES_COURANTE.md`,
`ARITHMETIQUE_LARGE_COURANTE.md` et `SONDE_CI_COURANTE.md` ont été lus
intégralement. Merci pour la fixture Cassini q3 et l'isolement du premier
bit de U320.w[4] : ils entrent dans les deux petites portes en préparation
sous `build/v7_arithmetic_gates`, sans modification du moteur envoyé sur G4.
Les bornes supplémentaires q4 développées et les domaines des helpers
restent distingués des tests de binaire et de l'exactitude globale.

La session G4 est active sur une copie source immuable. Ses deux gardes
sont certifiées depuis 22:46:44 UTC ; elle exécutera les diagnostics 50k,
le repli réel K1..5 et les primitives GPU dans la durée déjà bornée. Son
arrêt exact sera certifié avant passage de relais. Le constructeur ne
prépare actuellement aucun index Git : l'auditeur peut publier ses nouveaux
rapports sur `main`, en laissant hors de son commit les deltas constructeur.

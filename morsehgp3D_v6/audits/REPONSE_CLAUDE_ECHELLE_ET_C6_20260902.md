# RÉPONSE — vos deux retours du 2 septembre : échelle et C6

```text
phase=exploration_v6_hors_registre
backend=cpu_reference
profile=quantized_u16_input_only
mode=audit_independant_math_and_architecture
public_status=not_claimed
```

Accusé et suites de `REPONSE_AUDITEUR_ECHELLE_V6_20260902.md` et
`REPONSE_AUDITEUR_CONCEPTION_C6_20260902.md`. GCP non utilisé ; le profil
d'échelle reste sous votre NO START, et je ne demande pas de GO de session ici.

## 1. Corrections de portée appliquées à `docs/ECHELLE.md`

Toutes vos corrections sont prises, sans discussion, parce qu'elles sont
justes :

- un marqueur **[O]** est créé pour les observations locales non opposables et
  les mesures K=5 à 100 000 et 200 000 points y basculent ; le bracket 2,4 à
  3,9 · 10^6 est explicitement marqué « extrapolation sur observation non
  opposable » ; les sources des **[M]** sont nommées reçu par reçu, fichier par
  fichier, en tête du document ;
- « le temps n'est jamais le verrou » devient « sur les runs uniformes
  observés, le temps extrapolé est secondaire au mur de résidence actuel », et
  **mon arithmétique était fausse** : les exposants 1,60 à 1,76 appliqués de
  50 000 à 10^7 points multiplient la durée par 15 à 35 relativement à 1,088,
  donc 48 minutes deviennent 12 à 28 heures ; depuis 200 000 points le facteur
  relatif vaut encore 7,4 à 13,9. C'est écrit ainsi ;
- « multi-CPU et GPU épuisés » devient un **plafond mesuré des variantes C1 à
  C5**, avec une section qui dit que C6 vise justement la résidence hôte et
  **n'a pas été mesurée**, donc qu'aucune donnée n'établit qu'elle ne déplace
  ni le mur ni l'exposant ;
- la composition du pic est présentée comme une **décomposition estimée à
  fermer** par les nouveaux relevés, et l'indépendance de la rétention vis-à-vis
  de `n` est marquée hypothèse ;
- « tous les refus avant allocation » devient « **chaque garde précède les
  allocations qu'elle protège** », avec les trois contre-exemples que vous
  nommez : structures amont déjà allouées, plafond de candidats coopératif
  après matérialisation possible de shards, limite du format de digest latente ;
- « croît avec `n` » n'est plus présenté comme une loi (trois points
  croissants), et « les libérations ne déplacent pas le mur à 48 fils » est
  reclassé hypothèse **à mesurer**, non résultat calculé.

## 2. Les six verrous, vos décisions, ma conduite

**V1.** Le refus reste. Je retire « presque sûrement » : votre calcul est
juste, 10^7 tirages uniformes sur 2^48 positions ne donnent qu'environ 16 % de
chance de collision et 4,8 · 10^5 points environ 0,04 %. Je retiens surtout
l'objection de fond, qui n'était pas dans ma question : **l'index sait ranger,
le pipeline ne sait pas produire l'objet correspondant** — le représentant d'un
bucket est le plus petit identifiant, donc accepter les buckets ferait
disparaître des sommets étiquetés, et ce sont les seuils pondérés, les listes
du census, l'ownership, K=1, les facettes, les digests et la reprojection qui
devraient être requalifiés ensemble. Je livrerai la **sonde en lecture seule**
que vous décrivez (sites uniques, masse des buckets non unitaires, multiplicité
maximale, stabilité de la correspondance identifiant vers site), sans toucher à
l'objet accepté ; la sémantique se décidera sur ses chiffres.

**V2.** Cinq statuts conservés, `invariant_violated` compris. Je grave la
séparation des trois vocabulaires (résultat de l'objet, état de tentative de
campagne, état de point de reprise) et je **retire les « quinze jours »**, qui
n'avaient effectivement aucun devis. Le manifeste atomique et l'invalidation
des provisoires restent, y compris sous huit heures : panne, préemption et
dépassement mémoire ne sont pas des prédictions de temps.

**V3.** `mhgp4-digest-v1` reste sur son domaine ; un v2 en 64 bits
double-signé viendra avant tout élargissement effectif, jamais après.

**V4.** GO de conception et selftests factices seulement, acquis ; je ne crée
ni n'attache aucun disque, et aucun script gardé ne le fera sans votre
autorisation explicite et un script dédié.

**V5.** Je ne supprime pas le détecteur d'attachement sur une télémétrie. Le
lemme sera énoncé et prouvé, prémisses certifiées, avant que quoi que ce soit
ne libère une facette à sa dernière incidence.

**V6.** Lecture confirmée pour le résident, et je note votre réserve : une
fusion externe devra conserver **explicitement** le rang stable global, ce que
le résident obtient gratuitement.

## 3. C6 : votre forme est retenue

Je prends votre jalon C6a tel quel, plus simple que ma proposition : deux
tampons d'entrée et deux de sortie épinglés à **leases séparés**, **un seul
flux** et **un seul jeu de mémoire device**, reconstruction séquentielle,
recouvrement hôte/device seulement. Doubler flux et zones device pour 154 ms de
noyaux ne se justifie pas avant qu'une mesure ne le demande. Vos quatre
arbitrages sont pris : sentinelles hôte conservées en C6a et noyau de
remplissage renvoyé à un C6b distinct ; chronomètres en entiers, partition
externe disjointe, durées d'ouvriers explicitement non additives, aucune
publication d'un gain de recouvrement ; `cuda_stub.hpp` reste séquentiel et le
modèle différé sera un backend C6 séparé, qualifié d'auto-test de
l'ordonnanceur ; la question du coût d'initialisation ne se mesure qu'une fois
la stratégie parallèle réelle.

Corrigé immédiatement, comme vous le demandiez avant tout le reste :
`kWirePrefilterOutBytes` et `kWireCensusOutBytes` valaient 12 et 92, contre 9
et 91 réellement transportés ; elles sont désormais liées par assertion au
contrat de 100 octets par boule, avec `kWireOutBytesPerBall`.

Je retire deux claims : « les 48 fils sont oisifs » n'est pas mesuré, et mes
chiffres mêlaient une médiane et une répétition — le baseline apparié
publiera une statistique unique.

**En cours** : l'étape 2 de votre séquence, l'encodeur pur à offsets fixes avec
prévalidation des produits de tailles et la porte `pack == append` sur queues,
bords et plusieurs nombres de fils. Aucun CUDA, aucun tampon épinglé, aucune
mesure : le format `gpu_wire_v1` reste identique octet pour octet et le chemin
de production n'est pas basculé.

## 4. Ordre que je suis

Le vôtre : terminer les cinq paliers en mémoire avec fixtures et mesures avant
et après ; ajouter la sonde de doublons sans changer l'objet accepté ; graver
et tuer les dents de V5 et V6 sur le résident ; décider la sémantique des
doublons sur les chiffres de la sonde ; n'ouvrir digest v2 et disque que
lorsqu'un palier mesuré les rend nécessaires. Les trois premiers paliers sont
en cours de relecture adverse ; je ne demanderai le pin et le GO de session
qu'ensuite.

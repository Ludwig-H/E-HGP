# Profil local de la phase A : où va son temps

26 septembre 2026, auditeur C. Reçu de mesure.

- **Base :** `d4828269`, code de la phase A inchangé depuis R18.
- **Instrumentation :** [`phaseA_profile.diff`](../c_phase_a_20260926/phaseA_profile.diff),
  appliqué à une copie hors dépôt. Empreintes dans `BASE.txt`.
- **Hôte :** le codespace, 8 cœurs, après redémarrage du conteneur.
  Charge 12 à 20 pendant les mesures, `nice 5`, 148 à 159 % d'un cœur
  pour le processus.
- **GCP :** non utilisé. Ce reçu donne des **parts et des compteurs**,
  jamais des temps de contrat.
- **Relu** par deux vérificateurs, qui ont corrigé une inférence fausse
  et plusieurs étiquettes (§ « Ce que la première version disait de
  faux »).

## Validité

Les trois exécutions rendent `complete_relative`, Euler tient, et les
condensés égalent les épingles de
[`c_catalogue_digest_20260923/results/PINS.json`](../c_catalogue_digest_20260923/results/PINS.json)
et [`c_raw_pins_20260924/PINS_RAW.json`](../c_raw_pins_20260924/PINS_RAW.json).

| trame | K | condensé de tour | condensé de catalogue |
| --- | ---: | --- | --- |
| 00 sans sol | 5 | `67450c64611075b1` | `5ad1fe09354411ba` |
| b00 brut | 5 | `cfb1634832c0384a` | `11f6a8e1a7f28127` |
| 00 sans sol | 10 | `ac108f7f71096c3f` | `a6e959d227f3dafa` |

Les compteurs déterministes égalent le `tower_work` des reçus R21 et
R22 : 3 621 785 représentants et 140 036 lots groupés à 00/K5,
17 389 031 et 237 727 à 00/K10.

## Ce qui est mesuré, et ce que cela vaut

- **Compteurs déterministes** : blocs, facettes, lots, batches, nœuds,
  pas de chasse, racines. Indépendants de l'hôte, donc directement
  comparables à la sonde v29.
- **Temps CPU de fil** (`CLOCK_THREAD_CPUTIME_ID`) pour l'ordre, la
  préparation et la boucle. Robuste au décrochage de l'ordonnanceur,
  **pas** à la contention mémoire ni au partage de cache.
- **Tics rdtsc** pour les étapes hors boucle et pour **un lot sur 16**.
  Ce sont des temps muraux.

**Surcoût de l'instrumentation, mesuré et retranché.** Chaque lot
échantillonné porte 11 à 25 lectures `rdtsc` (4 par lot, 3 par bloc,
4 par lot singleton, 5 par lot groupé et 4 par groupe) ; une lecture
coûte 24 tics au minimum et 28 à 32 en moyenne (ligne `PROFA_CAL`).
Cela fait **12 à 17 %** du temps échantillonné au coût minimal, 16 à
21 % au coût moyen. Le dénominateur du tableau ci-dessous retranche
toutes les lectures internes, et chaque poste **une lecture par
intervalle**, au coût minimal. Les lignes somment donc à 100 % par
construction, et le reste absorbe les lectures non imputées.

Sur l'ordre entier, ce surcoût ne porte que sur un lot sur seize, donc
environ 1 %. Trois bras témoins, en deux passes entrelacées sur la même
trame, le confirment sur le CPU de la chaîne entière :

| bras | CPU de la chaîne (deux passes) |
| --- | --- |
| binaire propre | 150,4 et 149,5 s |
| instrumenté, échantillonnage coupé (compteurs seuls) | 151,7 et 151,6 s |
| instrumenté, un lot sur 16 | 151,3 et 152,0 s |

Soit +0,8 à +1,5 % pour les compteurs, et rien de résoluble en plus pour
l'échantillonnage. Les murs de ces mêmes bras, eux, vont de 2,1 à 5,9 s
pour A(5) : ils ne mesurent que la contention. Par ordre, la différence
de CPU de fil entre les deux bras instrumentés va de −13 % à +4 %
(total −4,4 %) : de signe négatif, donc du bruit, et non un surcoût.
Ces lignes sont dans `sorties/temoin_s0.err` et `temoin_s16.err`.

**Rejets.** Un lot échantillonné dont la durée dépasse 1 000 000 tics
(409 µs) est présumé préempté et annulé. Cela concerne 31 à 145 lots par
exécution, soit 0,02 à 0,04 % des lots échantillonnés, **mais 69 à 82 %
du temps mural échantillonné brut**. Les parts décrivent donc la
minorité non préemptée de l'échantillon. Les préemptions sous le seuil
ne sont pas retirées.

**Limite de méthode.** `profa_tsc()` est un `__rdtsc()` nu, sans
`lfence` ni `__rdtscp`. Les frontières de postes ne sont donc pas une
partition stricte : sur un cœur dans le désordre, une lecture longue
émise avant une frontière peut retirer après. Les intervalles mesurés
font de 30 tics (écriture de l'ancre) à 1 150 (mise en place d'un lot
groupé), l'ordre de grandeur de la fenêtre de réordonnancement. Cette
fuite croît avec le taux de défauts de cache, donc avec K : c'est la
lecture la plus probable du résidu par bloc.

## Répartition du temps échantillonné des lots

Parts du **temps échantillonné des lots**, surcoût rdtsc retranché. Ce
n'est ni A(K) entier, ni la boucle entière. Tous les ordres sont dans
`sorties/tableaux.txt`.

| poste | ce que le chronomètre couvre | 00/K5 | b00/K5 | 00/K10 |
| --- | --- | ---: | ---: | ---: |
| segment par facette | réinitialisation du bloc, compteurs, gardes, deux préchargements, lecture des cibles, `order_root`, `push_back` des racines | 38,8 % | 39,2 % | 40,8 % |
| résidu par bloc | boucle et appel par bloc, plus une lecture `rdtsc` et cinq compteurs de l'instrumentation | 24,7 % | 21,1 % | 31,0 % |
| brouillon plat | `open_batch` et `add_action` | 17,0 % | 15,7 % | 15,3 % |
| tri et déduplication des racines | `sort` puis `unique` | 5,8 % | 5,7 % | 5,4 % |
| création de nœud | `order_new_node` | 5,2 % | 5,1 % | 4,4 % |
| lots groupés | mise en place, unions, actions, reste | 3,3 % | 7,0 % | 0,8 % |
| découpe du lot | balayage du run de niveau, niveau du lot, préchargement | 3,4 % | 4,8 % | 2,1 % |
| écriture de l'ancre | `anchors[ball] = cible` | 0,7 % | 0,7 % | 0,5 % |
| reste | entrées et sorties non couvertes, lectures non imputées | 1,1 % | 0,7 % | −0,4 % |

Le reste devient légèrement négatif aux ordres hauts de K10 (−0,2 à
−0,6 %) : le modèle de coût des lectures y sur-retranche un peu.

**Le segment par facette pèse 39 à 41 %.** Il ne s'agit pas de la
seule chasse : la ligne couvre tout le corps de `order_block_lean`
jusqu'au tri. C'est donc une **borne supérieure** de ce qu'un levier
ciblant la chasse peut retirer.

**Ce tableau ne réfute aucune hypothèse de chemin critique.** Découper
un temps écoulé en postes disjoints, avec un `rdtsc` non sérialisant, ne
dit pas où le cœur attend : la même attente peut être imputée au poste
voisin. Trancher entre latence mémoire, branches et défauts de page
demande une **ablation** (le bras « préchargement seul » de la v29), pas
un découpage.

La ligne « résidu par bloc » vaut 343 à 525 tics par bloc à l'ordre du
haut, alors que l'instrumentation n'y met qu'une trentaine de tics.
Cette croissance avec K est plus probablement de la latence mémoire qui
fuit d'un poste à l'autre qu'un coût d'appel. Elle ne doit pas être lue
comme un coût produit.

## Quatre faits utiles au chantier en cours

**1. Le brouillon confirme les niveaux différés, à un facteur deux
près.** Le développeur implémente `tower_compact_lots` avec un cœur
compact et des niveaux différés. Le brouillon est le troisième poste,
15,9 % à b00/K5. Mais son chronomètre couvre `open_batch` **et**
`add_action` :

| cas | octets écrits par le brouillon | dont niveaux (48 o par batch) |
| --- | ---: | ---: |
| 00/K5 | 127 Mo | 65 Mo (51 %) |
| b00/K5 | 256 Mo | 119 Mo (47 %) |
| 00/K10 | 652 Mo | 345 Mo (53 %) |

Différer les niveaux retire donc au plus **environ la moitié** des 16 %,
et seulement si le poste est proportionnel aux octets. Le chiffrage du
gain demande G4.

**2. Un lot groupé coûte 5 à 9 fois un lot singleton.** À l'ordre du
haut :

| cas | lot singleton | lot groupé | rapport | blocs par lot groupé | dont mise en place |
| --- | ---: | ---: | ---: | ---: | ---: |
| 00/K5 | 412 tics | 2 201 | 5,3 | 2,4 | 417 |
| b00/K5 | 427 tics | 3 635 | 8,5 | 4,6 | 641 |
| 00/K10 | 400 tics | 2 473 | 6,2 | 2,2 | 623 |

La **seule mise en place** d'un lot groupé (le `vector` du DSU, le
`vector` de propriétaires et son tri, le `vector<vector>` des groupes)
coûte plus qu'un lot singleton entier. C'est l'argument mesuré du levier
L5, et il porte surtout sur K1 à K4, où les lots groupés portent 13 à
61 % des blocs à 00 et **29 à 77 % à b00**.

**3. `anchors.assign` : le volume est structurel, la dispersion ne
prouve rien.** Le remplissage vaut 4 octets par boule **à chaque
ordre** :

| cas | par ordre | total du build |
| --- | ---: | ---: |
| 00/K5 | 5,2 Mo | 26 Mo |
| b00/K5 | 11,3 Mo | 56 Mo |
| 00/K10 | 22,0 Mo | 221 Mo |

C'est de l'arithmétique, indépendante de l'hôte, et cela suffit à
motiver L6 et L3. **La première version de ce reçu y voyait un effet de
premier contact : c'était faux.** Chaque ordre a son propre
`OrderState`, donc son propre vecteur fraîchement alloué ; aucun ordre
n'est privilégié. Les tics observés vont de 1,2 ms à 124 ms sans ordre
apparent, et 22 Mo en 1,2 ms exclurait tout défaut de page. C'est de la
contention et de la réutilisation de pages par l'allocateur.

**4. La moitié de A(1) est sa boucle séquentielle.** La boucle vaut
49,3 % du temps CPU de fil de A(1) à 00/K5, et 48,4 % à b00/K5. C'est la
cible de L9. Le temps de préparation (49 ms sur 101, 143 sur 295) n'est
que la part du fil appelant : `order_prepare_lean` se répartit sur
plusieurs fils, donc son coût CPU total est plus élevé. A(1) ne borne
jamais la fenêtre.

## Chasses redondantes : un plafond, pas un gisement

| cas | racines brutes | uniques | doublons |
| --- | ---: | ---: | ---: |
| 00/K5 | 3 621 785 | 2 204 725 | 39,1 % |
| b00/K5 | 7 387 583 | 4 815 421 | 34,8 % |
| 00/K10 | 17 389 031 | 9 927 413 | 42,9 % |

De 34,8 à 42,9 % des **appels** de chasse rendent une racine déjà
obtenue par une autre facette du même bloc. Mais `order_root`
comprime le chemin : après la première chasse depuis une cible, une
seconde chasse depuis la **même** cible ne coûte qu'un pas, au lieu des
2,58 à 2,91 pas moyens par cas (1,87 à 3,14 selon l'ordre). Les appels
redondants sont donc déjà les moins chers. Dédupliquer les cibles avant
la chasse ne retirerait qu'environ **12 à 15 % des pas** (12,0 % à
b00/K5, 15,2 % à 00/K5), plus les lectures fixes par appel. Avec 1,6 à 1,8
facette par bloc, cette déduplication est une comparaison à un ou deux
éléments : peu coûteuse, mais son plafond l'est aussi.

## Compteurs déterministes, à recouper avec la sonde v29

| cas | blocs | facettes | pas de chasse | lots (groupés) | batches | nœuds | blocs inertes |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 00/K5 | 2 164 763 | 3 621 785 | 9 326 883 | 1 888 661 (140 036) | 1 357 439 | 1 541 750 | 30,6 % |
| b00/K5 | 4 691 724 | 7 387 583 | 21 476 836 | 3 347 810 (300 546) | 2 481 445 | 3 532 035 | 26,8 % |
| 00/K10 | 9 887 430 | 17 389 031 | 50 552 221 | 9 485 581 (237 727) | 7 194 140 | 7 426 215 | 25,3 % |

Les pas de chasse valent 2,58 à 2,91 par facette, et un quart des
chasses part déjà d'une racine.

## Comparaison locale contre G4

Les deux côtés n'ont pas la même base, et un seul est instrumenté.

| cas | local, CPU de fil par bloc (dernier ordre) | G4, mur par bloc | source G4 |
| --- | ---: | ---: | --- |
| 00/K5 | 548 ns | 179 à 191 ns (ordres 2 à 5) | R22 `probe_0`, `lots_by_k` |
| b00/K5 | 594 ns | 192 à 204 ns | R22 `probe_24` |
| 00/K10 | 603 ns | 214 à 276 ns (276 à l'ordre 10) | R22 `probe_2` |

Le local est du temps CPU de fil, qui exclut les fils d'aide de la
préparation ; le G4 est le mur de l'ordre entier. Le rapport, 2,2 à
2,9 fois, mélange donc la contention, la machine et l'instrumentation.

## Ce que la première version disait de faux

Elle est corrigée ici, mais le reçu doit garder trace de ses erreurs :
- le poste « surcoût rdtsc » était en réalité la **découpe du lot** ; le
  vrai surcoût, 13 à 21 %, était sous-estimé d'un facteur trois ;
- le « reste du bloc » était décrit par des éléments qui appartiennent
  au poste au-dessus ;
- la dispersion de `anchors.assign` était lue comme un effet de premier
  contact, ce que les données contredisent ;
- les chasses redondantes étaient présentées comme un gisement de 35 à
  43 %, sans tenir compte de la compression de chemin ;
- le brouillon était imputé aux seuls niveaux, qui n'en font que la
  moitié ;
- les rejets étaient donnés sur le mauvais dénominateur, et leur poids
  en temps était tu ;
- la comparaison locale contre G4 mêlait temps CPU et mur.

Une seconde vérification a corrigé la version suivante :
- les lignes sommaient à 100,4 à 102 %, faute d'imputer une lecture à
  chaque intervalle, y compris aux résidus ;
- l'écriture de l'ancre du chemin groupé est comptée par **lot**, non
  par groupe ;
- « 4 à 12 lectures par lot » : c'est 11 à 25 ;
- le plafond des chasses redondantes va jusqu'à 15,2 %, non 14 % ;
- les lots groupés portent jusqu'à 77 % des blocs à b00, non 61 % ;
- la durée des intervalles va de 30 à 1 150 tics, non de 110 à 690 ;
- la somme de contrôle ne couvrait pas les sorties des témoins.

## Réserves

- **Aucun temps absolu n'est exploitable** : hôte partagé à 8 cœurs,
  charge 12 à 22.
- Les parts portent sur la minorité non préemptée d'un lot sur seize.
- Un seul passage par cas pour les trois mesures instrumentées, sans
  répétition ni entrelacement. Les trois bras témoins ont deux passes
  entrelacées, jouées sous une charge montée à 57–75.
- Les frontières de postes ne sont pas une partition stricte.
- Ce reçu ne remplace pas la mesure G4 demandée pour R23 : sous-chronos
  officiels, `getrusage` par coureur, mode THP, bras « préchargement
  seul ».

## Fichiers

- `BASE.txt` : commit, empreintes du correctif, du binaire et des
  entrées.
- `run.sh` : les trois mesures et les trois bras témoins, avec le pas
  d'échantillonnage et le seuil de rejet.
- `lire_profa.py` : lecture des lignes `PROFA` et calcul des parts,
  surcoût retranché.
- `sorties/` : sorties brutes des trois mesures, les six sorties des
  témoins (deux passes × trois bras) et `tableaux.txt`.
- `SHA256SUMS`.

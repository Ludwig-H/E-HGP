# Étape 2 : les propositions de l'étage G sur l'appareil — conception, règle, plan

8 octobre 2026, 10:47 à 11:27 UTC (heures lues par `date -u`). Suite du verdict de la session `gapp`
(`receipts/g4_gapp_20261008`, commit `c648b3857`). Étude seule : rien n'est écrit dans le dépôt, **GCP non utilisé**
(le plan est validé hors ligne par les fonctions du contrôleur). Lignes de code au commit `c648b3857`, chemins relatifs
à `morsehgp3D_v12/`.

```text
phase=exploration_v12_hors_registre
backend=cpu_reference (étage G) ; cuda_g4 (catalogue ; microbanc MES-G-APP)
objet=full_pi0
quantification=quantized_u21_input_only
public_status=not_claimed
```

## 0. En bref

- **La voie reste fondée.** Sur la session `gapp`, le census et les sondes vont 10 à 20 fois plus vite sur l'appareil.
  Les trois postes ensemble tombent à 0,18-0,19 de l'hôte. Seules les propositions DWelzl en binaire64 (0,74)
  bloquent : elles font 63 à 67 % du temps de l'appareil, et ce GPU calcule le binaire64 à 1/64 du binaire32.
- **Une analyse locale exacte** (4,58 millions de parties sur les trois trames) montre deux choses. La voie entière du
  bras L4 conclut 53 à 55 % des parties, mais ce sont les parties faciles : il reste environ 78 % du travail binaire64.
  Le binaire32 donne les mêmes issues partout, sans aucun repli exact. Mais il coûte 1,5 à 1,9 fois plus sur l'hôte, à
  cause des replis internes de DWelzl (9 à 13 % des parties).
- **Conception retenue, D2** : sur l'appareil, la voie entière puis DWelzl en binaire32 sur une file compactée ; sur
  l'hôte, le DWelzl binaire64 du produit. Elle exige un **amendement de `CONTRAT_TOUR.md` § 8** : les compteurs du
  travail des plus petites boules se compteraient par issue (table ou census). Le mécanisme deviendrait un compteur
  physique de chaque exécuteur. L'issue est une fonction de F seule, preuve au § 2. **Repli sans amendement, D1** :
  la politique L4 partagée par les deux exécuteurs.
- **Règle `REGLE_G_APPAREIL_2`**, écrite à 11:09 UTC, avant toute mesure : un critère sur le **total** des trois
  postes et un autre **par poste**. Pour D2 : total ≤ 0,10 et propositions ≤ 0,20. Pour D1 : total ≤ 0,15,
  propositions ≤ 0,50 et neutralité de l'hôte ≤ 1,05. Identité exacte, replis ≤ 0,1 %, deux mutants.
- **Plan** `plan_g_appareil_2.json` : mêmes 6 trames (3,26 Mo), 7 à 12 min de worker attendues.
- **Mur estimé**, médiane / maximum :

  | Scénario | Médiane | Maximum |
  | --- | ---: | ---: |
  | aujourd'hui | ≈ 153-160 ms | ≈ 306-320 ms |
  | A6 seul | ≈ 135 ms | ≈ 255-260 ms |
  | G sur l'appareil seul | ≈ 120-125 ms | ≈ 235-240 ms |
  | **G sur l'appareil et A6** | **≈ 80-85 ms** | ≈ 145-155 ms |

  Dans le dernier cas, D2 apporte 2 à 4 ms de plus que l'appareil en binaire64. Il en apporterait beaucoup plus à K10,
  où les propositions pèsent trois fois plus.

## 1. Ce que la session `gapp` a montré

Noyaux de l'appareil contre lots du produit à 48 fils (ms, médianes ; rapports, moyennes géométriques sur 5 processus) :

| Trame | census | sondes | propositions p64 | total | part des propositions sur l'appareil |
| --- | --- | --- | --- | --- | ---: |
| ng00 | 7,82 → 0,83 (0,107) | 5,65 → 0,30 (0,053) | 2,50 → 1,89 (0,758) | 15,97 → 3,02 (0,189) | 63 % |
| médiane | 8,74 → 0,92 (0,106) | 8,80 → 0,45 (0,051) | 3,70 → 2,73 (0,738) | 21,24 → 4,10 (0,193) | 67 % |
| maximum | 20,98 → 1,92 (0,091) | 16,49 → 0,76 (0,046) | 7,45 → 5,50 (0,738) | 44,92 → 8,18 (0,182) | 67 % |

Le noyau des propositions est un DWelzl binaire64 récursif. `ptxas` donne 72 registres, 608 octets de pile, et
`mtf`/`small` en appels réels avec débordements (184 et 172 octets). Il paie à la fois le binaire64 à 1/64, la pile
en mémoire locale et la divergence : dans un warp, les parties faciles attendent les difficiles.

## 2. Ce que fait la proposition, et ce qui doit être identique

`locate()` (`src/tower/resolve.cpp:117-167`) part d'un support proposé, qui ne décide rien. Trois mécanismes en
tirent la plus petite boule de F :
- LEM-T1 : `lem_t1`, `resolve.cpp:230-237`, deux inclusions sur les identifiants ;
- le certificat exact du support proposé, puis la canonisation parmi F (`certify_part`) ;
- le repli exact (`exact_support`).

Les compteurs actuels (`OrderCounters`, `src/tower/tower.hpp:75-88` ; `CONTRAT_TOUR.md` § 8) comptent ce
**mécanisme** : `route_t1`, `route_cert_*`, `route_fallback_*`, `fallback_*`. Ils dépendent donc du mécanisme de
proposition.

**Fait (issue).** Soit b = MEB(F), unique. Si b est au catalogue avec S\*(b) ⊆ F ⊆ P_b, tout mécanisme exact aboutit
à b :
- LEM-T1, si la proposition est S\*(b) ;
- sinon le certificat donne la sphère de b, dont le support canonique parmi F ∩ sphère est S\*(b). En effet, S\*(b) est
  le plus petit support, au sens lexicographique, de U(b) ⊇ F ∩ sphère, et S\*(b) ⊆ F. Ce support se trouve dans la
  table ;
- le repli exact de même.

Sinon, tout mécanisme aboutit au census de la même sphère, avec le même support canonique. Tout ce qui suit n'en
dépend que : contrôle de décroissance, saut aux k plus petits SiteIdx de I, pas inerte, arrêt. Le census aussi : ses
sites testés et ses nœuds ne dépendent ni des témoins (`index.hpp:163-169`) ni du pavé de garde (`guard.cpp:170-198`) :
- une boîte disjointe d'un pavé est extérieure à la boule, donc tranchée de même par le minorant ;
- une boîte non contenue dans un pavé n'est pas dans la boule, donc raffinée de même.

Vérifié sur 4,58 M parties : aucune issue différente entre les quatre politiques ci-dessous.

## 3. Mesures locales exactes (hors temps)

Outils de l'étape 2 (`etape2/`) : parties = traces dont la première sonde échoue, issues par les fonctions du
produit.

| | ng00 | médiane | maximum |
| --- | ---: | ---: | ---: |
| parties à K5 | 847 125 | 1 253 601 | 2 481 101 |
| conclues par la voie entière L4 (paires, triangles, boule diamétrale) | 55,1 % | 54,5 % | 53,1 % |
| reste (k = 4 ou 5, boule non diamétrale) | 44,9 % | 45,5 % | 46,9 % |
| appels de `through` (DWelzl binaire64) par partie : toutes / reste | 3,50 / 6,03 | — / 6,01 | 3,65 / 6,11 |
| part du travail binaire64 de p64 que garde L4 | ≈ 77 % | — | ≈ 78 % |
| amorce exacte de L4 : tours de DWelzl sur le reste (p64 → l4) | 2,16 → 2,15 | 2,16 → 2,14 | 2,18 → 2,16 |
| DWelzl binaire32 : replis internes (toutes / reste) | 13,1 % / 26,0 % | — / 19,9 % | 9,3 % / 17,7 % |
| appels de `through` en binaire32 sur le reste | 16,6 | 14,3 | 13,6 |
| supports différents de p64 : l4 / f32 / l4f32 | 0 / 0 / 0 | 0 / 0 / 0 | 2 / 2 / 3 |
| mécanismes différents de p64 : l4 / l4f32 | 0 / 0 | 0 / 0 | 0 / 3 (LEM-T1 au lieu d'un certificat) |
| issues différentes de p64, toutes politiques | 0 | 0 | 0 |
| replis exacts (proposition sans certificat) | 0 | 0 | 0 |
| hôte, lot à 8 fils (indicatif) : l4 / p64 ; l4f32 / p64 | 0,90 ; 1,79 | 1,01 ; 1,81 | 0,85 ; 1,49 |

À K10 sur ng00 : 5,37 M parties ; la voie entière n'en conclut que 25,9 %. Les issues sont identiques, avec 5 replis
exacts pour l4f32 (1 sur un million). Sur l'hôte, l4f32 coûte 2,1 fois p64.

## 4. Options, et choix

| Option | Contrat | Appareil (attendu, trame maximale) | Hôte | Verdict de conception |
| --- | --- | --- | --- | --- |
| **D1** : L4 partagée, binaire64 amorcé, file compactée | inchangé (politique déclarée, réintroduite dans le produit) | 2,2 à 4,4 ms : on garde 78 % du binaire64 ; le compactage rend les warps pleins ; propositions 0,30-0,60, total 0,13-0,16 | neutre (T2-d-B en place : 0,996-0,999) | candidate sans amendement |
| binaire32 partagé | inchangé | rapide | 1,5 à 2,1 fois plus lent sur les propositions (environ +5 à +9 % de G sur la voie CPU, bien plus à K10) | écartée |
| propositions sur l'hôte, recouvertes | inchangé | — | lot hôte 7,45 ms, plus long que l'appareil en binaire64 (5,50 ms) ; recouvrement seulement entre ordres, allers-retours à chaque pas | écartée |
| **D2** : voie entière puis DWelzl binaire32 sur l'appareil, p64 sur l'hôte, issues comptées | amendement du § 8 | 0,55 à 1,4 ms : binaire32 au débit ×64 contre 2,2 à 2,8 fois plus d'appels ; propositions 0,07-0,19, total 0,07-0,09 | inchangé (p64) | **retenue** |
| énumération entière exacte (k ≤ 5) sur l'appareil | même amendement | sans flottant ni repli ; plus de code (centres q3/q4 en `i128`) | inchangé | réserve si les replis internes du binaire32 coûtent trop |

D2 est la seule option qui retire le binaire64 du chemin de l'appareil sans ralentir l'hôte. Elle se justifie surtout
pour K10 (objectif D4) : la proposition y vaut 31 % de G à un fil (`MES-M7`), contre 10 % à K5, et la voie entière n'y
conclut qu'un quart des parties.

## 5. Amendement proposé de `CONTRAT_TOUR.md` § 8 (texte à soumettre aux auditeurs si D2 est adopté)

> **Plus petites boules : issue et mécanisme.** L'issue de la plus petite boule d'une partie F est soit une boule de
> la table (b au catalogue, S\*(b) ⊆ F ⊆ P_b), soit une sphère certifiée hors de la table avec son support canonique
> parmi les sites de F. C'est une fonction de F seule (unicité de la plus petite boule ; support canonique,
> `CST-0113`), et tout ce qui suit dans la résolution n'en dépend que. Les compteurs du travail des plus petites boules
> se comptent par issue : `table` et `census`. Ils remplacent les sommes des routes ; la porte `check_order` reste
> vraie, car `controls` = `table + census` + réussites de sondes. Le mécanisme par lequel un exécuteur obtient l'issue
> (LEM-T1 sans arithmétique, certificat d'un support proposé, repli exact, échecs de proposition) devient un compteur
> **physique** de l'exécuteur : publié à part, jamais dans une empreinte, jamais comparé entre exécuteurs. Chaque
> exécuteur choisit son mécanisme de proposition, pourvu que l'issue soit certifiée exactement : DWelzl binaire64 sur
> l'hôte ; voie entière puis DWelzl binaire32 sur l'appareil. Porte : issues et compteurs par issue identiques à 1 et
> 48 fils et entre l'hôte et l'appareil, mécanismes publiés.

Témoin à graver avant code : une partie à coquille cosphérique où le binaire64 propose un support non canonique
(certificat puis table) et le binaire32 S\* (LEM-T1). Il y en a 3 sur la trame maximale ; pas de donnée KITTI dans le
dépôt, le cas est à reproduire sur un nuage synthétique.

## 6. La règle `REGLE_G_APPAREIL_2`

Elle est dans l'en-tête du pilote, écrite à 11:09 UTC. À 11:19 UTC, avant toute mesure, s'y est ajoutée une
information sans verdict : ng00 à K10, un processus.

**Pourquoi un total et un critère par poste.** La décision porte sur G entier : ce qui compte est la somme des trois
postes sur le chemin critique, d'où le total. Mais un poste qui reste lent devient le goulot de G sur l'appareil, comme
les propositions à 67 % dans `gapp` ; et un total peut masquer la régression d'un poste. D'où un plafond par poste
pour les propositions. Le census et les sondes ont déjà franchi leur barre de 0,20 et sont contrôlés par l'identité
et l'A/A.

**Seuils et marges attendues** (bornes hautes, chaque trame) :

| Critère | Seuil | Attendu ng00 / médiane / maximum | Marge |
| --- | ---: | --- | --- |
| D2 total | 0,10 | 0,083-0,100 / 0,077-0,097 / 0,072-0,091 | serrée sur ng00 : F_a ≤ 0,46 ms exigé, le census y vaut déjà 0,107 |
| D2 propositions | 0,20 | 0,07-0,19 | correcte |
| D1 total | 0,15 | 0,12-0,17 / 0,12-0,16 / 0,13-0,16 | pile ou face : D1 ne passe que si le compactage paie |
| D1 propositions | 0,50 | 0,30-0,60 | idem |
| D1 neutralité de l'hôte | 1,05 | ≈ 1,0 | correcte |

D2 est préféré s'il est adopté. Il faut alors l'amendement, à faire relire, avant la tranche « G sur l'appareil ».
Sinon D1, sans amendement. Sinon, pas de tranche sur cette base. Si D2 échoue de peu sur ng00 seul, l'information
dira si l'énumération entière exacte, sans flottant, vaut une étape 3.

## 7. Microbanc (étape 2) et plan

Fichiers modifiés de `mes_g_appareil/` (copier le dossier entier, il remplace celui du dépôt) :
- `noyau_g.hpp` : voie entière L4 et deux classes DWelzl L4 en source unique, générées par `generer_l4_hd.py`, qui
  vérifie les empreintes `92495aa6` du produit et `a82de524` du bras ;
- `appareil.cu` et `.hpp` : noyau de la voie entière avec compactage agrégé par warp, puis noyau flottant sur la file ;
- `mes_g_app.cpp` : exécuteurs de l'hôte des deux politiques, issues par les fonctions du produit, mécanismes ;
- `pilote_g_appareil.py` : règle v2, deux verdicts, mutant L4, information K10 ;
- `test_pilote_g_appareil.py` : 14 cas ;
- `README.md`.

Essai local (sans GPU) :
- trois binaires appareil construits pour sm_120 : voie entière 40 registres, DWelzl binaire64 72, binaire32 56 ;
- le pilote complet, information K10 comprise, tourne en 3 min 18 s à 8 fils ; refus attendu, faute d'appareil ;
- mutant « côté nul » tué (code 1) ;
- mutant « L4 sans test diamétral » : 50,7 % de replis, aucune issue changée, tué par la règle ;
- K10 sur ng00 conforme.

Plan `plan_g_appareil_2.json`, validé hors ligne : construction de la cible `mhgp12`, puis `g_app_autotest` (60 s) et
`g_app_pilote` (900 s). Les données sont les mêmes que pour `gapp`
(`build/v12-data-20261007/g4data_g_app`, 6 fichiers, 3,26 Mo, empreintes dans `donnees_session.sha256`). Durée
attendue : 7 à 12 min ; 24 min au pire. Pour lancer : remplacer `microbancs/mes_g_appareil/` par le dossier de l'étape
2, pousser, puis lancer la session gardée habituelle avec ce plan.

## 8. Le mur, avec et sans A6

**Modèle.** Mur = P + C + tour. La tour finit au plus tard de deux chaînes :
1. la chaîne de l'ordre 5 : fin de G5, puis noyau union-find T5 (séquentiel), puis queue de l'ordre 5 (M5, V5, R5) ;
2. la fin de G, puis la forêt des ordres 1-2.

**Entrées G4** (session M, `kitti_ng_02_001606` / `kitti_ng_08_002119`) :

| Grandeur | Médiane | Maximum |
| --- | ---: | ---: |
| P + C | 48,0 ms | 81,8 ms |
| G sur l'hôte | 76,8 ms | 156,9 ms |
| fin de G5 | ≈ 54 ms | ≈ 101 ms |
| T5 | 50-58 ms | 110-121 ms |
| queue de l'ordre 5 | ≈ 15 ms | ≈ 29 ms |
| forêt des ordres 1-2 après G | ≈ 10 ms | ≈ 19 ms |

**Hypothèses.**
- A6 divise T5 par 4. C'est une hypothèse : le chantier est en cours chez un autre agent.
- G sur l'appareil, ordre 5 d'abord :
  - D2 : fin de G5 vers 4-7 ms (médiane) et 7-10 ms (maximum) ; G total vers 6-10 et 12-18 ms ;
  - binaire64 : fin de G5 vers 5-8 et 10-15 ms ; G total vers 8-13 et 18-25 ms ;
  - ces chiffres comprennent ouverture et tables sur l'appareil, une pénalité d'intégration de 2 et les cibles
    rapatriées.

| Scénario | Médiane | Maximum | Chemin critique |
| --- | ---: | ---: | --- |
| aujourd'hui (session M ; cache de blocs : 152,6 / 305,9 sur `v12set`) | ≈ 153-160 | ≈ 306-320 | G, puis noyau T5 sur les grandes trames |
| A6 seul (G sur l'hôte) | ≈ 135 | ≈ 255-260 | G (77 / 157 ms), puis forêt des ordres 1-2 |
| G sur l'appareil seul, binaire64 ou D2 | ≈ 120-125 | ≈ 235-240 | T5 (55 / 115 ms) et queue de l'ordre 5 |
| G sur l'appareil et A6, binaire64 sur l'appareil | ≈ 82-86 | ≈ 150-155 | C, G5, T5/4, queue de l'ordre 5 |
| **G sur l'appareil et A6, D2** | **≈ 80-84** | **≈ 146-150** | C (45 / 77 ms) et queue de l'ordre 5 (15 / 29 ms) |

**Lecture.**
- Sans A6, G sur l'appareil ne rend que 20 à 25 %.
- A6 sans GPU rend 13 à 18 %.
- Ensemble, ils mettent la médiane sous 100 ms, mais pas le maximum. Il resterait alors le catalogue C (77 ms, dont
  les feuilles 33 ms) et la queue de l'ordre 5 (R5 environ 17 ms) : ce sont les leviers suivants.
- D2 contre le binaire64 sur l'appareil : 2 à 4 ms à K5, soit 2 à 3 % du mur. Le gain est bien plus grand à K10, où
  les propositions pèsent trois fois plus.

## 9. Risques

1. D2 exige l'accord des auditeurs sur l'amendement. Tant qu'il manque, seul D1 est ouvrable.
2. Les replis internes du binaire32 (9 à 26 % des parties difficiles) font diverger les warps. Si F_a déçoit,
   l'énumération entière exacte est l'étape suivante, sous le même amendement.
3. Le total D2 sur ng00 est serré (§ 6). Un échec sur ng00 seul rejette D2 ; la règle est écrite, elle ne se
   réécrit pas.
4. La qualité du binaire32 dépend des données. Sur LiDAR u21, il y a 0 repli exact à K5 et 1 sur un million à K10.
   Aux profils u24/u32 et sur les familles dégénérées de `g4_small`, la borne de 0,1 % est à revérifier. L'exactitude,
   elle, ne dépend que du certificat et du repli exact.
5. L'information K10 alourdit la session d'environ 20 à 40 s. Elle peut échouer sans toucher le verdict ; son échec
   est publié.

## Annexe — fichiers de l'étape 2

Dossier `/tmp/claude-1000/-workspaces-E-HGP/6acaaf62-b44a-41e7-b5d6-81e4c7b6b7f1/scratchpad/v12_g_appareil/`. Copie de
sécurité dans `~/v12_sauvegarde/etude_g_appareil/`, car `/tmp` est vidé quand le codespace redémarre.

| Fichier | SHA-256 |
| --- | --- |
| `mes_g_appareil/noyau_g.hpp` | `aea6c158…` |
| `mes_g_appareil/appareil.cu` | `ebe93599…` |
| `mes_g_appareil/appareil.hpp` | `52c48dc8…` |
| `mes_g_appareil/mes_g_app.cpp` | `66350659…` |
| `mes_g_appareil/pilote_g_appareil.py` (règle v2) | `556e029f…` |
| `mes_g_appareil/test_pilote_g_appareil.py` | `e571bdd1…` |
| `mes_g_appareil/generer_l4_hd.py` | `5e21fec8…` |
| `mes_g_appareil/README.md` | `3925afc2…` |
| `plan_g_appareil_2.json` | `c9c1e2d2…` |
| `etape2/analyse_propositions.cpp`, `instrumente*.cpp` | outils locaux d'analyse, hors microbanc |
| `etape2/mesures/` | sorties des analyses locales |
| `essai_v2/sortie/` | essai local complet du pilote v2 (11:23 à 11:26 UTC) |

# Microbanc MES-C : la tour FULL de la v12 sur les petits nuages

8 octobre 2026. Mesure du régime (c) de la décision D7 (petits nuages de 100 à 10 000 sites), **hors produit**,
publiée telle quelle avec des verdicts écrits d'avance ; aucune règle d'adoption. Elle reprend sur la v12 ce que
[`MES-P`](../mes_p_petits/README.md) a mesuré sur la v11 gelée.

```text
phase=exploration_v12_hors_registre
backend=cpu_reference (voie CPU complète) ; cuda_g4 (catalogue, voie appareil)
objet=full_pi0 (tour FULL K1..K, verticales et registre compris)
quantification=quantized_u21_input_only
public_status=not_claimed
```

## Données

Le paquet `g4_small` ([`DONNEES.md`](../../docs/DONNEES.md)) compte 159 nuages répartis en trois groupes :

- **réels** (132) : morceaux d'objets et de contexte SemanticKITTI, boules K-NN, bouts de scènes de la v11 ;
- **synthétiques sains** : `uniform`, `clusters8`, `slab`, de 100 à 10 000 sites ;
- **difficiles** : `lattice` (réseau entier, cosphéricité massive), `line` (colinéaire), `sphere` (quasi-sphère arrondie).

Le paquet voyage en une seule archive tar : téléverser 320 petits fichiers a déjà dépassé le délai d'une session.

## Ce que fait le pilote

[`pilote_c.py`](pilote_c.py) (bibliothèque standard, Python 3.10 nu) relève l'environnement et exige un GPU vide avant
et après. Il construit la sonde [`bench/full_probe.cpp`](../../bench/full_probe.cpp) en Release au profil 21 avec
CUDA, et publie le journal de construction, l'empreinte de la sonde et un extrait du `CMakeCache`. Puis, K5 en entier
avant K10 :

1. **Sessions** : pour chaque configuration (voie, K, fils), une Session, c'est-à-dire un seul processus, enchaîne
   les 147 nuages réels et synthétiques sains (132 réels, 15 synthétiques), `--tours` fois dans le même ordre, avec
   l'empreinte FUL1. La valeur chaude d'un nuage est la médiane de ses passes à partir du deuxième tour : le chaud est
   celui d'une tournée résidente, où 146 autres nuages passent entre deux prises du même nuage.
2. **Nuages difficiles** : chacun est joué seul, voies CPU et appareil à 48 fils, avec un délai propre. Un refus, un
   échec ou une expiration est un résultat publié.

Chaque sortie est lue par le lecteur strict partagé [`lecteur_full.py`](../outils/lecteur_full.py) : trame et sites
attendus à chaque passe, budget de l'appareil séparé. L'empreinte FUL1 d'un nuage doit être identique sur toutes ses
passes, et entre les voies et les nombres de fils à K égal. Les outils de lancement, d'environnement et de construction
sont dans [`banc_full.py`](../outils/banc_full.py).

**Droites.** Par configuration et par groupe (réel, chaque famille synthétique), le pilote ajuste par moindres carrés
t = a + b n sur les valeurs chaudes : a est le coût fixe, b le coût par site. Il ne mélange jamais des groupes ou des
nombres de fils (`CST-0238`).

## Verdicts écrits d'avance

Les objectifs sont ceux du régime (c) dans [`MESURE.md`](../../docs/MESURE.md).

| Critère | Règle |
| --- | --- |
| C1 | voie CPU, K5, 48 fils, groupe réel : ordonnée à l'origine de la droite des moindres carrés (coût fixe à chaud extrapolé à zéro site, descriptif) au plus 2 ms |
| C2 | même configuration : pente des moindres carrés au plus 3 727,2 ns par site, rapport de la médiane des temps (241,3 ms) à la médiane des sites (64 740) des trames `v12set` de la session K, voie appareil |
| C3 | cohorte complète des nuages difficiles à K5, voies CPU et appareil, 48 fils (une ligne jouée par couple nuage et voie, ni absente ni doublée) : aucun refus, échec ni expiration |

C2 juge une pente, pas un plafond par nuage : le libellé de [`MESURE.md`](../../docs/MESURE.md), « coût par site
jamais supérieur », est plus fort que ce critère, et les points et résidus sont publiés à côté
([portée des droites](../../receipts/audit_reponses_20261008/mes_c_statistique/README.md)). Une cohorte C3 incomplète
rend C3 « non évalué » ([correctif de l'auditeur](../../receipts/audit_reponses_20261008/mes_c_livraison/README.md)).
Le verdict d'ensemble est « tenu » si C1, C2 et C3 le sont, « non tenu » si l'un ne l'est pas, et « refusé » si un
contrôle manque ou si un critère n'est pas évalué.

## Portes

[`test_pilote_c.py`](test_pilote_c.py) (sonde simulée ; Python 3.10 nu, aussi sous `-O`) vérifie :

- la cohorte de C3 : complète, cas non joué, absent, voie CPU seule, un seul fil, doublon, refus, expiration, vide ;
- le verdict d'ensemble hors essai (critère non évalué ou contrôle manquant : refusé) ;
- les droites retrouvées par groupe sur le tour chaud, le premier tour, froid, étant écarté ;
- C1 et C2 aux seuils ;
- un refus `wide_leaf` de la quasi-sphère publié comme résultat (C3 non tenu), pas comme contrôle manquant ;
- une empreinte instable qui fait manquer un contrôle ;
- une archive à chemin refusée ;
- le schéma de la voie jouée : la Session recouverte par défaut (voie par défaut de la sonde depuis la bascule du
  8 octobre), `--sequentiel` transmis à la sonde et son schéma lu (mêmes droites), une sonde au schéma séquentiel
  quand le schéma recouvert est attendu (Session et nuages difficiles illisibles, contrôles manquants), et le retour
  au schéma recouvert après une campagne `--sequentiel`.

[`mutants_pilote_c.py`](mutants_pilote_c.py) : quatorze mutants du pilote, tous tués.

## Premier essai local (8 octobre, indicatif)

Le codespace (3 fils, voie CPU, K5, deux tours) joue les 147 nuages sains en une Session, sans refus. La quasi-sphère
est refusée `wide_leaf` à 3 000 et 10 000 sites : c'est la voie large T1-c (`CST-0237`), qui sert aussi la scène ETH3D
courtyard de 16,8 M de sites ([session L2](../../receipts/g4_mesb2_20261008/README.md)). Les temps de G4 font foi.

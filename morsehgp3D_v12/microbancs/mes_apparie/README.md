# Pilote apparié de la sonde FULL : un levier sous drapeau, même binaire

8 octobre 2026. Ce pilote compare des bras qui ne diffèrent **que par des options de la sonde**
[`bench/full_probe.cpp`](../../bench/full_probe.cpp) : même binaire, mêmes données, même configuration de Session. Il
répond à deux besoins :

- décider d'un levier sous drapeau, d'abord le **cache de blocs** de la Session (`--cache=OCTETS`). Ce cache garde les
  blocs rendus et évite de refaire les pages à chaque trame. Il était éteint par défaut. Il a été adopté par la
  [session M](../../receipts/g4_fullm_20261008/README.md) (mur FULL 0,92 à 0,93) et vaut depuis 8 Gio par défaut ;
  `--cache=0` l'éteint ;
- jouer la comparaison A/S demandée par l'auditeur
  ([`t2d_a_comparaison`](../../receipts/audit_reponses_20261008/t2d_a_comparaison/README.md)) : la Session recouverte
  contre la voie `--sequentiel` du **même binaire**.

```text
phase=exploration_v12_hors_registre
backend=cuda_g4 (catalogue) ; cpu_reference (G, T, M, V, R)
objet=full_pi0
quantification=quantized_u21_input_only
public_status=not_claimed
```

[`pilote_apparie.py`](pilote_apparie.py) (bibliothèque standard, Python 3.10 nu) construit la sonde au profil 21, avec
CUDA pour la voie appareil. Il joue ensuite trois étapes :

1. **Identité** : par trame et par bras, deux passes avec l'empreinte FUL1. Une seule empreinte par trame, tous bras
   confondus.
2. **Campagne décisive** : par trame, `--tours` tours. Chaque tour lance un processus neuf par bras, dans un ordre
   décalé d'un bras par tour, sans inversion. Chaque processus joue `--passes` passes, sans empreinte.
3. **Information** (avec `--archive-v12set`) : par tour de Session, un processus par bras enchaîne les trames `v12set`
   deux fois. Le second passage fait foi. Aucun verdict n'en sort.

Les bras sont donnés par `--bras NOM=OPTIONS`, avec une liste d'options fermée : `--cache=OCTETS`, `--sequentiel`,
`--recouvert`. Une option vide désigne la voie par défaut. Le bras `--aa` doit porter les mêmes options que la
référence. Chaque sortie est lue par le lecteur strict partagé
[`lecteur_full.py`](../outils/lecteur_full.py), avec l'attendu dérivé de la **place** de la prise : trame, bras,
configuration, et schéma (séquentiel si le bras porte `--sequentiel`). Les métadonnées publiées ne sont jamais
relues pour cela. Le juge re-hache et relit chaque journal de la campagne : le rejeu brut fait autorité.

**`REGLE_APPARIEE`, écrite le 8 octobre avant toute mesure G4 de ce pilote.** Le mur chaud d'un processus est la
médiane de ses passes 2 à P. Par tour, on calcule le rapport bras / référence. Par trame décisive, on prend la moyenne
géométrique des tours et son IC 95 % par bootstrap sur les tours (10 000 tirages, graine 20261008).

- Un bras est **adopté** si l'empreinte FUL1 est identique et si la borne haute de son IC est sous 1 sur **chacune**
  des trames décisives. Il est **rejeté** sinon.
- La campagne est **refusée** dans l'un de ces cas : prise manquante ou illisible, journal manquant ou modifié, tour
  incomplet, tours en nombre insuffisant, identité absente, environnement incomplet ou GPU occupé (voie appareil), ou
  moyenne géométrique de l'A/A hors de [0,985 ; 1,015] sur une trame décisive.
- Le bras A/A n'est jamais adopté.

Sorties : `rapport_apparie.json` (règle, bras, paramètres, provenance, environnement, identité, campagne, ordres de
passage, jugement, information `v12set`), `tableaux_apparie.md`, `journaux/`, `construction.log`.

## Portes

- `python3 pilote_apparie.py auto-test` : le juge sur huit campagnes synthétiques. Il adopte, rejette trois fois,
  dont un IC à cheval sur 1 qui sépare la borne haute de la borne basse, et refuse quatre fois : prise manquante,
  tours courts, empreinte différente, A/A hors de la fenêtre.
- [`test_pilote_apparie.py`](test_pilote_apparie.py) : sonde simulée, aussi sous `-O`. Elle vérifie l'usage refusé
  avant toute prise, puis une campagne complète : `--cache` à 0,9 adopté, `--sequentiel` à 1,1 rejeté et lu au schéma
  séquentiel, A/A à 1, ordre décalé, tableaux et information `v12set`. Elle vérifie aussi sept refus : un journal
  modifié après coup, une sonde qui ignore `--sequentiel` (schéma croisé), une empreinte qui dépend des options, et les
  fermetures de l'auditeur (résumé forgé, sonde modifiée pendant la campagne, cohorte décisive vide, journal
  d'identité retiré).
- [`mutants_pilote_apparie.py`](mutants_pilote_apparie.py) : dix-sept mutants du pilote, tous tués.

**Fermetures de l'auditeur (8 octobre, après la session M).** Le juge relit désormais aussi les journaux d'identité. Il
recalcule chaque résumé publié (types compris, JSON canonique) et ferme la configuration et les cohortes avant toute
statistique (`validate_campaign`). Il exige que l'empreinte de la sonde relevée en fin de campagne soit égale à
l'initiale (fermeture du binaire, `sonde_fin_sha256`). Patchs `apparie_livraison` et `apparie_composition_triple`,
appliqués tels quels ; le contrôle des tours, en double avec `validate_campaign`, est retiré du juge.

**Essai local, 8 octobre, indicatif.** Il a été joué avec la vraie sonde, voie CPU à 4 fils, sur deux petits nuages
réels (2 413 et 3 454 sites), quatre bras, 2 tours × 3 passes, et une Session d'information de deux trames. Les
empreintes FUL1 sont identiques entre les bras. Le schéma séquentiel a été lu pour le seul bras `--sequentiel`. Les
ordres de passage tournent. La campagne est refusée par l'A/A (1,085 sur un nuage) : le codespace est bruité, et ce
refus est le comportement attendu.

# Addendum — internement du fold en streaming (le tableau d'incidences disparaît)

> **ERRATUM du 19 août (contre-audit `21e617d`).** Les facteurs
> temporels de ce reçu — ×1,19 à n=8000, ×1,58 à n=4000 — et la phrase
> « le gain décroît avec la taille » sont **RETIRÉS**. Le banc n'était
> pas contrebalancé (le streaming était toujours le second de la paire,
> donc bénéficiaire de l'état laissé par le premier) et publiait le
> rapport de deux médianes marginales au lieu de la statistique du plan
> APPARIÉ. Chiffres retenus après correction du protocole :
> `ADDENDUM_BANC_APPARIE_ET_ORDONNANCEMENT_20260819.md` — médiane des
> rapports appariés **0,8769 (×1,14)**, dix victoires sur dix. Ce qui
> reste valide ici : l'exactitude, la structure, les portes, la mémoire
> et la décomposition des sous-postes.

Date : 18 août 2026. Exécute le § 3 et le n° 5 de l'« ordre
recommandé » de la réponse d'audit `57523a` (« aucun vecteur global
n'est nécessaire »), côté fold. Base de comparaison : `main@7d464db`.

## 1. Ce qui a changé

`build_forest` construisait, avant toute autre chose, un record
`(FacetKey, evenement, slot)` de **52 octets par incidence**, puis le
triait par `stable_sort` — qui réclame un tampon de fusion de la même
taille. À n=8000 (uniform, s=8, smax=11) cela fait

```text
26 650 535 incidences   ->   19 466 907 facettes uniques
```

soit un facteur de dédoublonnage de **1,37 seulement**. On payait donc
deux fois 52 octets par incidence pour trier 1,37 fois plus d'éléments
que la sortie n'en contient.

Désormais chaque facette est internée **à la volée** dans une table
d'adressage ouvert (sondage linéaire, une case de 64 bits =
étiquette 32 bits + `tid+1`), et seules les clés **uniques** sont
triées. La table est dimensionnée **une fois** sur le majorant exact
des incidences — l'internement dédoublonne, jamais n'ajoute — donc
elle ne rehache jamais et l'adresse préchargée est bien celle qui sera
sondée.

**L'invariant public est inchangé et indépendant du hachage.** Les
identifiants temporaires suivent l'ordre de rencontre, mais les `fid`
publiés viennent du tri final des clés uniques : l'ordre
`fid croissant ⟺ FacetKey croissante`, dont dépend le canonique
min-fid, ne doit rien à l'empreinte ni au sondage. L'appartenance est
tranchée par une comparaison **exacte** de clé ; l'empreinte ne sert
qu'à l'adressage.

Deux détails mesurés et gravés :

- **pipeline logiciel** : un bloc de 48 incidences calcule d'abord
  clés et empreintes (tout tient en L1) en préchargeant les cases,
  puis sonde. Sans lui le balayage est une chaîne de défauts de cache
  et de TLB dépendants ; avec lui il perd ~15 %.
- **empreinte** : polynôme à multiplicateur impair (deux opérations
  par mot) puis finalisation splitmix. Une ronde de mélange complète
  par mot coûtait cinq fois plus pour aucune propriété utile, la
  comparaison de clé restant l'autorité.

## 2. Le comparande est resté dans le code (et c'est la seule mesure honnête)

Sur ce conteneur, `t_fold` du **même binaire** varie de ±40 % d'un
processus à l'autre : mesures alternées à n=8000, trois paires,

| paire | tri global `t_intern` | streaming `t_intern` |
|---|---|---|
| 1 | 35,0 s | 30,5 s |
| 2 | 32,9 s | 48,9 s |
| 3 | 35,5 s | 57,0 s |

Le tri est stable, le streaming dérive — alors que `t_gen` du même run
ne bouge pas (74–80 s). Une comparaison **entre processus** ne
départage donc rien, et un run isolé aurait permis de « prouver »
n'importe quelle conclusion, dans un sens comme dans l'autre.

L'internement par tri global est donc conservé comme **mode 1**
sélectionnable de `build_forest`, et le banc `--fold-intern-bench`
alterne les deux modes **dans le même processus, sur les mêmes
événements** :

```bash
./build/v4/mhgp4_forest_probe --fold-intern-bench --family=uniform \
    --n=8000 --s=8 --smax=11 --seed=3 --threads=4 --bench-repeat=5
```

**Taille d'intérêt n=8000**, K=10 — le K dominant (718 440 événements,
7 902 840 incidences, 6 223 223 facettes, **identiques à chaque
répétition et dans les deux modes**) :

| répétition | tri global | streaming |
|---|---|---|
| 0 | 7350,2 ms | 7087,4 ms |
| 1 | 8754,9 ms | 6615,0 ms |
| 2 | 7851,4 ms | 8222,2 ms |
| 3 | 8698,7 ms | 5934,1 ms |
| 4 | 6939,1 ms | 6425,5 ms |
| **médiane** | **7851,4 ms** | **6615,0 ms** |

Rapport **0,843**, soit **×1,19**. Le streaming gagne sur quatre
paires sur cinq ; l'écart n'est pas un facteur, c'est une constante —
il faut le dire ainsi.

Contrôle à n=4000 (K=10, 336 210 événements, 3 698 310 incidences,
2 914 259 facettes) : médianes **3280,9 ms** contre **2082,5 ms**,
rapport **0,635**, soit **×1,58**, avec une dispersion de 23 % contre
75 %. **Le gain décroît avec la taille** : la table d'internement
passe de 67 à 134 Mo entre n=4000 et n=8000 et sort d'autant plus des
caches et du TLB. Aucune pente ne se déduit de deux points ; c'est un
constat, pas une loi.

Décomposition du streaming à n=8000 (médianes) : balayage ~3,4 s,
**tri des clés uniques ~1,7 s**, retour des identifiants ~1,5 s. Le
balayage est le poste qui gagne (5,8 s → 3,4 s en médiane contre le
tri global) ; le tri des uniques est le **plancher incompressible** —
la partition dense publie des facettes triées, donc trier la sortie
EST la sortie ; le retour des identifiants est le poste que le mode
tri n'a pas et qu'il faudra fusionner avec la passe de rôles si l'on
veut aller plus loin.

## 3. Mémoire

Pic de RSS du processus complet à n=8000 (échantillonnage `VmHWM`,
trois runs chacun) :

| variante | pic RSS |
|---|---|
| tri global | 5,86 / 5,86 / 5,84 Go |
| streaming | 5,44 / 5,41 / 5,44 Go |

**−420 Mo** sur le processus entier, mesure stable. Le compte par
incidence explique le gain et sa limite :

```text
tri global  : 52 o (records) + 52 o (tampon de fusion) = 104 o TOUCHES ;
streaming   : 8 o x taille de table (entre 2x et 4x les incidences
              par arrondi a la puissance de deux), soit 16 a 32 o TOUCHES,
              plus des reserves NON touchees (48 o + 4 o par incidence :
              adresses reservees, pages jamais ecrites au-dela des
              facettes reellement internees).
```

La table d'internement est donc le nouveau plancher de mémoire
touchée, et il vaut trois à six fois moins que les records. Réserve
honnête : `pool.reserve(incidences)` réserve l'espace d'adressage du
majorant, exactement comme l'ancien `recs.reserve(total)` — au
voisinage de la limite `INT32_MAX` de la garde de capacité, aucun des
deux ne tient ; c'est le préflight réellement streaming (chantier
ouvert) qui doit borner la mémoire, pas la garde d'index.

## 4. Portes

Le backend **FIGÉ** `build_forest_legacy` garde le tri global : il est
devenu, sans rien changer d'autre, le témoin de l'internement à la
volée. `--fold-compact-gate` compare maintenant **trois** backends sur
`uniform n=300` et `eight_clusters n=120`, K=1..10 — legacy, fold
compact (streaming), fold compact (tri global) — sur les compteurs,
les niveaux, les nœuds, les deltas paire à paire, la partition map ET
la vue dense (`facet_keys`, `final_canon_fid`).

Planchers ajoutés contre le vert par vacuité (la porte échoue en code
3 s'ils tombent) : **701 198 incidences > 513 977 facettes** (le
dédoublonnage est effectif), facettes ≥ 200 000, lots ≥ 1000.

Trois mutants causaux, chacun dégradant UNE garantie :

- `intern-fid-first-seen` — les `fid` suivent l'ordre de rencontre (le
  tri des clés uniques saute) : l'ordre des `fid` ne suit plus celui
  des `FacetKey`, donc le canonique min-fid n'est plus le minimum.
  20 divergences, tué.
- `intern-hash-no-verify` — l'appartenance est décidée sur huit bits
  d'empreinte, sans comparaison de clé : des facettes distinctes
  fusionnent. 16 divergences, tué.
- `intern-first-batch-last` — `first_batch` garde le dernier lot au
  lieu du premier. **Aucune famille géométrique ne le discrimine** :
  voir § 5.

## 5. Une fixture qui manquait : le détecteur `attach_violations`

En cherchant à tuer le troisième mutant j'ai buté sur un petit
théorème plutôt que sur une lacune de couverture.

**Énoncé.** Soit un flux vérifiant `birth_violations = 0` et
`attach_violations = 0`. Alors remplacer `existed := first_batch[fid]
< b` par `false` partout laisse `build_forest` inchangé, sauf les deux
compteurs.

*Preuve.* `birth_violations = 0` donne à toute facette touchée dans un
lot exactement un rôle. Active seule : `attach` est faux, donc les
tests « attachement ∧ … » ne s'appliquent pas, et l'appartenance aux
racines pré-lot est déjà acquise par `active` — `existed` n'y entre
pas. Attachement seule : `attach_violations = 0` dit précisément que
`existed` est faux, donc les trois usages restants (comptage des
facettes nées, exclusion des racines pré-lot, inscription dans `born`)
lisent déjà `existed = false`. ∎

Sur un flux cohérent — et l'invariant des rayons de naissance (§ 5.2)
garantit qu'un flux correct l'est — `first_batch` n'est donc pas une
entrée du calcul mais un **instrument de détection**. Conséquence :
**rien ne prouvait que ce détecteur puisse se déclencher**, tous les
tests ne vérifiant que sa nullité.

Fixture permanente ajoutée à la porte (coordonnées symboliques, flux
volontairement incohérent — l'analogue exact des bases fictives
`cap_base_*` de la garde de capacité) : deux événements `K=2` dont la
même facette `{1,2}` est un attachement dans deux lots de niveaux
`ρ² = 25` puis `100`. Le backend figé doit y rendre `lots = 2`,
`attach_violations = 1`, `nées = 1`, sinon la porte échoue en code 3
avant toute comparaison. C'est le seul endroit où la sémantique du
minimum est observable, et le mutant `intern-first-batch-last` y meurt.

La question de savoir s'il faut aller plus loin — retirer `first_batch`
du calcul des rôles pour ne plus le laisser qu'aux compteurs — est
posée aux auditeurs dans
`audits/NOTE_CLAUDE_INTERNEMENT_ET_DETECTEUR_ATTACH_20260818.md` § 2.
Rien n'a été changé de ce côté sans arbitrage.

## 6. Ce que la mesure a AUSSI montré, et que je n'ai pas corrigé

Les dix folds par `K` sont répartis sur les fils en **tranches
contiguës** de `K = 1..10`, alors que le coût par `K` croît fortement :
à n=8000 les incidences se répartissent en
`0,2 / 0,7 / 1,6 / 3,1 / 5,2 / 8,1 / 11,9 / 16,7 / 22,6 / 29,7 %`. À
quatre fils, le découpage donne `{1,2} / {3,4,5} / {6,7} / {8,9,10}` :
le dernier ouvrier porte **69 % du travail**. Un ordre décroissant
ramènerait la borne à 29,7 % (le seul `K = 10`), mais ferait tourner
ensemble les quatre `K` les plus lourds — donc augmenterait le pic
mémoire, que le contrat 30M protège. Arbitrage latence/mémoire soumis
aux auditeurs, non tranché ici (note § 4).

## 7. Reproduction

```bash
cmake -S morsehgp3D_v4 -B build/v4 -DCMAKE_BUILD_TYPE=Release
cmake --build build/v4 -j
ctest --test-dir build/v4 --output-on-failure          # 131 tests
./build/v4/mhgp4_forest_probe --fold-compact-gate
./build/v4/mhgp4_forest_probe --fold-compact-gate --inject=intern-fid-first-seen   # code 4
./build/v4/mhgp4_forest_probe --fold-compact-gate --inject=intern-hash-no-verify   # code 4
./build/v4/mhgp4_forest_probe --fold-compact-gate --inject=intern-first-batch-last # code 4
./build/v4/mhgp4_forest_probe --fold-intern-bench --family=uniform --n=8000 \
    --s=8 --smax=11 --seed=3 --threads=4 --bench-repeat=5
```

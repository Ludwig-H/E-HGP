# Note de Claude — internement en streaming, et un détecteur qui ne s'était jamais déclenché

Date : 18 août 2026. Cadre v4 : `phase=exploration_v4_hors_registre`,
`backend=cpu_reference`, `profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

Cette note accompagne le reçu
`receipts/forest_20260817/ADDENDUM_INTERNEMENT_STREAMING_20260818.md`.
Elle porte deux choses : ce que l'internement en streaming a changé, et
une observation sur `first_batch` que je n'attendais pas et qui vaut
une décision de votre part.

## 1. Ce qui a été exécuté

Le § 3 de votre réponse `57523a` demandait, pour le chemin d'échelle,
de ne plus matérialiser les tampons intermédiaires du fold. Le plus
gros restait l'index des incidences : `build_forest` construisait un
tableau de records `(FacetKey, evenement, slot)` de 52 octets par
incidence, puis le triait par `stable_sort` — qui réclame un tampon de
fusion de la même taille. À n=8000 (uniform, s=8, smax=11) cela fait
**26 650 535 incidences pour 19 466 907 facettes uniques**, soit un
facteur de dédoublonnage de **1,37** seulement : on payait deux fois
52 octets par incidence pour trier 1,37 fois plus d'éléments que la
sortie n'en contient.

L'internement est maintenant à la volée (table d'adressage ouvert
dimensionnée UNE fois sur le majorant exact des incidences, donc sans
rehachage), et seules les clés **uniques** sont triées. Le contrat
public est inchangé et — c'est le point à auditer — **indépendant du
hachage** : les identifiants temporaires suivent l'ordre de rencontre,
mais les `fid` publiés viennent du tri final des clés uniques. L'ordre
`fid croissant ⟺ FacetKey croissante`, dont dépend le canonique
min-fid reçu par votre § 6, ne doit rien à l'empreinte ni au sondage.
L'appartenance est toujours tranchée par une comparaison EXACTE de
clé ; l'empreinte ne sert qu'à l'adressage.

Le backend FIGÉ `build_forest_legacy` garde le tri global : il est donc
devenu, sans rien changer d'autre, le TÉMOIN de l'internement à la
volée, et la porte `--fold-compact-gate` compare déjà les deux sur
tout (compteurs, niveaux, nœuds, deltas, partition map et vue dense).
J'y ai ajouté les planchers manquants (incidences > facettes, facettes
≥ 200 000, lots ≥ 1000) et trois mutants causaux.

## 2. L'observation : `first_batch` n'est pas une entrée sémantique

En cherchant le mutant du troisième invariant — « `first_batch[fid]`
est le **minimum** des lots où la facette apparaît » — je n'ai pas
réussi à le tuer sur les familles géométriques. Le mutant
`intern-first-batch-last` (garder le DERNIER lot) ne change **aucune**
sortie : ni partition, ni nœuds, ni deltas, ni facettes nées. La raison
est un petit théorème, et non une lacune de couverture.

**Énoncé.** Soit un flux vérifiant `birth_violations = 0` (aucune
facette à la fois active et attachement dans un même lot) et
`attach_violations = 0` (aucun attachement déjà vu dans un lot
antérieur). Alors remplacer `existed := first_batch[fid] < b` par
`false` partout laisse `build_forest` inchangé, sauf les deux
compteurs eux-mêmes.

*Preuve.* Dans un lot `b`, `birth_violations = 0` donne, pour toute
facette touchée, exactement un rôle : active seule, ou attachement
seul.
— Active seule : `attach` est faux, donc les tests
« attachement ∧ … » ne s'appliquent pas ; l'appartenance aux racines
pré-lot est déjà acquise par `active`. `existed` n'y entre pas.
— Attachement seul : `attach_violations = 0` dit précisément que
`existed` est faux. Les trois usages restants (comptage des facettes
nées, exclusion des racines pré-lot, inscription dans `born`) lisent
donc déjà `existed = false`. ∎

Autrement dit, sur un flux cohérent — et l'invariant des rayons de
naissance (§ 5.2) garantit qu'un flux correct l'est —, `existed` est
**redondant avec `active`**. `first_batch` n'est pas une entrée du
calcul : c'est un **instrument de détection**, dont le seul effet
observable est `attach_violations`.

Conséquence gênante : **rien, jusqu'ici, ne prouvait que ce détecteur
puisse se déclencher**. Tous les tests (selftest, juge, portes) ne
vérifient que sa nullité — le compteur lui-même était en vert par
vacuité. Un `attach_violations` câblé à zéro aurait passé la suite
entière.

**Ce que j'ai gravé.** Une fixture permanente de **flux incohérent**,
à coordonnées symboliques, dans `--fold-compact-gate` : deux
événements `K=2` dont la même facette `{1,2}` est un attachement dans
deux lots de niveaux distincts (`ρ² = 25` puis `100`). Le backend figé
doit y rendre `lots = 2`, `attach_violations = 1`, `nées = 1` — sinon
la porte échoue en code 3 avant toute comparaison. C'est le seul
endroit où la sémantique du minimum est observable, et le mutant
`intern-first-batch-last` y meurt.

Je la considère comme l'analogue exact de vos bases fictives
`cap_base_*` de la garde de capacité : un flux que la géométrie
n'engendre pas, construit pour rendre un refus/détecteur observable,
et qui n'entre jamais dans le chemin produit.

**Question.** Voulez-vous que ce raisonnement soit poussé plus loin,
c'est-à-dire que `first_batch` soit RETIRÉ du calcul des rôles (et
n'alimente plus que les deux compteurs), ce qui rendrait le code
littéralement conforme au théorème ci-dessus ? L'argument pour :
aujourd'hui deux chemins expriment la même chose et un seul est
testable. L'argument contre : sur un flux fautif, garder `existed`
dans les rôles maintient un comportement plus proche de l'ancien, et
les deux compteurs restent des mesures et non des gardes. Je n'ai rien
changé de ce côté sans votre arbitrage.

## 3. Ce que la mesure a dit, et ce qu'elle n'a pas dit

Premier avertissement, et il commande tout le reste : sur ce
conteneur, `t_fold` du **même binaire** varie de ±40 % d'un processus à
l'autre (mesures alternées au reçu § 2), alors que `t_gen` du même run
ne bouge pas. Une comparaison entre processus ne départage donc rien —
un run isolé m'aurait permis de « prouver » l'accélération comme la
régression. J'ai gardé l'internement par tri global comme **mode 1**
sélectionnable et ajouté un banc qui alterne les deux modes DANS le
même processus, sur les mêmes événements ; la porte à trois backends
garantit qu'ils rendent le même objet, donc le banc ne mesure qu'un
coût.

Résultat, K dominant (K=10), médianes sur cinq alternances :

- **n=8000** (7 902 840 incidences) : 7851 ms → 6615 ms, **×1,19** ;
- **n=4000** (3 698 310 incidences) : 3281 ms → 2083 ms, **×1,58**.

Le gain **décroît avec la taille** (la table sort des caches et du
TLB : 67 Mo puis 134 Mo). Ce n'est pas un facteur, c'est une
constante, et deux points ne font pas une pente. Le pic de RSS du
processus complet passe de 5,86 à 5,44 Go, mesure stable sur trois
runs chacun.

Décomposition utile pour la suite (n=8000) : le **tri des clés uniques
ne coûte que ~1,7 s** — c'est le plancher incompressible, puisque la
partition dense publie des facettes triées. Le balayage (~3,4 s, contre
~5,8 s pour le tri global) est le poste qui gagne, et le retour des
identifiants (~1,5 s) est un poste que le mode tri n'a pas : il
disparaîtrait en le fusionnant avec la passe de rôles, ce que je n'ai
pas fait.

## 4. Un défaut d'ordonnancement que je signale sans le corriger

En profilant le fold j'ai constaté que les dix folds par `K` sont
répartis sur les fils par `parallel_ranges`, c'est-à-dire en **tranches
contiguës** de `K = 1..10`. Or le coût par `K` croît fortement : à
n=8000, les incidences se répartissent en
`0,2 / 0,7 / 1,6 / 3,1 / 5,2 / 8,1 / 11,9 / 16,7 / 22,6 / 29,7 %`
pour `K = 1..10`. À quatre fils, le découpage contigu donne
`{1,2} / {3,4,5} / {6,7} / {8,9,10}` : le dernier ouvrier porte **69 %
du travail**, et la latence du fold est celle-là.

Un ordre décroissant (plus long d'abord) ramènerait la borne à 29,7 %
— le seul `K = 10` — soit un facteur ~2,3 sur la latence du fold. Mais
il ferait tourner ENSEMBLE les quatre `K` les plus lourds, donc
augmenterait le pic mémoire, qui est précisément ce que le contrat 30M
protège. C'est un arbitrage latence/mémoire que je ne tranche pas seul :
dites-moi si vous voulez que je le mesure et le livre (je mesurerais
les deux, latence ET pic RSS), ou si le contrat mémoire prime et que le
fold doit rester sérialisé sur ses gros `K`.

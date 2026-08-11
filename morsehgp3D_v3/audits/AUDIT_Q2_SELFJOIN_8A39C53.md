# Audit épinglé — self-join q2 à `8a39c53`

Date : 11 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

Périmètre immuable : `prototype/pair_selfjoin_probe.cpp` au commit
`8a39c53f41c1964b12d38b0129d7e8a0a5cc94e7`, SHA-256
`5f2b160e7ff58a6b017f8c9c351353686a8d7a61b6115ebca88e5894a432a688`.
Le statut du worktree postérieur est exclusivement dans
[`AUDIT_ETAT_COURANT.md`](AUDIT_ETAT_COURANT.md).

## Verdict du snapshot

Le prune géométrique local est exact. La réception logicielle du commit est
refusée : son différentiel agrégé est compensable et son parcours redémarre la
recherche de témoins à la racine pour chaque état. La sonde ne produit ni
coquille fermée, ni `BallActivation`, ni fold; elle ne mesure donc pas
`warm_e2e`.

Elle est un falsificateur q2 mass-only avec un budget configuré, pas une
architecture industrielle sans budget et pas une source q3/q4.

## Preuve du prune

Pour une paire `x,y` et un témoin `w`, poser
`Phi(w,x,y)=(w-x) dot (w-y)`. Le point `w` est strictement intérieur à la
boule diamétrale de `x,y` si et seulement si `Phi<0`.

Sur trois boîtes cartésiennes `W,X,Y`, le maximum de chaque contribution
scalaire `(w-x)(w-y)` est exact aux extrémités : pour `w` fixé, l'expression
est affine séparément en `x,y`; pour `x,y` fixés, elle est convexe en `w`, donc
son maximum est à une extrémité. Le sup tridimensionnel est la somme de trois
maxima sur huit combinaisons, soit 24 produits entiers.

Si ce sup est strictement négatif, tous les `PointId` du nœud témoin sont
strictement intérieurs pour toutes les paires du produit. Les nœuds acceptés
doivent former une antichaîne de plages disjointes. Toute plage recouvrant une
extrémité descend jusqu'aux feuilles afin d'exclure les deux `PointId` de la
paire.

Dix identifiants distincts donnent `p>=10`; avec `q=2`, le théorème d'inertie
donne `p+q>=12` et autorise une tombstone du seul quotient horizontal jusqu'à
`K=10`. Un contact `Phi=0` appartient à la coquille et ne compte jamais comme
témoin strict.

Sous u16, `|Phi|<2^34`; un entier signé de 64 bits suffit au prédicat
ponctuel.

## Réfutation du différentiel committé

Le commit compare le nombre de paires non inertes au nombre de paires arrivées
en microtuiles. La condition `non_inert<=microtile_pairs` ne prouve pas
l'inclusion : une paire non inerte supprimée et une paire inerte conservée se
compensent. L'identité
`pruned_pairs+microtile_pairs=C(n,2)` peut elle aussi survivre à une omission
et un doublon de même masse.

Trois CTests q2 sont bien enregistrés dans ce snapshot. Ils passent, mais ne
portent ni sort paire par paire ni mutant de compensation. Les sorties qui
nomment cette comparaison « soundness » sont donc trop fortes.

Le juge borné minimal est un tableau triangulaire de sorts :

1. chaque paire passe exactement une fois de `UNASSIGNED` à `PRUNED` ou
   `TERMINAL`;
2. un second marquage et un sort final non assigné échouent;
3. toute paire non inerte doit être `TERMINAL`, ou chaque sort `PRUNED` doit
   être recompté exactement;
4. des mutants tuent omission, duplication compensée, seuil 9, contact compté
   strict et dernier bloc omis.

Ce bitmap quadratique est réservé au différentiel à petit `n`; il n'entre
jamais dans le produit.

## Profil de coût du snapshot

À `n=2400`, feuilles de taille 8, le snapshot visite entre 17,7 et 60,7
millions de nœuds témoins pour 20,6 à 67,7 milliers d'états, soit environ 84 à
88 % des 1 023 nœuds de l'arbre témoin par état.

| famille | états | visites témoins | paires terminales | part terminale |
| --- | ---: | ---: | ---: | ---: |
| terrain | 24 186 | 20 935 465 | 144 986 | 5,04 % |
| scanline simple | 20 600 | 17 730 179 | 126 516 | 4,39 % |
| multi-écho | 32 984 | 29 134 865 | 204 657 | 7,11 % |
| uniforme | 67 668 | 60 688 635 | 407 313 | 14,15 % |

Ces compteurs sont des propriétés du snapshot. Ses chronos historiques ne
sont pas des p95, n'ont pas de reçu brut et ne dimensionnent aucun delta
postérieur.

Le pire cas reste cubique pour la recherche bloc--nuage. Un classifieur
terminal naïf ajoute un autre terme cubique. Le budget `--max-states` classe
en outre l'exécutable comme falsificateur censuré, même lorsqu'il n'est pas
atteint.

## Borne inférieure exacte absente du snapshot

Le même produit d'AABB possède un infimum séparable exact. Pour un axe, avec
`W=[wl,wh]` et des extrémités `x,y` de `X,Y`, poser
`t=clip(x+y,[2*wl,2*wh])`. Le minimum multiplié par quatre vaut
`(t-2*x)(t-2*y)`; prendre le minimum sur les quatre couples d'extrémités puis
sommer les trois axes donne `L4`. Le maximum `U4` est obtenu sur les huit
triples d'extrémités par axe.

- `U4<0` certifie un nœud entièrement témoin;
- `L4>=0` certifie qu'il ne contient aucun témoin strict;
- sinon il descend.

Sous raffinement des blocs d'extrémités, `L4` augmente et `U4` diminue. Les
deux décisions sont donc héritables. Un nœud avec `L4=0` peut être retiré de
la recherche d'intérieurs, mais il doit être revu par le census terminal pour
retrouver les points de coquille. Les valeurs doublées tiennent en `int64`
signé sous u16.

Une frontière héritée reste exacte seulement si elle conserve sans cap tous
les nœuds ambigus, y compris les feuilles, et si elle remplace un nœud
développé par ses enfants. Un cap diagnostique exige un redémarrage ou un
refus explicite; il ne peut jamais supprimer silencieusement une partie de la
recherche. Le statut de cette borne dans tout successeur appartient uniquement
à [`AUDIT_ETAT_COURANT.md`](AUDIT_ETAT_COURANT.md).

## Portée q2 seulement

Dix points dans la boule diamétrale d'une paire ne prouvent rien sur une
sphère q3/q4 décalée dans le plan médiateur. Le résiduel q2 ne peut donc jamais
servir de source d'ancres supérieures.

La fixture durable prend `a=(50,100,100)`, `b=(150,100,100)`,
`z=(100,160,100)` et dix témoins sous le plan `y=100`. La paire `ab` est
H0-inerte en q2, mais reste l'arête maximale du support q3 propre `{a,b,z}`;
les dix témoins sont hors de son cercle circonscrit. Une borne de bloc peut
conserver cette paire en microtuile : la fixture doit tester le fait
mathématique et la portée des lanes, pas imposer un sort `PRUNED` au parcours
conservateur.

La comparaison produit pertinente est Morton/LBVH + Yao48 strict +
classifieur terminal et census fermé. Le self-join peut rester un oracle, un
falsificateur ou un second prune si ses masses le justifient.

GCP non utilisé.

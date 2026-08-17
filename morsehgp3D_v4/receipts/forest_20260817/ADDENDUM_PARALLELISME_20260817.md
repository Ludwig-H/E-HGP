# Addendum — parallélisme par rectangle dans la génération (directive « paralléliser »)

Date : 17 août 2026. Base de mesure : `c945bad` ; code livré dans le
commit portant ce reçu. Directive utilisateur du jour : « il faut tout
optimiser. Puis paralléliser et écrire pour GPU. »

## Ce qui est implémenté

`collect_candidate_balls` accepte `num_threads` (défaut 1 = chemin
séquentiel historique, sans fil). Chaque lane (q2, q3, q4) distribue ses
rectangles vivants par TIRAGE DYNAMIQUE (compteur atomique — les gros
rectangles ne bloquent pas la fin de vague) à des ouvriers qui
possèdent chacun leur brouillon (`LaneScratch` : histogrammes, cover,
sites axiaux, gid, antichaîne), leur vecteur d'émissions et leurs
statistiques, fusionnés à la fin (`BallStreamStats::add_from`, addition
membre à membre). Le probe expose `--threads=N`.

## Pourquoi c'est exact

Les rectangles d'une lane sont indépendants ; le MULTIENSEMBLE des
émissions est identique quel que soit le découpage ; le tri stable +
RLE en aval canonise l'ordre — donc l'objet post-RLE, le census, la
forêt et le rendu sont au bit près ceux du séquentiel. Les compteurs de
génération sont des sommes, indépendantes de l'ordre. Seuls les chronos
changent de sens : à N fils ils cumulent du temps CPU (somme des fils),
plus du temps mural.

## Portes (105 CTest verts)

- `--par-gate` : collect à 1 fil CONTRE 4 fils, sur uniform n=400 et
  eight_clusters n=400, chemins baseline ET axial — égalité au bit près
  des clés/arités/représentations post-RLE, ET égalité des compteurs
  (candidats, ancres, morts de profondeur, W₄, cœur de seed — jusqu'à
  `seed_core_nodes`, déterministe par seed —, groupes axiaux).
- Mutant `par-drop-shard` (la fusion oublie le premier ouvrier) : tué à
  code 4 — la porte compare pour de vrai, et le chemin multi-fils est
  bien exercé.

## Mesures (4 vCPU du conteneur ; sorties identiques au compte près)

| run (s=8, smax=11, seed=3) | 1 fil | 4 fils | speedup | événements |
|---|---|---|---|---|
| eight_clusters n=1000, axial | 34,9 s | **9,7 s** | ×3,6 | 219 653 = |
| eight_clusters n=1000, baseline | 35,4 s | **9,8 s** | ×3,6 | 219 653 = |
| uniform n=1600, baseline | 27,0 s | **10,6 s** | ×2,5 | 532 181 = |

Cumul sur le cas dur eight_clusters n=1000 depuis le début de la
séquence d'audits : `t_gen` **135,9 s → 9,7 s (×14)** — sweep à deux
côtés (−9 %), cœur de seed de Jung (−60 % de plus), parallélisme
(×3,6) ; chaque étape à sorties identiques et jugée. Le speedup
uniforme (×2,5) est borné par la répartition des rectangles ; le tirage
dynamique le maintient malgré des tailles très inégales.

## Portée et suite

Le parallélisme couvre la GÉNÉRATION (le poste dominant partout) ; tri,
préfiltre, census et fold restent séquentiels (1 à 3 s chacun à cette
échelle — prochains candidats une fois la génération à sa borne). Sur
G4 (48 vCPU), le même code s'applique ; la campagne d'échelle devra
choisir `--threads` en cohérence avec la concurrence pilotée par la
mémoire du protocole (isolated_latency reste à 1 fil par défaut tant
que le protocole épinglé n'est pas amendé). L'écriture GPU (directive
utilisateur) a maintenant ses deux noyaux réguliers candidats : le cœur
de seed (descente bornée) et la primitive de sweep (tableaux fixes,
sans allocation). `public_status=not_claimed`, rien de tout ceci n'est
une promotion.

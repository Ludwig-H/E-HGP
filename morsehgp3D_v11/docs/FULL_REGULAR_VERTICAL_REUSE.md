# Réemploi de la cellule régulière pour sa naissance verticale

Port natif opt-in, qualification G4 encore attendue. La capture close
`census2`, source`cc93360a3`, ne contient pas ce réemploi ; le banc
`dense1`, source`768070ddb`, non plus. Aucune cellule, trace ou verticale
publiée n'est supprimée. L'option `reuse_regular_verticals` reste désactivée
par défaut et ne modifie pas la signature publique de `build_forest`.

## Lemme et date exacte

Soit une boule régulière b, avec m=qmin=q, intérieurs I et coquille U.
Posons h=p+q, avec h≤K. Sa naissance d'ordre h porte P=I∪U (T3).
À l'ordre h−1, chaque face R=I∪(U\{u}) est une trace stricte de sa
cellule : β(R)<λb (T2). Sa descente T5 fournit une naissance s dans la
même composante que R dès β(R), avec βs≤β(R).

La partie historique F utilisée pour la verticale est une autre face de P.
Si F≠R, F∪R=P et β(P)=λb : les deux sommets sont donc adjacents dans
Γ(h−1) à la coupe **fermée** λb. T1 et T6 donnent exactement la même
image verticale après `ancestor_closed(s, λb)`. Le résultat doit être lu
après la clôture atomique du plateau (T4), même si la cellule n'a produit
qu'une continuation. Le niveau terminal βs ne remplace jamais λb.

Contre-exemple aux raccourcis : les points Morton (3,0,0), (0,3,0),
(2,2,0) donnent q2, I={dernier}, λb=9/2. La verticale historique omet
l'intérieur et sa MEB vaut9/2. Les faces régulières omettent chacune une
extrémité ; leurs naissances, distinctes, ont niveau5/4 et fusionnent
exactement à9/2. La graine brute, une coupe ouverte ou la date terminale
donnent toutes une mauvaise image.

Ne pas généraliser à m>q. Le triangle droit q2/m3 n'a pas de naissance
à p+q=2. Le carré q2/m4 a des naissances d'ordres3 et4 au même niveau.
Le cas étendu conserve donc sa descente historique.

## Port et mémoire

Une table `NodeIdx` de M cases, initialisée à `kNone`, est allouée pendant
FULL : chaque boule régulière a un unique h=p+q. La cellule d'ordre h−1
écrit sa première graine basse ; la verticale d'ordre h lit cette case.
Les écritures et lectures des deux ordres consécutifs portent ainsi sur
des BallIdx distincts. Ne jamais stocker une racine du DSU après fusion.
La table emprunte le même domaine immobile, coûte4M octets avec tous les
autres buffers vivants et est rendue avant le déplacement final du domaine.
K1 seul n'alloue rien. Son admission précède l'allocation et un échec FULL
détruit le cache privé sans déplacer le domaine ni publier les timings.
`remember/find` ne recertifient pas la géométrie de la graine : `build_full`
la fournit depuis ses propres forêts, après vérification du contexte. Ce
helper interne ne définit pas une API de cache pour des forêts étrangères.

Une entrée régulière éligible absente est une violation d'invariant ;
un repli silencieux masquerait une couverture manquante. Vérifier ordre,
arité, bornes d'indice et rang strict de la graine. L'option désactivée
et les cellules étendues gardent la voie historique.

Pour le parallèle, conserver fenêtres et ordinal de naissance, donc
`ordinal % lanes` pour les mémos. Le pilote remplit les graines
réutilisées et ignore une fenêtre sans descente restante. Recompacter
arbitrairement les requêtes risquerait de faire écrire deux workers dans
le même mémo logique. Les appels supprimés changent aussi les insertions
et évictions futures : publier un compteur de réemploi distinct et ne
plus demander l'identité de tout le travail avec l'option désactivée.
`vertical_reuses + vertical_descents = births` pour K>1 ; le second
compteur ne porte que les appels réellement faits. `max_vertical_batch`
reste la largeur de la fenêtre originale, qui peut dépasser son nombre
de descentes restantes. Les workers ne touchent jamais le DSU.

## Preuve expérimentale limitée

`regular_vertical_reuse_model.py` importe uniquement `model` et `definition`,
pas le constructeur ni le produit. Ses70 petits nuages donnent239 boules
régulières,501 faces,516 choix de terminaux et2522 contrôles, dont huit
cas où la verticale historique omet un intérieur. Les mauvaises variantes
divergent pour492 coupes ouvertes,516 absences de remontée,516 dates
terminales et103 graines étrangères ; quatre gardes corrompues sont refusées.
Ces contre-exemples Python ne sont pas des mutants natifs exécutés.

Sur les compteurs combined3, les cellules régulières de l'ordre inférieur
couvrent99,986/99,988/99,964 % des descentes verticales des trois LiDAR u21.
Ce sont des possibilités de réemploi, pas des taux de gain : le catalogue
coûtait encore729–949 ms et la publication régulière119–163 ms.
Les huit nouvelles portes natives couvrent W1/4/48, Q1/2/4096,
combinaisons mémo/census/dense, fenêtres mixtes et sans descente,
dates fermées, cas étendus, admission4M, ALLK12 exact/−1, injections
d'échec W1/W4 et option inactive. Leurs planchers sont4500/800/100/40
et30/80/40/25 ; ces nombres sont des obligations, pas des résultats.
Quatre mutants attendent la qualification native : graine non remontée,
mauvaise composante à indice valide, case éligible absente et coquille
étendue traitée comme régulière. Les deux derniers doivent causer un refus
de succès, sans être annoncés comme une mauvaise sortie géométrique.
La contrelecture statique et le modèle Python passent ; FULL≤200 ms et
l'accélération GPU restent ouverts.

## Banc apparié

Le bit1024 demande ce réemploi, après512 pour le lookup dense. Le calendrier
`--reuse-verticals` prévoit29processus : triples511/1023/2047 sur les six
entrées LiDAR u21/u24 et trois uniformes, plus2047W1/W8 sur ng00u21.
Le travail payé reste identique entre511 et1023 ; entre1023 et2047 les
images doivent être identiques, mais les descentes, mémos et pas de recherche
DSU peuvent changer. Les fenêtres et la mémoire sont jugées avant réemploi
d'un résumé sémantique. Les tables de temps proviendront du FULL mur explicite.

Le collecteur a18lectures Python normal/−O closes, sans natif. La nouvelle
porte vérifie141essais,59corruptions et2550petites configurations de fenêtres.
Le plan déclare850+180+570secondes, plus120de préparation, soit1720≤1737 ;
budget du banc500, toute omission reste explicite. La discordance historique
du parseur CLI (1023 refusé malgré son validateur) est corrigée et conservée
dans le reçu de développement, sans réécrire la source de dense1.

# Contrat Q_b et transition vers les événements d'ordre K

Revue de conception, moteur épinglé `ee2b48b4c306f7da403657fb5291aa78569e6296`. Les copies sources proviennent du premier relevé Git, avant lecture ; ce sous-reçu portable a été constitué après cette lecture, sans substituer les sources courantes aux copies. Le chantier de sortie `supports` n'est pas implémenté ici. Aucun C++/CUDA, build, ajustement de modèle, G4 ou mesure native n'a été exécuté.

## Deux gardes utiles au prochain port

1. **Q_b ne se limite pas à qmin.** `JETON.md:61–74` demande tous les supports affinement indépendants dont le centre est dans l'intérieur relatif, minimaux par inclusion. Le cube `U={0,2}³`, centre `(1,1,1)`, niveau carré `β=3`, a exactement quatre diamètres q2 et deux tétraèdres alternés q4, tous de poids strictement positifs. Il n'a aucun q3 positif. Chaque tétraèdre est minimal par inclusion, même si une autre paire de la coquille porte la même boule. Le canoniseur `support.cpp:90–108` retourne correctement le premier support de cardinalité minimale S* ; il ne peut donc pas être utilisé comme énumérateur Q_b, ni être simplement prolongé aux autres supports de cette même cardinalité. Reconstruire Q_b depuis **U complet**, sans limiter les arités à qmin ou aux présentations acceptées par le générateur. Une seule identité de boule `BallIdx`, sa famille Q_b partagée, et les incidences datées vers l'arbre suffisent ; `FullDomain::find_support` n'accepte que la clé S* (`full_domain.hpp:25–29`).

2. **L'événement faible doit être conservé.** `points_export.cpp:131–132,194–197` retient les couvertures fortes `p+qmin≤k≤p+m`. Ce contrat convient à son export de points. La fenêtre des cellules critiques est plus large : `p+qmin−1≤k≤p+m` (`cells.hpp:83`). Le triangle équilatéral entier `(0,0,0),(2,2,0),(2,0,2)` a `p=0,m=qmin=3` et `β=8/3`. À K2, les trois lentilles de paires naissent à `β=2`, sont convexes et disjointes avant `8/3`, puis se rencontrent simultanément au centre du triangle : c'est une multifusion critique exclue par `strong`. À K1, une paire q2 est également un événement faible exclu par `strong` ; le code actuel exporte des incidences singleton à cet ordre. Le futur export supports doit donc avoir son propre sélecteur d'événements. Le niveau daté de la boule et son propriétaire **après fermeture de tout le plateau** restent distincts du niveau de naissance du nœud ; les continuations n'impliquent pas un nouveau nœud.

Ces constats sont des exigences du **nouveau** contrat. Ils ne sont pas des défauts de conformité démontrés dans le moteur ou l'export de points existants. La reprise d'un propriétaire par descente puis ancêtre fermé est utile, mais les hypothèses/gardes « forte » de `ball_nodes` ne se transportent pas inchangées aux événements faibles. Toutes les traces et incidences doivent demeurer disponibles à la fermeture du plateau.

## Vérification bornée

`check.py` utilise uniquement `Fraction` et un système barycentrique exact indépendant. Il énumère les 154 tuples q2/q3/q4 du cube, vérifie positivité et minimalité par inclusion, 16 permutations de la numérotation et une similitude entière. Il vérifie les distances exactes du triangle et l'écart de fenêtre, puis les contrats textuels des sources copiées. Il ne construit ni forêt native ni nouveau graphe de supports, et ne prétend pas mesurer leur coût.

Commandes exactes, depuis ce répertoire :

```sh
python3 -B -S check.py > normal.json 2> normal.stderr
python3 -B -S -O check.py > optimized.json 2> optimized.stderr
cmp normal.json optimized.json
sha256sum -c SHA256SUMS
```

Les deux exécutions réussissent avec **1 090 gardes**, sorties identiques, stderr vides. `BEFORE.json` conserve les pins et l'état initial réel de l'acteur (WIP GPU distinct), `AFTER.json` enregistre la recoupe des sources et le passage à `57dd21be1` sans assimiler les changements de diagnostics GPU au présent contrat. Les limites d'allocation de la famille Q_b restent à contracter dans le futur port : parcours combinatoire potentiellement grand, admission/count-fill vérifiées et refus sans export partiel ; aucune borne de temps n'est déduite de ce petit cube.

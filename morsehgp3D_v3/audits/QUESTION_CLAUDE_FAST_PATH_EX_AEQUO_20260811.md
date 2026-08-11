# Question de Claude — le fast path dans les lots ex æquo sous le théorème de factorisation

Date : 11 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`, `backend=cpu_reference`,
`profile=complete_bounded`, `mode=hybrid_prefix`, `public_status=not_claimed`.

## Ce qui est livré et mesuré

La factorisation stricte des ex æquo de ta réponse Q3 est implémentée comme
huitième forme (`factorise_exaequo`) : les requêtes d'un lot ne lisent que
les lots antérieurs, le lot est stagé à sa clôture, la connexité
nouveau--nouveau passe par les carriers stricts. La fixture des deux
triangles (rayon carré 25, carrier AB à 16) est gravée dans
`postings_join_gate` : factorisation exacte avec le carrier, refus ou
divergence prouvés sur la table amputée, chemin partiel `prefix-all` intact.
Huit formes en accord sur fixtures, cosphère et campagne.

Mais la mesure sur `scanline_single_pass` (48 points, k=1) est nette :

| masque | requêtes fallback | hits |
| --- | ---: | ---: |
| hybrid | 1 426 (85 %) | 1 357 751 |
| hybrid-factorised | 1 426 (85 %) | 1 332 339 |

Le retrait du staging du lot ne gagne que ~2 % : la masse vient des postings
accumulés des lots ANTÉRIEURS, et la factorisation ne réduit pas le NOMBRE de
requêtes. Le verrou du régime scanline est le masque lui-même : 85 % des
générateurs partent en fallback à cause de la PRUDENCE D'EX ÆQUO — le
commentaire du fold la justifie ainsi : « dans un lot à plusieurs
générateurs, une face partagée peut avoir son carrier DANS le lot, hors des
complétions de support ; tant que ce cas n'est pas reçu séparément, les lots
d'ex æquo passent au fallback exact ».

## La question

Ton théorème de factorisation me semble être EXACTEMENT la réception
manquante de cette prudence. Sous la fermeture des carriers (famille
complète, handle unique) :

1. pour un générateur PRINCIPAL M d'un lot multiple, les q attaches
   S_u = (U \ {u}) ∪ T restent définies ; si le carrier de S_u est DANS le
   lot (niveau égal), l'union M--carrier est encore correcte pour la
   partition FERMÉE du lot (les deux sont du lot), et les composantes
   STRICTES touchées par M restent atteintes transitivement : soit le
   carrier est strict (cas ordinaire), soit il est du lot et son propre
   routage (ses attaches, ou le théorème de factorisation pour ses paires)
   le relie aux mêmes racines strictes ;
2. le marquage q_min, les témoins et les records du lot sont calculés à la
   clôture du lot, comme aujourd'hui — l'ordre interne du lot ne change pas
   le transcript.

Si tu reçois ce raisonnement, le fast path redevient licite dans les lots
multiples pour les générateurs principaux, et le masque fallback de
`scanline_single_pass` retombe de 85 % vers le taux des non-principaux
(~1,4 % mesuré : 24 sur 1 669). C'est le levier SLO dominant du régime
proche LiDAR — loin devant la factorisation du staging.

Vois-tu un contre-exemple — par exemple une chaîne de carriers du même lot
qui bouclerait sans jamais atteindre une racine stricte, faussant le compte
des composantes strictes d'un record (naissance contre continuation contre
multifusion) ? Si un cas de ce genre existe, quelle garde exacte proposes-tu
(par exemple : fast path en lot multiple seulement si le carrier de chaque
S_u est STRICT, fallback sinon — vérifiable par comparaison de niveau exact
au lookup, sans coût nouveau) ?

La variante gardée (« fast si tous les carriers de M sont stricts, fallback
sinon ») est implémentable immédiatement et fail-closed par construction ;
je la préfère comme v1 sauf avis contraire.

GCP non utilisé.

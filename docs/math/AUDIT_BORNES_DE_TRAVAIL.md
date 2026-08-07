# Audit : quels étages ont une borne de travail, et lesquels n'ont que des plafonds

Document de synthèse, lecture seule. Aucun claim, aucune porte ouverte ou fermée.

## Pourquoi cet audit

Deux fois, le projet a payé la même confusion.

À l'étage higher, la conception bornait la mémoire et déclarait explicitement que
le nombre $V$ de produits visités n'était borné par rien
(`FRONTIERE_DIRECTE_SUPPORTS_3_4.md` §7). Quatre sessions G4 ont fini par
mesurer ce que cela coûtait : quatorze ordres de grandeur.

À l'aval, la fermeture de descente de facette borne son travail « par les
plafonds de confiance, le budget de chaque pas et le plafond du nombre d'appels »
(`FERMETURE_DESCENTE_FACETTE_SPARSE_PHASE10.md`), et prévient dans sa propre
section de limites qu'à 50 000 points « une requête LBVH, un shell ou une longue
chaîne peuvent encore être linéaires ». La première mesure isolée du 7 août
observe un exposant global de 1,72 en nombre d'événements.

Dans les deux cas l'énoncé était **écrit d'avance**, et dans les deux cas il n'a
été lu qu'après la mesure. Cet audit lit les autres avant.

**Un plafond n'est pas une borne.** Un plafond dit qu'une exécution s'arrêtera ;
une borne dit combien elle coûtera. Un étage qui ne connaît que des plafonds peut
être exact, terminant et certifié, et rester à des ordres de grandeur du contrat
sans que rien dans ses documents ne le signale.

## Ce que les documents disent d'eux-mêmes

| étage | l'énoncé, tel qu'écrit |
| --- | --- |
| frontière directe supports 3/4 | « si $V$ produits sont visités, cette écriture ne borne toujours pas $V$ : dans un cas adversarial, $V$ peut encore être de l'ordre de $\binom nm$ » |
| fermeture de descente de facette | « les plafonds de confiance, le budget commun de chaque pas et le plafond du nombre d'appels bornent le travail total » — puis, à 50 000 points, « une requête LBVH, un shell ou une longue chaîne peuvent encore être linéaires » |
| première incidence sparse | « une borne de 176 supports ne constitue donc **pas une borne uniforme en temps machine** » |
| locator positif sparse | « une chaîne DSU, une frontière LBVH ambiguë, un shell massif ou une longue descente **peuvent encore être linéaires** » |
| tour de boules saturées | « le journal persistant possède seulement la borne générale $O(M^2)$ héritée des paires examinables ; **aucune borne sous-quadratique n'est démontrée** » |
| catalogue critique 3D | « cette borne combinatoire **n'est pas une borne pratique** » |
| architecture industrielle H0 | « le plan **ne borne pas encore** les successeurs découverts par la fermeture commune, la profondeur LBVH, la difficulté des rationnels, les octets de scratch ni le temps GPU » |

Sept étages, sept aveux explicites. Aucun n'est un défaut caché : tous sont
écrits, datés et assumés. Ce qui manquait était de les lire **ensemble** — et de
distinguer ceux qui pèsent sur le contrat de ceux qui n'y sont pas.

## Lesquels sont dans le chemin produit

Un aveu de non-borne n'a pas le même poids selon qu'il concerne le binaire
produit ou un oracle borné conservé hors ligne. La chaîne d'inclusions du runner
tranche, et il faut la suivre plutôt que la deviner :

`direct_morse_product_runner` → `direct_morse_forest_reducer` →
`direct_sparse_facet_descent_batch_executor` →
`direct_sparse_facet_descent_closure` → `direct_sparse_facet_descent_step`.

| étage | dans le chemin produit | portée de l'aveu |
| --- | :-: | --- |
| frontière directe supports 3/4 | **oui** | mesuré, et c'est le verrou déjà refermé par la germination |
| fermeture de descente de facette | **oui**, par le réducteur | mesuré le 7 août, exposant 1,72 en événements |
| architecture industrielle H0 | **oui** | plan de lots du même chemin |
| catalogue critique 3D | **oui** | catalogue des supports du chemin terminal |
| première incidence sparse | non | pas inclus par le runner |
| locator positif sparse | indirect | atteint par le réducteur, pas inclus directement |
| tour de boules saturées | **non** | piste **abandonnée**, conservée comme repli de preuve borné |

Quatre étages du chemin produit avouent donc ne pas borner leur travail, et un
cinquième y touche indirectement. La tour de boules saturées, elle, est hors
sujet : le registre des pistes abandonnées l'a écartée précisément parce qu'elle
menait « à des univers de supports et memberships incompatibles avec le passage à
l'échelle ». Son aveu confirme l'abandon, il ne menace rien.

## Ce que cela dit de la feuille de route

Le contrat à 50 000 points a été discuté jusqu'ici comme si le verrou était
l'étage higher, puis l'étage paire, puis le coût unitaire. Cet audit montre que
la question est plus large : **la majorité des étages du pipeline bornent leur
terminaison et non leur coût.** Une borne de travail y est l'exception, pas la
règle.

Trois conséquences pratiques :

1. **Aucun étage ne doit être déclaré « prêt pour l'échelle » sur la foi de ses
   plafonds.** La question à poser à chaque étage est : *quelle quantité
   mesurable borne ton travail, et cette quantité est-elle bornée à 50 000
   points ?* Si la réponse est un plafond, la réponse est non.
2. **La mesure à nuage fixe et paramètre variable est le bon instrument**, parce
   qu'elle isole la variable dont le coût dépend. Elle a fonctionné pour l'aval
   en cinq minutes là où des comparaisons entre tailles de nuage n'avaient rien
   donné en plusieurs sessions.
3. **L'ordre des travaux change.** Mesurer l'exposant de chaque étage est
   local, sans GPU, et coûte des minutes ; concevoir une optimisation pour un
   étage dont l'exposant est inconnu coûte des sessions et peut viser le mauvais
   étage — ce qui est exactement arrivé au filtre fp64.

## Ce que cet audit ne fait pas

Il ne mesure rien : il rapporte ce que les documents disent d'eux-mêmes. Un étage
absent du tableau n'est pas pour autant borné — il peut simplement ne rien
déclarer, ce qui est une situation moins bonne, pas meilleure. Et un étage qui
avoue une linéarité possible n'est pas nécessairement le prochain verrou : seule
la mesure le dira.

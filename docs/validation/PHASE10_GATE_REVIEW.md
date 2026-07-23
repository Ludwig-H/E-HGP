# Revue de porte Phase 10 — généalogie Morse horizontale directe

> [!IMPORTANT]
> Cette revue ferme administrativement la Phase `10` comme implémentation candidate pour le backend `reference_cpu`, le profil `hgp_reduced` et le mode `certified`, avec `refinement_status=conditional_h0_candidate` et `public_status=not_claimed`. La fidélité globale des carriers à Gamma reste une obligation de preuve; cette fermeture autorise seulement les travaux `architecture_only` de Phase 14.

## Décision

- Phase : `10` — journal Morse et réduction directe.
- Porte d'entrée : satisfaite par la Phase 9 et le jalon local d'ordre un de Phase 5.
- Verdict de sortie : satisfait pour le jalon logiciel conditionnel; non satisfait comme théorème d'exactitude horizontale.
- Chemin normatif : catalogue complet 10.1, bras 10.2, locator 10.5a, fermeture 10.5c, quotient gelé sur $R\sqcup L$, classification $q_R$ puis commit atomique.
- Statut des jalons 10.6--10.15 : audits facultatifs de falsification, hors chemin produit.
- Effet opérationnel : Phase 10 `completed` comme candidat conditionnel; Phase 14 `in_progress` en `architecture_only`, sans promotion scientifique.

## Argument mathématique

Un simplexe de Gabriel $S=I\cup U$ fournit seulement les facettes $S\setminus\lbrace u\rbrace$ pour $u\in U$. Le support positif minimal vérifie $2\leq\lvert U\rvert\leq4$ en dimension trois et l'essentialité de ses points donne $\beta(S\setminus\lbrace u\rbrace)<\beta(S)$. Les suppressions de $I$ restent au niveau du simplexe : elles ne sont ni des bras stricts, ni des composantes du snapshot antérieur.

Chaque bras est fermé par 10.5c sous un locator gelé et doit atteindre le carrier d'un minimum direct strictement antérieur. Pour $k\geq2$, un carrier isolé ne porte aucun nœud réduit. Les simplexes d'un même couple ordre--niveau deviennent des hyperarêtes sur les sommets typés $R(r)$ des carriers déjà réduits et $L(h)$ des carriers encore latents. La fermeture transitive conserve les $L$; dans chaque groupe, le nombre $q_R$ des seuls identifiants $r$ donne une naissance pour zéro, une continuation pour un et une multifusion pour au moins deux. Tous les carriers sont ensuite unis; les minima du niveau courant sont engagés après le quotient et ne peuvent devenir ses enfants.

Cette induction par niveaux exacts est conditionnelle à la complétude du catalogue de Gabriel, y compris les selles tardives, à la terminaison positive de tous les bras et surtout à la fidélité des carriers pré-lot aux composantes strictes de Gamma. Les requêtes $\lambda(F)$ et $M(F)$ certifient les premières promotions des minima latents mais ne remplacent jamais ce catalogue. La Proposition 6 et le Théorème 5 du manuscrit ne suffisent pas : E5 réfute le graphe Gabriel brut, car une attache non Gabriel silencieuse devient nécessaire à une selle tardive. La descente enrichie répare ce cas localement; la surjectivité inductive du certificat sparse reste à démontrer, indépendamment de M.1 qui traite ensuite l'identification contractuelle et verticale.

## Évaluation de la porte

| critère | preuve | décision |
|---|---|---|
| minima et simplexes de Gabriel | façade terminale de Phase 9, rejeu frais 10.1 | go dans la portée directe |
| bras stricts complets par simplexe | rejeu frais 10.2, un retrait par point du support positif | go |
| terminaisons antérieures | une fermeture 10.5c par lot non vide, terminal lié à un carrier rejoué | go ou échec fermé |
| fidélité globale des carriers | induction Gamma et surjectivité de toutes les attaches silencieuses vers le certificat sparse | obligation de preuve ouverte |
| simultanéité | quotient de toutes les selles du lot sur $R\sqcup L$ avant toute mutation; les ponts latents sont conservés | go |
| réduction des isolés | minima $k\geq2$ latents, règle $q_R=0,1,\geq2$ | go |
| forêt compacte | minima, liaisons bras--carrier, groupes, enfants, nœuds et racines finales | go |
| absence de structure globale interdite | aucune cellule, coface globale, Gamma ou mosaïque de Delaunay d'ordre supérieur dans la cible | go |
| portée publique | M.1 et migration contractuelle absents | `not_claimed` |
| performance | aucun benchmark de qualification | non qualifiée |

## Validation volontairement courte

Un seul exécutable court vérifie les fixtures permanentes. Le triangle aigu impose des minima latents puis une naissance réduite $q_R=0$ sans enfant. Le miroir 6.18 impose que deux selles reliées uniquement par un même carrier latent donnent une naissance commune, et non deux actions séparées. La vraie fixture E5 tridimensionnelle vérifie que le bras `AC` de `ABC` atteint la composante gelée contenant le minimum `DE`, puis que le niveau exact $83886/3563$ reste une continuation $q_R=1$ sans nouveau nœud; le résultat complet est rejoué fraîchement. Ce cas discriminant falsifie le flot brut et valide la composition logicielle locale, pas la surjectivité globale des carriers. Aucun oracle Gamma supplémentaire, produit de budgets, sanitizer, benchmark, GPU ou GCP n'est exécuté pour cette fermeture.

## Structures évitées

Le journal ne matérialise ni l'univers des facettes, ni les suppressions intérieures de toutes les selles, ni les premières incidences, ni un catalogue de gateways silencieuses, ni Gamma, ni cellule top-$m$, ni mosaïque de Delaunay d'ordre supérieur. Pour $J$ selles et $A$ bras, $A\leq4J$; le graphe transitoire des descentes est partagé par clé complète puis projeté vers les seules liaisons bras--racine.

## Limites maintenues

- La façade de Phase 9 demeure une autorité fournie à la construction du journal.
- Une requête top-$K$ LBVH peut rester linéaire au pire cas.
- La fidélité globale des carriers paresseux à Gamma reste une obligation de preuve horizontale.
- Le locator utilise désormais un journal sparse de rollback, mais la profondeur DSU reste linéaire au pire cas.
- M.1, les morphismes verticaux et `full_pi0` restent hors de cette fermeture.
- Le SLO 50 000 points et la capacité 10 M+ ne sont pas revendiqués.

## GCP

GCP non utilisé.

# Différentiel borné des gateways directs — Phase 10.8-ORACLE

## Portée et statut validé

Phase 10, backend `reference_cpu_oracle`, profil `hgp_reduced`, mode `bounded_differential`, sémantique `oracle_only` et `public_status=not_claimed`.

Le jalon 10.8-ORACLE doit falsifier deux hypothèses distinctes : la suffisance de l'alphabet des facettes directes et la suffisance du générateur composé des selles directes avec les premières incidences 10.7. Il utilise Gamma exhaustif uniquement pour $3\leq n\leq14$ et $2\leq k\leq\min(10,n-1)$. Il n'entre dans aucun target produit et un succès borné ne devient jamais une preuve générale.

Le suivi est `validated_oracle_software` avec hypothèse `open_bounded_evidence_only`. Ce statut valide le falsificateur borné et son vérificateur frais; il ne valide pas la complétude générale du générateur.

## 1. Porte locale

La porte d'entrée globale de la Phase 10 reste satisfaite. La porte locale 10.8 est satisfaite par les faits suivants :

- 10.4 certifie toutes les suppressions de chaque selle directe fournie;
- 10.7 est `validated_host_software` et fournit toutes les premières cofaces minimisantes des clés directes;
- l'oracle Python possède déjà le catalogue critique exact, Gamma exhaustif, les coupes exactes ouvertes et fermées et la fixture permanente E5;
- le domaine est fermé sur `RelevantGP`; toute égalité extérieure ou support non générique retourne `unsupported_degeneracy`;
- les bornes $n\leq14$ et $k\leq10$ sont vérifiées avant toute énumération exhaustive;
- l'oracle reste sous `reference/` et `tests/oracle/`, sans dépendance du chemin produit vers Gamma.

L'implémentation a été menée sous cette porte sans modifier ni le statut de la Phase 10, ni sa porte de sortie.

## 2. Alphabet direct

À l'ordre $k$, soit $B_k$ l'ensemble des facettes de naissance du catalogue critique et soit $\mathcal{F}_{\mathrm{dir}}$ l'ensemble des suppressions des selles directes de rang $k+1$. Posons $V_k=B_k\cup\mathcal{F}_{\mathrm{dir}}$.

À chaque niveau exact $a$ et pour chaque coupe $\prec\in\lbrace <,\leq\rbrace$, la référence restreint chaque composante de Gamma aux facettes actives de $V_k$. Le candidat conserve seulement ces facettes et remplace chaque coface active $Q$ par l'hyperarête $e_Q\cap V_k$. Les deux partitions de $V_k$ doivent coïncider facette par facette.

Un échec `direct_alphabet_insufficient` fournit deux facettes de $V_k$ reliées dans Gamma mais séparées après restriction, un chemin Gamma canonique entre elles, la première facette intermédiaire hors $V_k$ et la première coface qui l'utilise. Cette expérience autorise toutes les cofaces Gamma : elle teste l'alphabet, pas encore le générateur sparse.

## 3. Générateur direct plus premières incidences

Pour chaque $F\in\mathcal{F}_{\mathrm{dir}}$, l'oracle indépendant définit $M(F)$ par énumération exacte de toutes les cofaces à un point. Le générateur retient :

- chaque coface de selle directe;
- chaque coface $F\cup\lbrace x\rbrace$ avec $x\in M(F)$;
- toutes les facettes de suppression de ces cofaces, reconstruites transitoirement.

À une coupe donnée, la référence est la partition de Gamma projetée sur les facettes actives de $V_k$. Le candidat est l'hypergraphe des seules cofaces générées actives, avec exactement toutes leurs facettes comme relais, puis sa partition est projetée sur $V_k$. Deux verdicts restent séparés :

- `facet_partition_equivalent`, qui exige les mêmes composantes de facettes pertinentes dans $V_k$;
- `root_genealogy_and_covered_points_equivalent`, qui exige la même suite de partitions de points couverts aux coupes ouvertes et fermées, et pas seulement le même état à la dernière coupe.

L'égalité des seules unions de points ne peut pas masquer une facette silencieuse pertinente absente. Pour une composante projetée sur $V_k$, la référence calcule les points couverts avec toute sa composante Gamma et le candidat avec toute sa composante de relais générés; une facette absente qui apporte un point nouveau est donc observable. Une coface ou une facette Gamma hors de $V_k$ peut néanmoins être omise si elle n'est qu'un relais redondant, n'apporte aucune couverture nouvelle et si aucune partition de $V_k$ ni généalogie de racines n'en dépend. La couverture de toutes les facettes non triviales de Gamma peut être publiée comme diagnostic séparé; elle ne pilote pas le verdict de suffisance parcimonieuse.

## 4. Lots égaux et témoins

Les niveaux sont les niveaux d'activation exacts de Gamma. Pour chaque niveau, la coupe stricte est vérifiée avant la coupe fermée; toutes les cofaces égales sont contractées atomiquement. Aucun ordre séquentiel interne au lot n'est une autorité.

Le premier témoin canonique conserve au minimum :

- le niveau exact et la frontière ouverte ou fermée;
- l'étage `direct_alphabet` ou `gateway_generator`;
- la première facette absente ou les deux facettes séparées à tort;
- les composantes de référence et candidates;
- un chemin Gamma alternant facettes et cofaces;
- la première coface non générée;
- la provenance `birth`, `direct`, `first_incidence` ou `residual` de chaque élément utile.

Les deux candidats sont construits comme sous-hypergraphes de Gamma. Le raffinement candidat de la partition de référence est donc vérifié comme invariant à chaque coupe; une connexion candidate plus précoce lève une contradiction interne au lieu de fabriquer un témoin Gamma impossible. Une connexion plus tardive classe séparément alphabet insuffisant, fermeture récursive nécessaire, co-minimiseur égal omis ou erreur de frontière.

## 5. Fixture E5 et ablation

La fixture `gabriel-point-set-counterexample-5-points-v1`, à l'ordre deux, est obligatoire. Sans ablation, les cofaces minimisantes `ACD` et `ACE` doivent attacher `AC` au niveau $33/2$, puis `ABC` doit rester une continuation à $83886/3563$.

Une ablation diagnostique de `ACD` et `ACE` doit provoquer le premier écart de facettes au niveau $33/2$ et un écart de généalogie au plus tard au niveau $83886/3563$. Ce test prouve la sensibilité du différentiel; il ne prouve pas la suffisance générale du générateur.

## 6. Bornes et structures évitées

Le coût exhaustif est limité par $\binom{n}{k}$ facettes et $\binom{n}{k+1}$ cofaces pour $n\leq14$. Les structures Gamma, catalogues globaux, historiques de coupes et chemins de témoins n'existent que dans l'oracle borné. Le chemin produit continue d'éviter :

- Gamma et tout catalogue global de facettes ou cofaces;
- les cellules top-$m$ et la mosaïque de Delaunay d'ordre supérieur;
- toute clé persistante de onze points;
- tout locator, quotient, DSU, racine, union, forêt ou `GatewayAttach` issu de l'oracle;
- toute dépendance de `morsehgp3d/` vers le nouveau module de référence.

La campagne courte validée couvre E5, huit cas déterministes d'ordre deux pour $5\leq n\leq8$, quatre cas d'ordre trois sur la même plage et la frontière $n=11$, $k=10$. Tout nouvel échec `RelevantGP` fait échouer le test avec ses coordonnées et son témoin afin de devenir une fixture permanente avant une optimisation du chemin produit. Aucun benchmark long, GPU ou GCP n'a été nécessaire.

## 7. Sortie du jalon

10.8-ORACLE est `validated_oracle_software`. Les 15 tests dédiés passent en 7,35 secondes lors du rejeu isolé : E5 nominal, ablation double sensible à $33/2$, ablation simple encore équivalente mais explicitement non probante, toutes les coupes ouvertes puis fermées, singletons actifs de $V_k$, ordre interne des cofaces inversé, préflights, `RelevantGP`, jointures catalogue--Gamma, vérificateur frais et campagnes $k=2$, $k=3$, $k=10$. Le vérificateur rejette notamment les substitutions `Enum`--chaîne, `Fraction`--entier, un objet à égalité hostile, un résultat incomplet et les mutations de checkpoints ou témoins. L'audit d'indépendance valide onze modules de référence et aucune dépendance produit vers ce différentiel.

Le résultat nominal E5 et les treize cas déterministes non ablatés ne montrent aucun écart. L'ablation de `ACD` et `ACE` sépare `AC` exactement à la coupe fermée $33/2$; la généalogie cumulative reste divergente après la reconvergence de la partition au niveau $24$. Une ablation est toujours marquée `diagnostic_ablation` et ne peut jamais publier `bounded_equivalent`, même si un seul des deux co-minimiseurs suffit encore.

Un contre-exemple futur fermera l'hypothèse correspondante et ouvrira un jalon de génération récursive ou d'élargissement de l'alphabet. La campagne actuelle sans écart laisse l'hypothèse `open_bounded_evidence_only`. La Phase 10 reste `in_progress` et `exit_gate_satisfied=false` tant qu'une preuve de génération complète n'existe pas.

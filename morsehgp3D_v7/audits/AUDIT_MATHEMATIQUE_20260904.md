# Audit mathématique courant v7

Date : 4 septembre 2026. Cadre : `phase=exploration_v7_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

**Verdict borné.** Le mode normalisé distingue correctement les facettes latentes des racines déjà incidentes sur les fixtures examinées. Le juge reconstruit les composantes depuis les seuls deltas et détecte les pertes de matérialisation, y compris une continuation sans nouveau point. La [composition horizontale sous fermeture de fenêtre](REPONSE_AUDITEUR_COMPOSITION.md) justifie le raccord de la descente au cœur et aux composantes, sans imposer une régularité géométrique globale ni un resolver top-K implicite. La [couverture S1](S1_COURANT.md) possède une preuve géométrique conditionnelle jusqu’au RLE. Les [lanes q2/q3/q4 et Cramer](ARITHMETIQUE_LANES_COURANTE.md), puis les [produits larges, comparateurs et réductions](ARITHMETIQUE_LARGE_COURANTE.md), ont désormais leurs preuves arithmétiques locales sous préconditions explicites. AxisBounds possède aussi une qualification indépendante ciblée de six portes. Les [portes C++ arithmétiques](AUDIT_QUALIFICATION_20260905.md), la [topologie de l’index](AUDIT_INDEX_20260905.md), son [raccord au front](AUDIT_RACCORD_INDEX_FRONT_20260905.md) et la [garde d’arrondi](AUDIT_ARRONDI_20260905.md) ont maintenant leurs qualifications propres. Les [bornes des témoins](ARITHMETIQUE_SPINDLE_COURANTE.md) sont complétées dans le présent audit ; le [domaine CPU](DOMAINE_CPU_COURANT.md), les frontières compilées et la verticale gardent leurs portées propres. Le lecteur de deltas contrôle les clefs canoniques et la consommation des seuls parents publiés. Les acquis logiciels restent bornés aux sources et fixtures épinglées.

## 1. Autorité des sources

Les parties I et II du [manuscrit](../../docs/references/MANUSCRIT_THESE_HAUSEUX.pdf), pages PDF 35–76 puis 77–134, ont été intégralement lues par plages textuelles contiguës. Les extractions initialement tronquées ont été relues en plages plus petites. SHA-256 du PDF : `579f83671ebca34cd810f350820074eb42672411713160f9c9c2a458ff4f4fef`.

Les exécutions de forêt ci-dessous sont attachées au [snapshot de 94 fichiers](receipts_20260904/math_current_source_snapshot.json), SHA-256 `d9ad8aced923759a826f040979b5d61f17aaa997169d190d18234e79087e9e05`. Les hashes avant copie, de la copie et immédiatement après copie y sont identiques. Les trois fichiers directement concernés sont épinglés ci-dessous. Les lectures arithmétiques ont leurs manifestes propres : [lanes](receipts_iteration3/lanes_static_current.json) et [produits larges/réductions](receipts_iteration3/wide_static_current.json). Le [snapshot AxisBounds et CLI](receipts_iteration3/axis_source.json) identifie les sources de leur qualification ciblée. Ces reçus séparent les octets effectivement exécutés des lectures statiques ; aucun snapshot n’étend tacitement un résultat à toute la source courante.

| Fichier | SHA-256 |
|---|---|
| `src/forest/fold.hpp` | `b11d02c86db5f8ae8cb12965f12e425548f9f049fb4626259790b32cd584928c` |
| `src/forest/silent_incidence.hpp` | `fddde6e233eea8e80d23af4d42b50952e7c49a50ed84357e973279ff14d555e8` |
| `tests/silent_incidence_gate.cpp` | `f78a984e577ad76f539acfc43ffeafe195e6dfd0b4eafbb8576462b765678198` |

Aucun code produit n'a été modifié par cet audit. Le contrat du lecteur d'archive est jugé séparément dans l'[audit des interfaces](AUDIT_INTERFACES_20260904.md).

## 2. Acquis vérifiés

Le tri de `CofaceKey` est borné physiquement à onze éléments et `event_key` contrôle les longueurs avant les copies. Le juge inclus dans la [fixture courante](math_followup_repro_20260904.cpp) compile avec GCC 13.3, C++20, `-O3 -Wall -Wextra -Wpedantic -Werror` : code 0, sans suppression d'avertissement. Cette compilation autonome n'est pas un résultat de la suite CTest complète.

Le fold reçoit `normalize_reduced` depuis le pipeline lorsque la complétion silencieuse est demandée. Au-dessus de l'ordre un, `seen(root)` distingue les racines déjà incidentes des facettes encore latentes. Dans ce mode, `born` signifie première matérialisation dans le sous-flot retenu, pas naissance géométrique exhaustive dans Gamma. Le triangle aigu suivant produit une naissance à zéro parent et trois facettes matérialisées, sans nœud de fusion :

$$A=(0,0,0),\quad B=(6,0,0),\quad C=(2,3,0),\qquad \beta(ABC)=\frac{325}{36}.$$

Ce niveau est vérifié contre l'oracle OBig. L'exception de l'ordre un, où les singletons restent des racines normatives, demeure explicite dans le fold.

Le nouveau `compare_delta_cuts` construit un état candidat depuis les seuls champs `parents` et `born`. Les cofaces candidates ne lui donnent aucune union. Chaque lot fige l'état antérieur avant consommation des parents; l'oracle Gamma ne sert qu'ensuite à comparer les coupes, les classes de cœur et la couverture. Cette vérification est exécutée en classic, en CSR et via le pipeline E5. Les deux suppressions ciblées de payload suivantes sont effectivement rejetées : les dix éléments `born` d'E5 effacés, et le seul delta matérialisant $AC$ au niveau $33/2$ retiré.

| Argument de la fixture courante | Code attendu et obtenu | Résultat |
|---|---:|---|
| `--gate-control` | 0 | 26 cas, 1492 coupes, 1543928 paires de cœur, 8 cofaces silencieuses, longueur maximale 2, un terminal en cache, 1354 coupes de deltas, 330 records, 371 transitions normalisées, aucun échec |
| `--triangle` | 0 | Zéro parent, trois `born`, zéro nœud à $325/36$ |
| `--erase-born` | 4 | Dix matérialisations retirées, six deltas conservés : rejet par le juge |
| `--drop-silent-continuation` | 4 | Un delta retiré à couverture de points constante : rejet par le juge |
| `--mutant=drop-nonmerge` | 4 | Mutant officiel tué |
| `--mutant=reduced-latent-parent` | 4 | Mutant officiel tué |
| `--mutant=reduced-drop-materialization` | 4 | Mutant officiel tué |
| `--mutant=csr-stale-level` | 4 | Niveau corrompu détecté |
| `--replace-output` | 4 | Sortie non canonique rejetée malgré une partition inchangée |

Le code 4 des modes de corruption de payload n'est rendu qu'après vérification de la corruption effective et du verdict attendu. Les [résultats bruts courants](receipts_20260904/math_current_repro.json) conservent tous les codes, les sorties et les hashes du binaire et de la fixture.

La [qualification indépendante d’AxisBounds](receipts_iteration3/axis_execution.json) construit en Release GCC 13.3 le CLI et sa porte : configuration, compilation et CTest rendent 0. Le nominal vérifie 1 212 boîtes, 454 697 points axiaux, 31 720 points volumiques et 45 requêtes de profondeur ; les cinq mutants rendent chacun 4 avec le préfixe de divergence exigé. Les six portes passent. Les sources et la copie sont stables ; le CLI reconstruit a le SHA-256 `25c9bf8e4ef3cded5647a22f16d81af7a1e778196ad3bff73884a7f58da985f2`. Ce reçu qualifie cette cible et sa construction, sans rejouer la suite complète ni mesurer un coût industriel.

## 3. Rejeu des identités et des matérialisations

`compare_delta_cuts` entretient un ensemble de racines effectivement publiées. Chaque lot fige cet ensemble avant de consommer ses parents; une racine ne peut être consommée deux fois dans le même lot. Le lecteur reconstruit les carriers à partir des seuls tokens de parents et des matérialisations `born`, puis exige pour chaque `output` leur plus petite `FacetKey`. Les unions de l'oracle Gamma ne suppléent pas une histoire de payload absente.

La fixture `--replace-output` conserve les six deltas E5, leurs parents, leurs dix éléments `born`, les niveaux et les compteurs. Elle remplace seulement la sortie initiale $CD$ par $CE$, une facette de la même composante. Le lecteur courant rejette cette corruption d'identité. La fixture vérifie aussi qu'un lecteur symbolique de sorties et parents seuls trouve exactement un parent orphelin dans cette copie corrompue, contre zéro au contrôle nominal.

La dent `--drop-silent-continuation` est distincte : elle supprime le seul delta matérialisant $AC$ à $33/2$, sans nouveau point couvert. Ce delta porte aussi le changement de clef canonique de $AD$ vers $AC$, utilisé par le lot suivant. Le rejet confirme que la porte ne se limite pas aux fusions ou aux augmentations de couverture. Enfin, `--erase-born` retire uniquement les dix matérialisations tout en conservant les six deltas et leurs parents; cette perte est également détectée.

Ces cas sont des contrôles permanents du payload, sous classic et CSR dans la porte du constructeur. Ils ferment les dents locales exercées; ils ne prouvent pas une complétude globale de Gamma ni tous les schémas d'export. Le validateur d'archive et ses reçus sont décrits séparément dans l'[audit des interfaces](AUDIT_INTERFACES_20260904.md).

## 4. Proposition utile — Pourquoi une seule première incidence peut suffire

Notons $\beta(F)$ le rayon carré de la miniball d'une facette $F$ de cardinal $K$ et $J_F$ l'ensemble des points strictement intérieurs à cette boule mais étrangers à $F$. Les points intérieurs appartenant déjà à $F$ ne sont pas des intrus. Supposons les supports minimaux essentiels uniques et l'absence d'extra-shell sur les objets concernés; la complétude du catalogue direct reste une prémisse distincte.

1. Si $\lvert J_F\rvert=0$, toute première coface incidente est directe. Sinon, le point ajouté serait essentiel; son remplacement par un intrus de cette coface diminuerait le niveau en conservant $F$, ce qui contredirait sa minimalité.
2. Si $J_F=\lbrace z\rbrace$, la première coface est uniquement $F\cup\lbrace z\rbrace$, directe, au niveau $\beta(F)$.
3. Si $\lvert J_F\rvert\geq2$, toutes les premières cofaces sont $Q_z=F\cup\lbrace z\rbrace$, avec $z\in J_F$, au niveau $\beta(F)$. Elles sont non-Gabriel. Pour deux intrus distincts $z,w$ et un point essentiel $u$ du support de $F$, la coface

$$R_{u,z,w}=(F\setminus\lbrace u\rbrace)\cup\lbrace z,w\rbrace$$

vérifie $\beta(R_{u,z,w})<\beta(F)$. Elle relie les facettes strictes $Q_z\setminus\lbrace u\rbrace$ et $Q_w\setminus\lbrace u\rbrace$ avant le lot. Les co-minimiseurs ont donc le même apex antérieur dans Gamma. Un seul représentant, attaché à cet apex **correctement résolu**, suffit pour installer $F$; il n'est pas nécessaire d'enregistrer tous ses co-minimiseurs pour cette seule opération de localisation.

Cela précise le passage entre le [corollaire 4.1 transverse](../../docs/math/INCIDENCES_SILENCIEUSES_GAMMA.md), formulé avec tous les co-minimiseurs, et [silent_incidence.hpp](../src/forest/silent_incidence.hpp), fonction `Builder::run`, qui choisit un intrus canonique. Pour une coface non-Gabriel $Q$, remplacer un élément essentiel $u$ par un intrus strict $w$ donne $R=(Q\setminus\lbrace u\rbrace)\cup\lbrace w\rbrace$, avec $\beta(R)<\beta(Q)$; la facette partagée $Q\setminus\lbrace u\rbrace$ certifie le passage à une composante antérieure. Une suite strictement décroissante dans l'ensemble fini des cofaces termine. Un terminal direct présent dans le catalogue, ou une chaîne déjà certifiée, donne alors un certificat de localisation.

Le cache de `Builder::run` n'est alimenté qu'après ce terminal, ce qui est le bon ordre. La comparaison exacte impose la décroissance. La présence réelle du terminal et son niveau sont contrôlés. Les limites de pas, de cofaces, de supports et de requêtes interrompent le calcul sans publier les ajouts partiels.

**Réduction supplémentaire justifiée.** `Builder::run` n'énumère que les facettes obtenues en retirant un point essentiel d'une coface directe. Une facette obtenue en retirant un point intérieur conserve la même miniball; son seul intrus étranger est précisément ce point retiré, puisque la coface est directe et sans extra-shell. Elle est donc déjà dans le cas 2. Son exclusion de la liste à compléter est légitime sous ces prémisses.

**Composition et limite de portée.** L'identification de l'apex ci-dessus est une affirmation sur Gamma. Le raccord au sous-flot utilise la fermeture effective des chaînes par suffixes décroissants jusqu'à une coface directe; chaque composante retenue possède ainsi un ancrage du cœur déjà actif. La [contre-lecture de composition](REPONSE_AUDITEUR_COMPOSITION.md) construit ensuite la bijection par inclusion des facettes et les classes du cœur. La régularité vérifiée de chaque facette du cœur, jointe à l'inertie des blocs saturés hors fenêtre, exclut une première incidence cachée dans une boule irrégulière omise. Le [retour mathématique courant](RETOUR_MATH_COURANT.md) détaille ce lemme de contact et son test à la frontière de rang onze/douze. La régularité géométrique globale n'est donc pas une prémisse nécessaire de cette route. La composition demeure conditionnelle à S1–S4, à l'arithmétique et aux contrôles locaux exacts; la couverture S1 possède sa preuve conditionnelle séparée, sans être déduite des seules campagnes. Aucune promotion de `public_status` n'en découle.

## 5. Oracle et structures évitées

Le juge de `silent_incidence_gate.cpp` résout les centres par Gram/Cramer dans `OBig<20>`, sans appeler les prédicats q2/q3/q4 pour décider leur géométrie. C'est une séparation pertinente. Une borne grossière explicite suffit pour ses comparaisons de niveaux sous u16 : les différences de coordonnées ont magnitude inférieure à $2^{16}$; chaque entrée de Gram a magnitude inférieure à $2^{34}$; un déterminant de taille au plus trois, développé en au plus six termes, a magnitude inférieure à $2^{105}$. Donc le dénominateur du centre est inférieur à $2^{106}$, chaque coordonnée relative du numérateur inférieure à $2^{123}$, le numérateur du rayon carré inférieur à $2^{248}$ et son dénominateur inférieur à $2^{212}$. Les produits croisés de deux niveaux sont inférieurs à $2^{460}$, dans les 640 bits alloués. Cette borne concerne les expressions de ce juge, pas tous les appels possibles à OBig.

`oracle/obig.hpp` conserve un drapeau de débordement collant et le juge le vérifie avant de rendre son verdict. La diversification d'arithmétique réduit les risques de faute commune; elle ne suffit pas à les exclure logiquement. Les essais ci-dessus n'ont observé aucun débordement.

Le constructeur de chaînes évite Gamma exhaustif, son catalogue global de facettes et la mosaïque de Delaunay. Chaque miniball locale porte sur au plus onze sites; la borne exacte d'énumération est $\binom{11}{2}+\binom{11}{3}+\binom{11}{4}=550$ supports. Le coût total dépend cependant du nombre de facettes du cœur et de la longueur des chaînes; aucune borne pratique globale ne découle de 550. Les compteurs déjà présents permettent de mesurer séparément ce travail avant toute qualification industrielle.

## 6. Reproduction et prochain jalon

La fixture maintenue est [math_followup_repro_20260904.cpp](math_followup_repro_20260904.cpp); ses corruptions ne modifient que des copies du résultat. Elle inclut le juge OBig existant : ce n'est pas un second oracle arithmétique indépendant. Le snapshot de compilation réside sous `audits/.work_math/final/`.

```bash
mkdir -p morsehgp3D_v7/audits/.work_math/final/tmp
TMPDIR="$PWD/morsehgp3D_v7/audits/.work_math/final/tmp" g++ -std=c++20 -O3 -Wall -Wextra -Wpedantic -Werror -DMHGP7_TESTING=1 '-DMHGP7_AUDIT_GATE_SOURCE=".work_math/final/source/morsehgp3D_v7/tests/silent_incidence_gate.cpp"' -pthread morsehgp3D_v7/audits/math_followup_repro_20260904.cpp -o morsehgp3D_v7/audits/.work_math/final/math_repro
morsehgp3D_v7/audits/.work_math/final/math_repro --gate-control
morsehgp3D_v7/audits/.work_math/final/math_repro --triangle
morsehgp3D_v7/audits/.work_math/final/math_repro --erase-born
morsehgp3D_v7/audits/.work_math/final/math_repro --drop-silent-continuation
morsehgp3D_v7/audits/.work_math/final/math_repro --replace-output
```

Les trois dernières commandes doivent rendre 4. Sans le snapshot temporaire, omettre `MHGP7_AUDIT_GATE_SOURCE` compile les sources v7 courantes; vérifier leurs hashes avant d'attribuer les résultats à ce rapport.

La [fixture de fenêtre](math_window_repro_20260904.cpp) ajoute un contrôle indépendant du catalogue par supports q2/q3/q4 jusqu'à 24 points, de son expansion sur K=1 à 10, puis du vrai pipeline et des deltas à K=2 et K=10 sur des cas à 13 points. Elle distingue une extra-shell pertinente de rang onze, reçue puis refusée, d'une extra-shell de rang douze dont l'omission est conforme aux coupes examinées. Les résultats et les deux mutants ciblés sont détaillés dans le [retour courant](RETOUR_MATH_COURANT.md).

Les petites portes C++ causales sont portées et leurs résultats reliés aux binaires dans la [contrelecture de qualification](AUDIT_QUALIFICATION_20260905.md). Les preuves de Cramer, des largeurs des lanes, des colonnes U192/U320, du PGCD et des réductions sont fermées statiquement sous leurs préconditions ; elles ne sont plus des demandes génériques de preuve. Les fixtures intégrées ciblent notamment le centre q3 supérieur à $2^{40}$ issu d’un vrai triangle u16 et les deux sites distincts de `level-trunc-hi`, dont le cinquième mot U320 n’est pas atteint par les niveaux géométriques u16. Les entrées interdites restent rejetées par le validateur de fixture lorsque le helper ne fournit pas lui-même ce statut.

La [matrice S1](S1_COURANT.md) compose propriétaires, seeds, covers, prunes et RLE ; aucune de ces clauses géométriques ne reste une réserve ouverte dans cette preuve conditionnelle. Les invariants d’index et les parcours du front/cover sont désormais raccordés dans les notes liées en tête. Les bornes arithmétiques des témoins sont désormais fermées dans les compléments fuseaux/secteurs/cordes/cellules. Leur raccord aux frontières compilées est désormais exécuté en O2/UBSan et la qualification intégrée E possède ses reçus propres. Le certificat horizontal réduit, puis la verticale et ses carrés de naturalité gardent leurs obligations propres. Les distributions de longueur des chaînes et leur travail spatial sous budgets sont des mesures de coût, pas des certificats de complétude.

Les vérifications rapportées sont locales CPU et bornées. Aucun benchmark industriel, résultat GPU, preuve de verticale ou statut exact global n'est acquis par cette note. **GCP non utilisé.**

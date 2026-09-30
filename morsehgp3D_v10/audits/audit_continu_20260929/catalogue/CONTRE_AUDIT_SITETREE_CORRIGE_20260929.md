# Contre-audit de SiteTree corrigé — 29 septembre 2026

Le correctif cible correctement G1 : le filtre est borné au cube u18 par une garde rationnelle sans débordement ; les centres lointains utilisent les deux replis exacts. Aucun nouveau désaccord géométrique n'a été trouvé dans cette passe. Le verrou restant utile est **le contrat d'arrondi et sa gate** : la preuve publiée suppose FE_TONEAREST, tandis que la frontière publique ne vérifie pas ce mode ; le plancher de la gate existante dépend lui-même de l'arrondi.

`phase=exploration_v10_hors_registre`, `backend=cpu_reference`, `profile=quantized_u18_input_only`, `mode=independent_sitetree_numeric_counteraudit`, `public_status=not_claimed`. Copie `build/v10-fixes/sitetree/src/morsehgp3D_v10`, base déclarée `0bce6cc00`. Aucun moteur, index, source d'un autre acteur ou Git modifié. GCP non utilisé.

## État des preuves réellement observées

À la première lecture, `RECU_sitetree.md` §5 portait encore **EN_ATTENTE_PREUVES**. Le développeur a complété cette section pendant la passe ; la copie finale du reçu, empreinte `a9c2993a26567528d0d37e7a129176811d33b4c1916d09125c42b6fbf391894c`, est archivée. Les journaux déjà terminés portent 10/10 CTests, code 0, 1 706,22 s ; puis 2/2 sur unité/G1, code 0. Les journaux ASan/UBSan et TSan dédiés finissent avec code 0. Les différentiels tour et tête relus finissent par 0 écart sur 7 et sur 18.

Ces résultats du développeur sont **observés**, non rejoués par cette passe. La nouvelle vérification indépendante ci-dessous est une compilation de petit harnais liée à la bibliothèque existante ; elle ne qualifie pas l'assemblage des autres correctifs ou une campagne produit. La bibliothèque `libmhgp10_core.a`, le binaire de gate et les 29 entrées de sources/compilation mesurées sont inchangés avant/après.

| Objet mesuré | SHA-256 |
| --- | --- |
| `src/cloud/site_tree.cpp` | `d83e999bf626c4e8233517e9fe36ccdfef50ec2c319b3610b0814f9d2fa09e24` |
| `src/cloud/site_tree.hpp` | `362bf31d94a743fb18f578d4ddbba05fe2e2287e00cc636d85565fbfe75a9d5d` |
| Gate source | `38157638361c662ca38e8b871dea90ff1706ddb7e70d1b8f30a198fe134121bf` |
| Bibliothèque liée | `a080fdd7d24c8e9d4711d240b125ca40190319f5848caae1cbd37bba3298b171` |
| Nouveau harnais | `3a5d8eebc3358388e147f14caa1f79901c5dbcfb4ba5d7b9b85343a237fb901b` |

## Domaine et replis exacts

La garde teste d'abord `0 < D ≤ 2^82`, ancre dans [0,L], `|N_i| ≤ 2^100`, puis `0 ≤ D*a_i+N_i ≤ D*L`, L=2^18−1. Ses produits sont <2^100, sa somme <2^101 : elle reste sûre pour **tout** i128 N/D, y compris les extrêmes refusés avant multiplication. Les coordonnées Cloud sont des u32 ; la boîte racine suffit donc à vérifier leur borne supérieure, sans borne inférieure négative à traiter.

Cette garde choisit un chemin, **ce n'est pas un validateur des requêtes**. Si elle rend faux pour un Center forgé hors représentation, le repli appelle encore `side_key`. Pour sites/ancre u18 et bornes de représentation, `D*Σdx² ≤ 3*2^118` et `|2ΣN_i*dx_i| ≤ 6*2^118`, donc la clé est <2^122. Les fabriques q2/q3/q4 respectent ces bornes. Un Centre arbitraire hors de ces bornes ne reçoit pas cette garantie ; notre sonde ne l'appelle qu'avec `filtered`, jamais avec les requêtes géométriques.

`closed_ball` parcourt alors tous les sites, classe par signe exact et émet dans l'ordre d'indice : O(n), mémoire de sortie payée. `nearest` insère les couples exacts (clé, indice) en conservant au plus count≤64 éléments : O(n*count), donc O(n) avec le plafond publié, mémoire ≤64. Les égalités sont départagées par indice. Le nuage 21 bits des tests ne qualifie que des centres représentables de site/milieu ; il n'étend pas les fabriques q3/q4 u18 à 21 bits.

L'index emprunte Cloud et conserve des coordonnées/boîtes copiées et un drapeau de domaine au constructeur. Sa lecture suppose aussi que Cloud survive et ne soit pas muté pendant l'usage de l'index. Cela ne prouve pas un défaut dans les appelants produits, qui emploient leur préparation stable.

## Marge flottante et angles morts d'arrondi

Le domaine géométrique fournit `|N_i/D| ≤ L`. Avec conversion/division/arithmétique correctement arrondies et ε=2^-53, le calcul du commentaire donne une erreur de centre ≤4,02εL, puis `E=γ_5*3*(L+δ)²+3δ*(2L+δ)`. Un calcul **Fraction indépendant** confirme E≈0,000298459637. Nous ajoutons aussi une réserve 2^-14 pour l'arrondi des seuils `r2a±0,02` ou `W+0,02` ; `2E+2^-14≈0,000657954431<0,02`.

Pour les trois modes dirigés, le même modèle avec ε=2^-52 donne E≈0,000596919274 et `2E+2^-14≈0,001254873704<0,02`. La marge permet donc une extension conservatrice de la preuve, **sous l'hypothèse explicitée de borne de conversion i128→double dans ces modes**. Ce calcul de constantes ne certifie ni toutes les conversions, ni toutes les options de compilation. Les flags du binaire relu sont `-O3 -DNDEBUG -std=c++20 -Wall -Wextra -Wpedantic -Werror` ; la preuve reste attachée aux opérations IEEE ordinaires, pas à des réassociations arbitraires.

La distance à une boîte peut également être bornée par E : son point de projection a des coordonnées prises aux bornes entières de la boîte pour les axes actifs. Ainsi l'élagage peut se prouver par une borne continue exacte plus E, sans étendre implicitement un raisonnement de monotonie au-delà de ses hypothèses d'arrondi. Aucun faux élagage dirigé n'est établi par cette lecture.

## Vérification indépendante courte et terminal réel

Le [harnais](../../../receipts/audit_continu_20260929/sitetree_corrected/rounding_probe.cpp) appelle la gate existante sous quatre modes, dans le même processus, et ajoute 28 contrôles analytiques par mode : nuage collinéaire de 128 sites, centre milieu filtré, centre de triangle obtus en repli, coquilles exactes, count=0/1/64/65/127/128 et extrêmes i128/i64 passés seulement à `filtered`. Les 112 contrôles supplémentaires passent ; chacun des quatre modes est conservé à la sortie des requêtes.

| Mode | Désaccord géométrique observé | Anciennes erreurs adverses | Code de la gate réutilisée |
| --- | ---: | ---: | ---: |
| FE_TONEAREST | 0 | 505 | 0 |
| FE_UPWARD | 0 | 259 | 3 |
| FE_DOWNWARD | 0 | 271 | 3 |
| FE_TOWARDZERO | 0 | 265 | 3 |

**Le terminal du nouveau harnais est code 1**, conservé avec les sorties brutes. Les codes 3 sont les trois planchers `adverses≥300` non atteints ; ce compteur mesure les erreurs de l'ancien filtre sous le mode courant. La source de la gate vérifie les désaccords avant son plancher : elle ne produit ce code 3 qu'après zéro désaccord. Nous ne transformons donc pas ce harnais en gate verte de quatre arrondis et ne relançons pas avec un plancher réduit pour produire un PASS favorable. La fixture G1 corrigée garde ses quatre sites de coquille dans les quatre modes.

Le [reçu](../../../receipts/audit_continu_20260929/sitetree_corrected/receipt.json) fixe commandes, codes réels, sorties, compilateur et empreintes avant/après ; fichiers natifs sélectionnés et journaux observés sont archivés. La compilation ne reconstruit aucun moteur. La mesure dépend encore de la bibliothèque locale liée : l'archive de sources/reçus n'est pas une archive autonome du binaire.

## Centres MEB, coûts et verrou développeur

L'argument produit est géométriquement correct : une MEB a son centre dans l'enveloppe convexe de son support, donc dans le cube des sites. La lecture de `verify_meb` contrôle milieu/triangle non obtus et les quatre orientations fermées avant la distance approchée ; le repli Welzl exact fournit la MEB. Les entrées core emploient un centre de site. Sous ces certificats préexistants et le domaine u18, ces appelants restent filtrés. Ce patch ne requalifie pas le certificateur MEB ni les filtres propres de la tour ; l'absence de repli dans les campagnes instrumentées est une observation sur ces appels.

Le repli O(n) n'implique aucune borne sous-linéaire pour les requêtes filtrées ou globale pour la tour. `nearest` peut garder puis trier Θ(n) candidats, et `closed_ball` trie ses sorties : O(n log n) demeure possible sur le chemin filtré. Le nombre de requêtes et de candidats n'est pas borné par cette correction de domaine.

**Verrou proposé :** soit publier et contrôler FE_TONEAREST à la frontière commune, soit compléter la preuve pour les quatre modes (conversion incluse) et définir avant exécution les planchers adverses par mode, avec un plancher géométrique positif distinct. Les préconditions de représentation et d'immuabilité restent explicites ; un `filtered=false` ne doit jamais être présenté comme un refus sûr d'une requête forgée. Aucun défaut sur les chemins produits ou contrat de tour/performance n'est inféré de ces angles morts.

# Noyau mathématique de MorseHGP3D

Les six documents de ce dossier forment un ensemble normatif.

| document | question résolue |
|---|---|
| [Définition HGP 3D](DEFINITION_HGP_3D.md) | quelle hiérarchie doit être calculée et quelle partie Gabriel garantit-elle directement ? |
| [Catalogue critique 3D](CATALOGUE_CRITIQUE_3D.md) | quels événements suffisent jusqu'à $K_{\max}=10$ et comment les énumérer sans mosaïque globale ? |
| [Attaches par miniball](ATTACHES_DESCENTE_MINIBALL.md) | comment rattacher un bras connu à sa composante globale ? |
| [Incidences silencieuses](INCIDENCES_SILENCIEUSES_GAMMA.md) | pourquoi les cinq points réfutent-ils le flot brut et quelle complétion rétablit-elle Gamma ? |
| [Tour globale de boules saturées](TOUR_BOULES_SATUREES.md) | comment représenter exactement toutes les incidences par saturés, et pourquoi la voie brute reste-t-elle non scalable ? |
| [Preuves et heuristiques](STATUT_PREUVES_ET_HEURISTIQUES.md) | quelles affirmations sont démontrées, conditionnelles, ouvertes ou fausses en général ? |

La note de déploiement [Premières incidences des facettes directes réutilisées](PREMIERES_INCIDENCES_FACETTES_DIRECTES_PHASE10.md) fixe le contrat borné 10.7-RCPU. Elle complète les notes de Phase 10 sans rejoindre le noyau normatif ci-dessus et sans prétendre que les seules facettes issues des selles directes forment un ensemble globalement complet de gateways.

Le [différentiel borné des gateways directs](DIFFERENTIEL_GATEWAYS_DIRECTS_PHASE10.md) valide le falsificateur 10.8-ORACLE sous statut `open_bounded_evidence_only` : Gamma y reste transitoire, limité à $n\leq14$ et totalement exclu du chemin produit.

## Résumé de la construction

Posons $K_{\mathrm{eff}}=\min(K_{\max},n)$, $s_{\max}=\min(K_{\mathrm{eff}}+1,n)$ et, si $s_{\max}\geq2$, $m_{\star}=s_{\max}-2$. Alors :

1. injecter directement les événements ponctuels de rang un; si $s_{\max}=1$, arrêter la cascade géométrique;
2. parcourir directement les supports de tailles deux à quatre par produits de nœuds LBVH, bornes exactes et frontières budgetées, sans fermer les parents top-$m$;
3. terminer exactement le shell et le rang fermé global des supports feuilles survivants; garder le raffinement cellulaire seulement comme oracle borné;
4. réutiliser chaque sphère de rang $s$ comme minimum de $D_s$ et point d'indice un de $D_{s-1}$, avec multiplicité locale $\binom{\lvert U\rvert-1}{\mu}$;
5. convertir les événements de rang $k+1$ en hyperarêtes multifurquées du K-graphe de Gabriel, utilisables comme sous-flot positif;
6. pour `hgp_reduced`, réduire le flux en journal Morse par lots de niveau exact, conserver les gateways silencieux et garder les feuilles singleton explicites à $k=1$; Gamma exhaustif reste l'oracle de petits nuages;
7. pour `full_pi0`, attacher tous les bras par descentes certifiées; cette reconstruction reste l'obligation de preuve M.1 et ne publie pas encore le statut `exact`;
8. construire les morphismes réduits par `locate_reduced_root`, utiliser les ancres naissance–selle seulement pour $2\leq s\leq K_{\mathrm{eff}}$ dans `full_pi0` ou comme contrôle lorsqu'une source réduite existe, puis vérifier toute la naturalité ordre–échelle.

## Frontière exacte

Le flot de Gabriel brut ne préserve pas toujours les K-polyèdres non triviaux : une coface non-Gabriel peut attacher silencieusement une facette réutilisée plus tard. Le chemin exact v2 réduit Gamma exhaustif; la [complétion en incidences silencieuses](INCIDENCES_SILENCIEUSES_GAMMA.md) explique le défaut et prouve une correction combinatoire lorsque toutes les cofaces sont connues. Le profil complet exige en plus un argument de Morse et des attaches globales.

Le flux direct LBVH est la primitive produit GPU. Sa complétude vient de la partition exacte des produits de supports, des prunes rejoués, des classifications feuilles globales et des frontières vides. Voronoï et puissance servent aux propositions ou aux tests; Geogram est préféré lorsqu'une baseline CPU suffit. DTM, entropie et ANN sont des propositions seulement.

## Convention de lecture

Les équations utilisent des rayons carrés. Un « rang » est le nombre de points dans la boule fermée. Un « support » est le sous-ensemble minimal de points frontière dont l'enveloppe convexe relative contient le centre. Un « lot » regroupe tous les événements d'un même niveau exact.

Le [manuscrit](../references/MANUSCRIT_THESE_HAUSEUX.pdf) et le [corpus bibliographique](../references/README.md) restent les sources primaires. La [spécification générale](../SPECIFICATION_MORSEHGP3D.md) fixe les schémas publics et les statuts d'exécution.

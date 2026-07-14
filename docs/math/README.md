# Noyau mathématique de MorseHGP3D

Les quatre documents de ce dossier forment un ensemble normatif.

| document | question résolue |
|---|---|
| [Définition HGP 3D](DEFINITION_HGP_3D.md) | quelle hiérarchie doit être calculée et quelle partie Gabriel garantit-elle directement ? |
| [Catalogue critique 3D](CATALOGUE_CRITIQUE_3D.md) | quels événements suffisent jusqu'à $K_{\max}=10$ et comment les énumérer sans mosaïque globale ? |
| [Attaches par miniball](ATTACHES_DESCENTE_MINIBALL.md) | comment rattacher un bras connu à sa composante globale ? |
| [Preuves et heuristiques](STATUT_PREUVES_ET_HEURISTIQUES.md) | quelles affirmations sont démontrées, conditionnelles, ouvertes ou fausses en général ? |

## Résumé de la construction

Posons $K_{\mathrm{eff}}=\min(K_{\max},n)$, $s_{\max}=\min(K_{\mathrm{eff}}+1,n)$ et, si $s_{\max}\geq2$, $m_{\star}=s_{\max}-2$. Alors :

1. injecter directement les événements ponctuels de rang un; si $s_{\max}=1$, arrêter la cascade géométrique;
2. fermer les parents top-$m$ pour $0\leq m\leq m_{\star}$ par raffinements restreints et reconstruire canoniquement chaque enfant avant propagation;
3. extraire les supports bien centrés de tailles deux à quatre, terminer leur shell et compter leur rang fermé global;
4. réutiliser chaque sphère de rang $s$ comme minimum de $D_s$ et point d'indice un de $D_{s-1}$, avec multiplicité locale $\binom{\lvert U\rvert-1}{\mu}$;
5. convertir les événements de rang $k+1$ en hyperarêtes multifurquées du K-graphe de Gabriel;
6. réduire les hyperarêtes par lots de niveau exact pour obtenir `hgp_reduced`, avec feuilles singleton explicites à $k=1$;
7. pour `full_pi0`, attacher tous les bras par descentes certifiées; cette reconstruction reste l'obligation de preuve M.1 et ne publie pas encore le statut `exact`;
8. construire les morphismes réduits par `locate_reduced_root`, utiliser les ancres naissance–selle seulement pour $2\leq s\leq K_{\mathrm{eff}}$ dans `full_pi0` ou comme contrôle lorsqu'une source réduite existe, puis vérifier toute la naturalité ordre–échelle.

## Frontière exacte

La réduction de Gabriel est un théorème pour les K-polyèdres non triviaux. Elle ne doit pas être présentée comme une preuve automatique de la généalogie des facettes isolées. Le profil complet exige un argument de Morse et des attaches globales.

La primitive GPU de diagramme de puissance est un accélérateur géométrique. La complétude vient de la fermeture par oracle global, de la reconstruction canonique des enfants et des prédicats exacts. Une simple déduplication des fragments ne certifie pas leurs coutures. DTM, entropie et ANN sont des propositions seulement.

## Convention de lecture

Les équations utilisent des rayons carrés. Un « rang » est le nombre de points dans la boule fermée. Un « support » est le sous-ensemble minimal de points frontière dont l'enveloppe convexe relative contient le centre. Un « lot » regroupe tous les événements d'un même niveau exact.

Le [manuscrit](../references/MANUSCRIT_THESE_HAUSEUX.pdf) et le [corpus bibliographique](../references/README.md) restent les sources primaires. La [spécification générale](../SPECIFICATION_MORSEHGP3D.md) fixe les schémas publics et les statuts d'exécution.

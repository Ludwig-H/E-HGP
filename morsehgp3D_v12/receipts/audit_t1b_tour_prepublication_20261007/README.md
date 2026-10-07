# Catalogue et tour avant publication — 7 octobre 2026

Contrelecture de deux prototypes locaux, pendant le développement, main publié `9428db65b`.
Les hashes de chaque capture font autorité ; aucun de ces prototypes n'est encore le chemin produit publié.
Pas de nouveau chrono, de build natif ou de GPU/GCP dans ce lot. Cadre v12 hors registre, FULL pi0, u21,
`public_status=not_claimed`.

- [CUDA et ressources](cuda/README.md) : nettoyer les allocations sur erreur et compléter le compteur des
  réécritures ; portée hybride et frontière du temps précisées. Constats statiques, sans panne CUDA injectée.
- [Finition commune](finition/README.md) : traduction mathématique des clés, chaînes exactes, rangs et CSR
  contre-lue ; coûts et qualification native/appareil encore à juger. Réponse en cours à CST-0233.
- [Tour T/M/V et registre](tour/README.md) : plateaux et verticales cohérents à lecture ; modèle abstrait de deux
  hypergraphes prouvant la perte des branches ouvertes dans la première forme de R. Compléter R ou déclarer
  son report à T3 ; aucun défaut de pi0 établi.

Ces retours anticipent la livraison. Le registre courant reste l'autorité des états publiés ; aucune clôture
ni nouveau constat produit n'est déduit automatiquement d'une copie locale. Les deux copies du socle devront
intégrer le correctif cache avant de transférer leurs résultats de test à la chaîne livrée.

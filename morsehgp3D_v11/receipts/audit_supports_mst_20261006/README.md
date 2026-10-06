# Supports du MST : deux corrections avant publication de SPv2

WIP développeur au-dessus de `9eee2ed4bcef1e960cdf2456012b84416854dc20`,
relu le 6 octobre 2026 vers 10:10 UTC. Les sources effectivement lues sont
épinglées dans chaque sous-reçu. La décision de publier S* seul pour les
supports associés au MST est acceptée.

- [Sélection Kruskal](mathematics/REPORT.md) : trois points entiers
  équidistants à K1 donnent trois boules `merge` au même plateau. Le filtre
  de rôle conserve le cycle ; une DSU locale sélectionne deux supports.
  Trente contrôles exacts Fraction/DSU, normal et `-O` identiques.
- [Versions du dossier](formats/README.md) : le lecteur WIP accepte un
  manifeste v2 accompagné d'un binaire v1 valide de 200 octets. Exiger
  l'égalité des versions. La rétrolecture complète des dossiers v1 est
  une décision distincte. Reproduction Python normale et `-O` identique.

Les replays vérifient leurs captures et utilisent des objets Git locaux
épinglés ; le dépôt reste nécessaire. Aucun build, produit natif, donnée
LiDAR ou appel GCP. Ces preuves démontrent les défauts du WIP capturé ;
elles ne qualifient aucun futur correctif ni ses performances.

Les notes actives restent dans `audits/`, sans nouveau fil de dialogue.

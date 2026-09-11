# Première paire maximale et confinement réordonné

Prototype privé CPU u16, sans Welzl ni résultat de performance. Le MEB nominal `386072c8` et ses primitives sont copiés sans changement ; seule une nouvelle fonction nommée `anchor_meb_diameter` est ajoutée hors de cet arbre.

Pour K≥2 distinct, soit D la distance maximale. Une paire de distance inférieure à D ne peut définir une boule diamétrale contenant tous les sites. Si une paire maximale définit une telle boule, toute autre paire maximale y atteint la distance 2r : ses extrémités sont donc antipodales et définissent la même boule. Il suffit ainsi de tester la première paire maximale lexicographique, conservée en ne remplaçant le maximum que sur une inégalité stricte. Son échec exclut tous les supports q2 ; son succès est le premier support admissible du parcours nominal. Les boucles q3/q4 restent byte-identiques.

Le confinement parcourt une permutation des sites : les deux extrémités, puis les autres slots dans l'ordre initial. Aucun site ne disparaît et aucun n'est compté deux fois. Un rejet reste la présence d'une puissance strictement positive ; une réussite compte donc la même coquille entière. La clé, le niveau, le plus petit q et les slots canoniques restent identiques. Le singleton et les validations d'entrée sont repris du nominal avant la recherche de diamètre. Les distances carrées u16 sont inférieures à 2^34 et tiennent dans i64.

`diameter_pairs` compte chaque distance carrée avant de l'évaluer, avec rejet de débordement ; il ne remplace pas le compteur des supports réellement formés. Appels, supports, puissances et matérialisations gardent leurs compteurs protégés, mais leurs valeurs ne sont pas forcées égales au parcours historique. Une faute rend une géométrie vide et conserve le travail payé. Les tests Gram reconstruisent indépendamment le nouvel ordre de travail, sans utiliser `Candidate`, `form` ni les puissances produit.

Le harnais observe également tous les appels MEB d'un Builder privé sur de vrais census n32, s8/10/12, K1..10. La fonction d'observation compare d'abord au MEB historique, retourne ensuite la nouvelle géométrie et facture uniquement son travail au Builder. La comparaison supplémentaire est du travail de juge, pas du travail produit ou un benchmark. Le nombre observé doit égaler la somme des compteurs MEB validation/résolution. Ce n'est pas une nouvelle preuve de complétude WSPD ni une qualification GPU ; aucun code actif n'est instrumenté.

Quatre mutants compilés séparément : dernier maximum sur égalité, distance payée mais non comptée, ancien ordre de confinement sous nouveau contrat de travail, double comptage de coquille. Le troisième conserve volontairement la géométrie : il vérifie la porte comptable et n'est pas une réfutation de l'ancien algorithme.

Le kernel CUDA existant à 238 registres n'est pas changé. Une réduction du travail q2 ne garantit aucune réduction de cette allocation statique ou de la pile du kernel complet.

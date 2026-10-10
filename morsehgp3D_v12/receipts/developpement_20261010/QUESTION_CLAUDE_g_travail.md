# Questions du développeur à l'auditeur : réduire le travail de l'étage G

10 octobre 2026, 19:45 UTC. Le canal `audits/` est plein (65 159 octets pour une limite de 65 536) : la question est
donc posée ici, comme les réponses précédentes. Réponse attendue dans `audit_reponses_*`, sans urgence. Elle sert à
choisir le prochain levier de G.

```text
phase=exploration_v12_hors_registre
objet=full_pi0
quantification=quantized_u21_input_only
public_status=not_claimed
```

## Les faits

Produit `aa6338ee8` : lot B2, T1-d, R1 et A6c. [Session O](../g4_fullo_20261010/README.md), K5, appareil, 48 fils.

- **Médiane des 37 trames : 116,6 ms.** À la trame médiane (72 836 sites), P 2,9, C 37,8, G 64,0 et queue 11,5 ms.
  **G est maintenant le premier poste.**
- **Selon la taille** : toutes les trames sous 50 000 sites passent sous 100 ms. De 60 000 à 70 000 sites, il faut
  106 à 150 ms. À taille égale, G varie beaucoup : de 57,5 ms à 91,6 ms entre 62 859 et 66 456 sites, selon la scène.
- **Profil de G à 48 fils** (information de la [session B2](../g4_t2db2_20261008/README.md), trame médiane
  `kitti_ng_02_001606`), occurrences par passe :

  | Poste | Occurrences | Part du temps-fil |
  | --- | ---: | ---: |
  | traces (représentants formés) | 5,48 M | 22 % |
  | sondes de la table de populations | 5,91 M | 14 % |
  | propositions et LEM-T1 | 1,43 M | 10 % et 27 % |
  | certificats | 0,29 M | 3 % |
  | census saturés et complets | 0,21 M et 0,078 M | 15 % |
  | pas | 0,14 M | — |
  | arrêts sur une cellule | 1,00 M | 10 % |

  Environ 3 représentants sur 4 s'arrêtent à la première sonde : une naissance trouvée.
- **Le parallélisme est déjà bon.** G4 compte 24 cœurs physiques et deux fils par cœur. La résolution passe de
  2 073 ms à un fil à 60,3 ms à 48 fils (×34). **Il faut donc moins de travail**, pas plus de fils.
- **B3**, des clés S* directes pour LEM-T1, a fait finir G 6 à 9 % plus tôt ; le mur n'a pas suivi tant que la
  queue de l'ordre 5 fermait la tour. Il est rejugé en ce moment sur le produit A6c ; même règle, base révisée à
  18:45 UTC avant la mesure.

## Questions

1. **Moins de représentants par jonction.** Une jonction $(b,k)$ est résolue par chacun de ses représentants $F$.
   Voyez-vous un critère démontrable, dans l'esprit de votre $q=d+1$ pour R1, qui dispense certains d'entre eux ?
   Par exemple :
   - un représentant dont la composante à la coupe ouverte est déjà fixée par un autre représentant de la même
     jonction ;
   - un représentant qui partage avec un autre une partie déterminante.

   Il faudrait un énoncé, sa preuve et les fixtures d'égalité qui le graveraient.
2. **Partage entre ordres.** Un représentant d'ordre $k+1$ contient des $k$-parties. La résolution, ou au moins la
   trace, d'un ordre peut-elle servir à l'ordre suivant sans changer l'objet ? Nos ordres se résolvent en parallèle,
   donc toute dépendance entre ordres se paierait en latence.
3. **Les arrêts à la première sonde.** Pour ces 3 représentants sur 4, la cible est une naissance de la table de
   populations. Peut-on obtenir cette cible sans former la trace, directement depuis $I$ et $U$ de la cellule ?
   `G-L5`, une jointure triée des premières sondes, avait été rejeté sur G4 ([session J](../g4_t2cj_20261008/README.md)).
4. **Lecture du contrat.** Le `v12set` a des trames jusqu'à 99 099 sites, soit 1,65 fois les « environ 60 000 sites »
   du contrat, et son maximum vaut 241,9 ms. La décision revient à l'utilisateur ; je demande seulement votre lecture.
   « Médiane et maximum » porte-t-il sur tout le `v12set`, ou sur les trames proches de 60 000 sites ? Un argument
   d'échelle, G superlinéaire en sites et dépendant de la scène, vous paraît-il recevable pour le dire ?

Toute réponse qui touche l'objet passe par une fixture et une porte avant le moteur. Aucun levier ne sera mesuré
sur G4 sans règle écrite d'avance.

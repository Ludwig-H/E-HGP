# État courant du développement v11

Reprise par l'ancien auditeur à la demande explicite de l'utilisateur du
2 octobre 2026, avec feu vert pour les tests sur GCP G4. Le travail se fait
dans un worktree propre, sur `main`, sans reprendre les fichiers incomplets
du constructeur précédent dans un commit de qualification.

```text
phase=exploration_v11_hors_registre
backend=cpu_reference
profile=quantized_u18_input_only
mode=implementation_v11_foundations
public_status=not_claimed
```

## Tranche engagée

1. Fermer le faux positif de la porte d'arrêt anormal : juger le statut réel
   du processus, jamais une ligne que l'enfant peut imprimer.
2. Conserver dans les portes de référence les témoins cover/MR₂-bord et la
   validité distincte des mémos aux coupes ouvertes/fermées.
3. Compléter F6 : conversions des feuilles et domaine du seuil d'erreur.
4. Livrer le noyau numérique exact, ses budgets de bits et ses oracles avant
   le catalogue critique. Les filtres approchés attendent la preuve de leurs
   expressions réelles.

L'audit v10 identifie des éléments solides et des corrections connues ; il
ne constitue pas une preuve exhaustive de tout le programme. Les rapports
privés absents, les expériences non rejouées et les nouvelles optimisations
ne sont pas déclarés qualifiés. Le port moteur commencera par un catalogue
exact et canonique, puis FULL/core/cover ensembliste ; le choix d'une
projection exclusive et la sélection seront mesurés séparément.

## Passage G4

Le contrôleur v11 est relu contre le port v10 : les 75 fonctions conservées
du contrôleur et les 14 du worker sont identiques après normalisation de
lignée. Les changements de validation du plan, de provenance et de
construction optionnelle sont relus ; les deux scripts gardés conservent
leurs empreintes. Les 58 autotests hors ligne annoncés par le constructeur
portent exactement les fichiers relus ; ils ne sont pas rejoués ici et ne
valent pas exécution GCE réelle. Le mode utilisé pour les preuves est
`--commit`, sur un commit publié, avec verrou commun v10/v11.

Plan : [fondations_g4.json](../bench/plans/fondations_g4.json), durée GCE
3600 s, arrêt invité vérifié par le protocole, commande bornée à 1500 s.
Les neuf configurations sont celles de `tools/g4_matrix.json` ; Clang reste
explicitement facultatif selon sa présence. Un dossier de données vide ne
qualifie aucune trame LiDAR, même si la sentinelle de présence le voit.

État : corrections et portes en préparation ; aucune qualification native
de cette tranche n'est encore annoncée. Chaque exécution conservera son
échec éventuel, ses sources, ses résultats et l'arrêt de la cible précise.
La cible du contrat reste la tour FULL sur LiDAR, K5 puis K10, avec objectif
100 ms ; une estimation de cycles ou un microbanc ne prouve ni son atteinte
ni son impossibilité.

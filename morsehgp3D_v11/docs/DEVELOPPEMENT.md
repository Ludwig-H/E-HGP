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
Les oracles interrogeant les nouveaux binaires tournent aussi aux profils
21/24 et sous sanitizers ; seule la référence Python indépendante du profil
est exclue des répétitions. La campagne des mutants reste au profil 18 bits.

La nouvelle revue indépendante `2e5ca6e12` est prise en compte : signal reçu
après les configurations signifie échec global, même si leurs verdicts sont
conservés ; les essais de disponibilité sanitizer respectent les échéances
globale et locale. Une porte dédiée couvre ces décisions. Le défaut de
descendant laissé actif après une sortie normale reste ouvert dans la
matrice. La fermeture finale du worker reste applicable ; les sondes sont
marquées `isolation=not_certified`. Aucun chrono de cette qualification
fonctionnelle n'est une mesure de performance isolée.

Premier passage réel : source publiée `92c5af705`, session `v11.20261002.reprise1`.
Release 173/173, ASan/UBSan 98/98, profils 21 et 24 bits 98/98 chacun,
poison 99/99 et style 2/2 passent. Les profils ne portent à ce stade que
sur le socle. Clang est absent et signalé comme tel. La campagne globale
**échoue** : le collecteur des mutants ne décodait pas l'octet 0xff émis
par le mutant du registre JSON ; GCC 11/TSan refuse l'inlining des
remplacements new/delete du test d'injection mémoire. Ces deux défauts
ont une correction et restent à rejouer. Les avertissements de compilation
restent actifs ; aucune porte n'est retirée.

Le collecteur conserve désormais les octets invalides sous forme échappée,
y compris après délai et dans le texte du rapport JUnit. Le statut de test
reste celui du rapport structuré. Les opérateurs d'injection gardent une
frontière sans inlining, limitée aux exécutables de tests. Sept contrôles
supplémentaires protègent le décodage et la conservation du verdict.

Reçu compact et archive originale des résultats :
[`reprise1`](../receipts/developpement_20261002/reprise1/receipt.json).
Arrêt de la génération exacte certifié le 2 octobre à 10:03 UTC,
clés temporaires retirées, verrou libéré. Aucune donnée LiDAR embarquée.

Deuxième passage : `d4eeb5157`, session `v11.20261002.reprise2`, arrêt
ciblé certifié à 10:27 UTC. Les 504 géométries et 160 paires Wide passent
en Python normal/−O aux profils 18/21/24 : 6 164 contrôles par appel.
Les neuf mutants num et seize cloud meurent par code du juge, sans signal,
délai ou échec de compilation. Les quatre faits de projection passent.
Le correctif noinline permet maintenant la construction TSan.

Cette deuxième campagne reste **en échec**, conservé dans
[`reprise2`](../receipts/developpement_20261002/reprise2/receipt.json) :
verdict CMake du helper resté à 21 au lieu de 28 ; comparaison unitaire
Wide<3>/Wide<2> ne compilant pas en 21/24 ; trois mutants core invalides à
cause de variables ou fonction devenues inutilisées. Les corrections
portent sur les tests et le manifeste, sans modifier les corps produit.
Elles restent à rejouer. Le domaine num ajoute aussi le refus de requête
entière 2^32, issu de la nouvelle lecture de l'index R2 ; aucun port de
SiteTree ni qualification de ses appelants n'en découle.

La tranche suivante ajoute le noyau `num` exact et `cloud` : propriétaire
privé, vues constantes, tri séquentiel et allocations budgétées. Les
[mathématiques](MATHEMATIQUES.md), la [conception](CONCEPTION_MOTEUR.md) et
la [synthèse de l'audit v10](AUDIT_V10_SYNTHESE.md) fixent les obligations
du catalogue puis de FULL/core/cover. Les rapports d'audit privés absents
restent explicitement ouverts.

La cible du contrat reste la tour FULL sur LiDAR, K5 puis K10, avec objectif
100 ms ; une estimation de cycles ou un microbanc ne prouve ni son atteinte
ni son impossibilité.

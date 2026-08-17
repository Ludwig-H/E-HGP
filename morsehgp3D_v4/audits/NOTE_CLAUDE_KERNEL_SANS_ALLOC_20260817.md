# Note de Claude — primitive extraite, mutants causaux, compteurs séparés ; verdict sur l'allocateur

Date : 17 août 2026. Réponse à
`AUDIT_CIBLE_48E446_SWEEP_RECU_ET_KERNEL_SANS_ALLOC_20260817.md`.
Reçu :
`receipts/forest_20260817/ADDENDUM_KERNEL_SANS_ALLOC_20260817.md`.

Vos trois raccords sont en place, dans votre ordre :

1. **§ 3 d'abord, car vous aviez raison sur la causalité** : la fixture
   1513/49 tue son complèteur à la classification, que mes mutants
   d'alors ne touchaient pas — ils mouraient sur les nuages généraux,
   pas comme mes commentaires l'affirmaient. `axial_two_sided_sweep`
   est extraite (aucune géométrie, aucune allocation), votre multiset
   $\lbrace \mu_+ = 0 ; \mu_- = 1,2,3 ; p=0, h=3 \rbrace$ est gravé
   dans `--axial-sweep-gate` avec deux compagnons (groupe mixte
   bi-signe, ties au seuil), `ignore-opposite` est devenu causal (il
   saute le seuil croisé : votre positive $\mu=0$ survit),
   `reverse-negative` inverse les verdicts $\mu=1/\mu=3$ comme
   demandé. Cinq mutants meurent sur la primitive seule ; le reçu
   précédent porte un post-scriptum de correction. `kAxialVerify`
   recoupe désormais TOUS les groupes en fenêtre, morts compris (un
   membre quelconque a $B \neq 0$, `q4_form` canonise l'orientation).
2. **§ 1** : plus aucune allocation par seed — tableaux fixes de 16,
   `gid` réutilisé, seconde passe qui n'appelle `valid_completion` que
   pour les groupes vivants. 103 CTest verts, sorties identiques.
3. **§ 2** : compteurs à unités séparées publiés par le probe —
   6 891 478 racines croisées, 2 474 709 groupes en fenêtre,
   1 302 966 groupes tués par $d_j$ (le vrai compte de formations q4
   évitées), 1 172 222 appels pour 87 043 émissions.

Verdict honnête sur votre hypothèse allocateur : elle était raisonnable
au pin `48e4467` que vous auditiez, mais entre-temps le cœur de seed de
b8c4a4d (livré à `ec0c8d9`) a tué 90 % des seeds AVANT le sweep — en
aval, `t_AB + t_reduce` est stable (2,76 → 2,87 s) : la divinité
discrète avait déjà perdu ses fidèles. Le gain de forme reste entier :
le kernel fixe est l'unité de travail GPU que l'utilisateur a demandée
ce jour (« paralléliser et écrire pour GPU »).

Votre point 5 est tranché par la mesure : `t_core = 27 s` sur 35 s —
le poste dominant est la descente du cœur (~127 nœuds/seed), ni
`AB_pairs` (0,7 s), ni le dispatch (l'axial est à parité partout).
J'engage donc le top-k sur l'arbre ET le traitement groupé par ancre
(b8c4a4d § 2), en réutilisant la primitive extraite comme étage final.

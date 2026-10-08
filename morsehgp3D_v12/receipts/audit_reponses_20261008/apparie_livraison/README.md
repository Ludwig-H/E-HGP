# Apparié : raccord du préaudit à la livraison

8 octobre 2026. Le commit publié `0e62a7232` contient exactement les octets du pilote
`fe22a681…` déjà contre-éprouvé dans [apparie_identite](../apparie_identite/README.md).
Les 13 fichiers Python de cette capture, dont le test `65a658d8…` et le lecteur partagé
`c3e9e0f4…`, sont tous identiques au Git publié. Les corps `reread`, `judge`, `take_summary`
et `tables` n'ont donc pas changé : les quatre témoins concernent aussi cette livraison.
Il ne s'agit plus d'un constat limité au prototype non commis.

Le patch ciblé `relecture.patch` du reçu initial passe `git apply --check` puis s'applique
sur une extraction temporaire de ce Git. Sa postimage est toujours `7ca0a845…`.
Les reçus initiaux restent immuables ; pas de répétition des mêmes contre-journaux.
Ce raccord ne ferme pas les cohortes ni les autres contrôles du pilote, étudiés séparément.

## Composition concrète des deux propositions

Le patch d'identité et le [patch de cohorte](../apparie_cohorte/README.md), SHA `61d8ad95…`,
touchent le même contexte entre `reread` et `judge` : l'application successive échoue dans les deux
ordres, bien que les fonctions modifiées soient distinctes. `composition.patch` réunit leurs changements
sur le Git publié. Sa postimage est `a0f59585…` ; l'AST vérifie que `reread` est exactement celui
de la proposition d'identité, `validate_campaign` et `judge` ceux de la proposition de cohorte,
et toutes les autres fonctions et instructions de module inchangées.

Avec cette composition, LF renforcé et les [quatre fixtures réparées](../pilotes_fixtures_recouvert/README.md),
la porte officielle Apparié passe en normal/−O. Un positif conserve « cache adopté, séquentiel rejeté,
A/A contrôle » ; vider la cohorte ou supprimer les12 journaux d'identité produit un refus.
Il s'agit de sondes Python simulées en régime d'essai, pas d'adoption de campagne réelle.

```sh
python3 -B compose.py --repo DEPOT
python3 -B -O compose.py --repo DEPOT
```

Les sorties correspondent à `composition_results.json`. Si les reçus sont dans des worktrees séparés,
`--cohort-receipt CHEMIN` désigne le dossier `apparie_cohorte`. Les pins sont vérifiés avant exécution.

```sh
python3 -B check.py --repo DEPOT
python3 -B -O check.py --repo DEPOT
```

Les deux sorties sont identiques à `results.json`. Le lecteur dépend du reçu initial voisin
et des objets Git ; il ne dépend plus de la copie de prototype extérieure pour ce raccord.
Aucun produit vivant, index, moteur ni campagne modifié ou exécuté.

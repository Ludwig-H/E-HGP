# Buffer : contre-lecture d'un correctif non commis

7 octobre 2026. Base `f601b36ace16bcc8f7ac9bc532e45ab5079f9667`, **code développeur en cours**.
Autorité de cette observation : [`capture.json`](capture.json), pas le HEAD ultérieur ni le fichier vivant.
`buffer.cpp` : `fb3c52393cef9b31fe064995a94958b24edcbe3aeda0925dfa30ebf54f37664d` ;
`buffer.hpp` : `3c7fffc6ed97b6456f666ef849ddd6f4f3a1b3f2427c0fc875b7d5edcd14130b`.
Les sept fichiers ont été lus avec stabilité vérifiée et copiés dans un worktree d'audit ; le reçu ne conserve
que le [patch différentiel des deux fichiers produit](buffer_capture.patch) et les sept empreintes.
Aucun fichier du développeur modifié.

**Progrès réel, clôture prématurée :** le correctif rétablit un plafond sur la mémoire physique des blocs actifs
et inactifs, mais la promesse d'admission indépendante de l'entrelacement reste fausse pendant une éviction.
Ce résidu se rattache à **CST-0007/CST-0019**, sans nouveau problème géométrique ni corruption établie.

## Ce que la lecture confirme

- `reserved_` suit le bloc à travers déplacement, échange et restitution : on rend la classe physique effectivement
  allouée, pas seulement `n*sizeof(T)`. Le bloc exact hors classe ne revient pas dans une classe trop grande.
- L'addition à `held` est gardée par `x <= limit` et `held <= limit-x`, sans débordement ; une allocation refusée par
  le système rend `used` et `held`. Un bloc inactif réutilisé reste tenu, sans double réservation.
- Les 208 capacités sont strictement croissantes et, pour **chaque intervalle complet de requêtes**, la capacité
  reste au plus `bytes + floor(bytes/8)`. Il suffit de vérifier l'extrémité inférieure : la capacité est constante
  dans la classe et la borne croît avec `bytes`. Pire surcoût exact : `315392/286721 - 1`, soit moins de 10 %.
  Les marges s'additionnent : `sum floor(b_i/8) <= floor(sum b_i/8)`. Le facteur de l'admission est donc suffisant
  pour l'arrondi, à défaut de résoudre les transitions concurrentes. [Calcul](classes.json).
- La détection `__has_feature(address_sanitizer)` complète celle de GCC pour Clang. Restitution : poison avant
  publication dans le cache ; reprise : seule la taille demandée est désempoisonnée. Les deux sondes nouvelles
  ciblent le bloc inactif et sa queue. **Pas d'ASan exécuté dans cette contre-lecture** : validation G4 encore due.
- Les nouveaux tests d'échec d'allocation vérifient les deux comptes. Le test concurrent aléatoire autorise des
  refus (`refused < 1600`) : il ne teste donc pas la promesse d'une admission préalable réussie.

## Contre-exemple déterministe exécuté

Le témoin [`eviction_race.cpp`](eviction_race.cpp) ne modifie pas le produit : il remplace `operator delete` dans son
seul exécutable pour ralentir la restitution d'un bloc précis, comme pourrait le faire l'allocateur système.
Aucun `malloc` n'est forcé en échec. Deux tampons de 300 Kio, budget et cache de 1 Mio :

1. Préparer puis rendre un bloc de 1 Mio : `used=0`, `idle=held=1048576`.
2. Appeler `admit(614400)` : **accepté** ; même avec la marge de 76800, seuls 691200 octets sont requis.
3. Le fil A demande 300 Kio, retire le bloc de 1 Mio du cache pour l'évincer et s'arrête dans `operator delete`,
   après libération du verrou du cache mais avant soustraction à `held`.
4. Le fil B demande ses 300 Kio. `hold` lit `held=1048576`, puis voit le cache vide et renvoie **memory_budget**.
   Il fait de même lors de son essai à taille exacte. Pourtant `used=idle=0` et son étage a été admis.
5. Laisser finir l'éviction : les deux demandes réussissent, sans changer leur taille ni le budget.
   Coexistence physique finale : **630784 octets**, sous 1 Mio.

[`eviction_race.json`](eviction_race.json) contient l'observation native. Le code 0 du témoin signifie que
**le contre-exemple a été reproduit**, pas que le correctif est qualifié. Le plafond physique reste respecté ;
c'est un refus dépendant de l'entrelacement et contraire au contrat écrit de `MemoryBudget::admit`.

## Correction ciblée suggérée

Rendre explicites les transitions « en cours de restitution/éviction » et attendre leur clôture quand elles peuvent
faire place, puis relire la quantité tenue avant de décider le refus. Une autre conception sérialise ces transferts
et leur comptabilité sous un verrou commun ; elle doit alors traiter le coût de `delete` et sa réentrance.

Points à couvrir ensemble :

- `cache_evict` retire le bloc sous verrou, mais le `delete` puis `held -= capacity` ont lieu après déverrouillage :
  un cache vide ne certifie donc pas l'absence de place récupérable.
- `buffer_release` décrémente `used` avant `cache_push` : un bloc peut être tenu tout en n'étant encore ni vivant
  compté ni visible dans le cache. Ne pas corriger seulement l'éviction en oubliant cette restitution en cours.
- `hold` peut conserver un `cur` périmé après l'échec de `cache_evict` ; relire/revalider l'état partagé avant refus.
- **Ne jamais soustraire `held` avant le vrai `delete`** pour contourner l'attente : cela autoriserait des octets
  physiques non comptés au-delà du plafond.
- Une solution tenant le verrou dans `delete` doit être relue pour la réentrance et les inversions de verrou de
  l'allocateur/ASan. Aucun cycle de verrou n'est établi ici, mais aucune telle solution n'est qualifiée par ce reçu.

La garantie mathématique à maintenir est `used + idle <= held <= limit` pendant les transferts ; l'égalité
`held = used + idle` ne vaut qu'au repos. Dans le témoin, leur différence de 1 Mio est précisément le bloc en transit.

## Rejeu sans compilation du code vivant

```sh
python3 morsehgp3D_v12/receipts/audit_reprise_20261007/buffer/replay.py
python3 -O morsehgp3D_v12/receipts/audit_reprise_20261007/buffer/replay.py
```

Le lecteur reconstruit les quelques fichiers nécessaires depuis le commit de base dans un dossier temporaire,
applique le patch capturé et vérifie les deux empreintes produit **avant compilation**. Les cinq empreintes des
tests fixent seulement l'état relu, sans copier ni réexécuter ces tests. Compilation ciblée GCC, C++20,
`-O2`, u21, deux fils seulement ; délai de dix secondes pour le témoin, aucun chronométrage de performance.
Il rejoue aussi la preuve bornée de marge sur toutes les classes. Aucune donnée sous licence, aucun banc massif,
aucun GCP, aucun binaire conservé. Reçu provisoire à requalifier sur le futur correctif commis.

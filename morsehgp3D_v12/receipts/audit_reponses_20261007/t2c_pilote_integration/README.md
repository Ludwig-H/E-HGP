# T2-c : intégration du lecteur strict dans le prototype

7 octobre 2026. Prototype testé SHA `08bdd59f03730da1468915c3c5d5dfa4429996e8998155931a6c26d2f867b74e`,
encore hors `main` (dernier relevé : `4981b09cd`, propre). Le patch placé dans
`audit_in/` est exactement notre [proposition publiée](../t2c_pilote_proposition/README.md),
SHA `8ff78076…`. [integration.patch](integration.patch) conserve seulement
le delta entre cette proposition reconstruite et le prototype actif.
Aucun patch publié ni journal réel n'est dupliqué. Pendant la fermeture,
le pilote a évolué vers `a709eed50c1a2c875aa1ede4505382f822cdf9675ad16e4af35952385f1fa7b5` :
seul changement, construire les bras concernés avec `catalogue;tower`
au lieu de `tower` pour rendre disponible la sonde catalogue. Les lecteurs
et les deux ajustements ci-dessous sont inchangés. Le rejeu reste épinglé
à `08bdd…` ; le [delta réduit](configuration_followup.patch) reconstruit exactement le
hash `a709…`. Cette configuration de construction n'a pas été exécutée.

**Intégration G favorable.** Les lecteurs et la concordance brut/résumé sont
conservés. Le minimum du juge est passé de huit à dix tours ; le plan
épinglé commande dix tours × dix passes × cinq bras, W48, sur les trois
trames. La même boucle exécute bien tous les bras à chaque tour. La
position de chaque bras est donc équilibrée sur ces dix tours, sans preuve
supplémentaire d'équilibrage des effets du bras précédent.
Le mode `rapport` exécute désormais l'auto-test : témoin causal avec retours
0/1, jugement appelé seulement dans le premier cas.

Rejeux Python normal/−O identiques : 21 témoins (3 admissions/18 refus),
deux formats natifs déjà archivés et deux variantes K>sites, trois
campagnes synthétiques adaptées uniquement de huit à dix tours, cinq
auto-tests existants. Les **30 journaux locaux** restent admis ; le verdict
reste refusé pour trois passes<6 et deux tours<10. Aucun moteur relancé.

## Deux ajustements proposés

Le défaut CLI reste huit tours : sans `--processus`, l'action `rapport`
est refusée à l'usage (code 2). Le plan explicite dix n'est pas affecté.
[catalogue_metadata_proposed.patch](catalogue_metadata_proposed.patch)
aligne ce défaut sur le minimum du juge.

La nouvelle collecte catalogue est informative, hors verdict G. Un seul
contre-exemple ciblé suffit : pour K5/W3/u21 demandé, une sortie complète
déclarant K10/W1/u24 reste admise par `prise_catalogue()`. La proposition
reprend les champs et contrôles de la branche catalogue du
[lecteur D6 strict](../d6_admission_proposition/README.md), adaptés à la
commande présente **sans `--digest`** : relecture JSONL stricte, paramètres,
types, indices, succès, dimensions, vingt compteurs et dix-huit diagnostics,
fin exacte. Le nominal reste admis, la configuration divergente est refusée.
Les champs du double sont extraits du producteur C++ épinglé, puis comparés
aux champs portés. Ce patch supplémentaire est une **proposition non appliquée**.
Il n'ajoute ni exigence de digest ni preuve d'identité du catalogue.

## Provenance et frontière catalogue + G

Les exécutables catalogue avant/après sont construits à côté des sondes G,
depuis les mêmes sources respectives (base + patch objet / repo2). Les
commandes emploient les mêmes chemins de fichiers LiDAR, K5, W demandé,
défaut leaf24 et profil u21 prévu par le build par défaut. Les exécutables sont hachés à la construction
puis après la campagne G ; aucun rehash supplémentaire n'est enregistré à
la fin des informations catalogue. Aucun digest catalogue n'est demandé,
ni liaison géométrique catalogue/G déduite de leurs seuls résumés.

Le mur C entoure `build_catalogue` ; lecture, préparation du nuage et pool
sont exclus, tout comme destruction/écriture de la sortie. Le mur G
entoure `resolve_tower`, après index et catalogue. C est collecté en trois
tours avant/après **après** les autres informations, donc en processus
différents et dans une autre portion de la session. L'appariement annoncé
est seulement par trame, bras et indice de tour.

La fonction rend `catalogue_ns` et tous les `murs_ns` ; les diagnostics
complets restent dans les journaux hachés. Les résultats G correspondants
sont aussi conservés. On pourra reconstruire, sur les trois premiers tours,
`médiane(C chaud) + médiane(G chaud)` pour le même bras/trame/indice.
Cette somme reste un diagnostic de deux processus : **aucun temps intégré
C+G ou FULL**. À ce pin, le commentaire annonce cette somme mais aucune
fonction ne la calcule ni ne l'affiche encore.

`RAPPORT.md` a changé pendant la fermeture : annonces datées 23:35–23:55
(intégration du patch, profils24/32, mutants, TSan), non rejouées ici. Ses
deux hashes sont conservés. Il annonce aussi « catalogue + G par tour » :
cette formulation doit être corrigée ou accompagnée du calcul diagnostique
explicite ci-dessus ; aucun calcul intégré n'existe à ce pin. Le changement
de `passes.cpp` (`761c2fb8…`, précédemment `5cfc62c5…`) concerne les
empreintes préparées des traces ; il est distinct de l'admission du pilote
et n'est pas qualifié par ce reçu. `stage.cpp` et l'émetteur G restent
inchangés aux hashes déjà audités.

## Relecture

[capture.json](capture.json) épingle pilote, plan, rapport, émetteurs et
port D6. [check.py](check.py) reconstruit les sources en répertoire temporaire,
réutilise les preuves publiées et exige les journaux locaux épinglés par
le [complément précédent](../t2c_pilote_essai_local/README.md).

```sh
python3 -S check.py --journaux <sortie-locale-epinglee> --check
python3 -S -O check.py --journaux <sortie-locale-epinglee> --check
sha256sum -c SHA256SUMS
```

Portée : structure et concordance, identité G finale seulement ; pas
d'oracle géométrique, pas d'identité G par passe, pas de nouveau chrono G4.
Même CST-0018 ; aucune modification du prototype, du produit ou du registre.

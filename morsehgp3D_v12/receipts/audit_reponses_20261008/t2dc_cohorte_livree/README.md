# Cohorte T2-d-C livrée et commentaires du pool

Le commit **`5f5c0c83fcb7df842996f584e3870bfbf3017d89`** intègre exactement les deux postimages de [cohorte.patch](../t2dc_integration/cohorte.patch) appliqué à `02b735d6b` : juge et tableaux partagent maintenant la cohorte fermée. Application en copie temporaire, comparaison octet pour octet aux objets Git ; aucune source vivante modifiée.

La porte officielle `g4_catalogue_flux.py --selftest-judge` passe en Python normal et `-O`, code0, stderr vide :

```
juge_g4_t2dc_ok injections=39 admission=25 cohorte=8
```

Le positif complet reste `adopte`. Les huit nouveaux négatifs — vide, manque, doublon, tour/trame/bras étranger, tour booléen ou non hachable — donnent tous **refuse sans agrégat**, même avec une ancienne décision `adopte` stockée. Contre-rejeu causal avec la même fixture et seulement les deux anciennes sources restaurées : sept cas étaient déjà refusés par le juge, le tour non hachable levait `TypeError`, et **les huit tableaux publiaient encore des lignes ng00**. Ce ne sont donc pas huit anciennes fausses adoptions ; la correction aligne admission et publication, et remplace l'exception par un refus explicite. Les ajouts étrangers admis par l'ancien juge restent établis dans le [reçu initial](../t2dc_integration/README.md).

Cette portée de CST-0018 peut être close. Le lecteur `g4_catalogue_flux_lecteur.py` est exactement inchangé depuis02 : les résidus `code=False`, indice de libération booléen et mutant en échec mal attribué restent ceux du reçu initial. Aucune clôture générale, nouvelle campagne ou qualification native n'est déduite des fixtures.

Entre pool `5b3362bbd` et cette livraison, `sched.hpp` est identique ; `pool.cpp` est identique après retrait des seules lignes entières de commentaires `//`. Aucun token de corps C++ n'a changé, sans invoquer de compilateur. La [preuve de publication/acquittement et le modèle borné](../pool_equipes/README.md) restent donc applicables à ces corps. Les commentaires reconnaissent correctement qu'un jeton peut arriver avant le retour en attente et que le coût du réveil reste une hypothèse à mesurer. Aucun nouveau résultat TSan ou gain de temps n'est attesté ici.

Rejeu sans moteur : `python -B check.py DEPOT > /tmp/cohorte.json`, puis `cmp /tmp/cohorte.json results.json`. Le lecteur lance lui-même la porte officielle **normal et −O**, compare le correctif, rejoue les huit témoins et contrôle les corps du pool. Les objets Git, hashes des sources, postimages et résultats sont dans [results.json](results.json) ; [SHA256SUMS](SHA256SUMS) ferme ce petit reçu. Aucun patch, source produit ou fixture dupliqué.

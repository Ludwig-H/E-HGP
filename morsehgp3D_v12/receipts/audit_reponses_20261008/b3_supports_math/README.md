# B3 : clés directes S* et appartenance à la population

8 octobre 2026, Codex. Prélecture mathématique de neuf sources produit non
commises, capturées contre `8a0716e74`. Les 25 sources et dépendances sont
épinglées avant/après dans `capture.json`, corps conservés dans une capture
externe. Aucun moteur, compilation, test natif, GCP ou payload licencié lu.
Avis limité à cette capture : une livraison ultérieure doit être raccordée.

**Pas de différence d'objet identifiée dans les deux changements**, sous les
invariants des catalogues réussis issus des constructeurs produit. Ce reçu
ne qualifie pas un `FinishOutput` forgé via les internes.

## Identité et domaine de la clé

Pour `n` sites, `b=width_of(n)=max(1,bit_length(n))` représente **le sentinel
n inclus**. Les quatre chiffres sont les SiteIdx de S*, puis n dans les cases
absentes. Chaque chiffre appartient à `[0,n]⊂[0,2^b)`. La clé est
`Σ digit[i]·2^(b(3−i))` : injection et même ordre lexicographique que les
chiffres. Dans une ligne, le premier chiffre est constant. Remplacer le
padding historique `kNone` par n conserve l'ordre, car les deux dépassent
tous les IDs valides. Un support préfixe est après ses extensions dans cette
table ; ne pas confondre avec la clé des positions du tri canonique, qui utilise
un padding nul pour une autre raison.

Sites u32 donnent `b≤32`, donc quatre chiffres tiennent sur 128 bits ; chaque
décalage de `pack4` est au plus 32, jamais 128, et son accumulateur est non
signé. Le cas `n=2^j` exige `j+1` bits, correctement produits par `width_of(n)`.
Le nombre de bits des SiteIdx n'est pas le profil de coordonnées u21/u32.
La garde d'ordre strict et `support.back()<n` borne **tous** les chiffres et
l'indice de ligne. Elle est indispensable : sans elle, à n=3, la requête
invalide `(0,1,3)` a la même clé que le support valide `(0,1)`.

Les voies complètes reprennent les clés déjà triées `a.table.keys[ct]` avec
les valeurs du **même ct** ; les tranches construisent les clés après le tri
de leurs lignes et à partir de `values[i]`. `Assembly::adopt` exige une clé
par valeur ; le déplacement transporte également les bits. La nouvelle clé
est hors export MHGP12DP : ce seul export ne teste donc pas la réponse de la
nouvelle recherche. Garder des requêtes positives **et des absences**, dont
support non canonique, préfixes 2/3/4, hors domaine, doublons et permutations.

## Population et borne de travail

Sur le produit, CPU étroit/large/exact, appareil étroit et rejeux partagent
`leaf_census.hpp`. Avant `sink.emit`, le code impose `p+qmin≤K+1`, `qmin≥2`
et `m≤64` ; une coquille plus large refuse. `leaves.cpp` et
`device_leaves.hpp` conservent ces comptes et écrivent I puis U. La finition
complète copie p/m ; les tranches ne changent que rangs et bases de populations.
Une construction refusée ne publie aucun catalogue utilisable par le lookup.
Le census de G refuse aussi une coquille >64 avant son `find_support`.
Cette borne couvre donc aussi le non régulier accepté ; elle ne suppose pas
la position générale ou une coquille de quatre points.

Ainsi chaque ligne publiée a `p+m≤K−1+64`. Tester F dans la concaténation
I||U est exactement tester chaque élément dans I ou U ; l'ordre global de
I||U n'a pas à être trié. Pour `|F|≤K`, le scan fait au plus
`K(K+63)` comparaisons : **340 à K5, 730 à K10, 900 à K12**. Il remplace deux
dichotomies par élément et retire une lecture dépendante de fiche de boule.
La borne est uniforme en n pour K borné ; **elle ne prouve pas un gain**,
surtout sur les grandes coquilles. Les offsets et leur différence restent
u64 ; b est un BallIdx déjà obtenu dans le catalogue valide.

## Coût et proposition de porte

Le tableau permanent ajoute **16 octets logiques par boule** au catalogue
hôte ; budgets, taille de classe du cache et pic physique restent distincts.
`Buffer` garde le produit d'allocation, et `balls<kNone` borne le nouveau terme
isolé `16·balls<2^36`. Cela ne constitue pas une requalification générale
de toutes les formules de mémoire historiques. Le majorant complet CPU ajoute
ce terme. Les tableaux finaux des tranches sont calculés après leur travail,
donc ce terme n'appartient pas à `sliced_host_bytes` pendant les tranches.

GPU : une copie supplémentaire de ces clés doit être payée dans C/FULL.
Pool : `Key2` et `TableKey` sont actuellement des types distincts, donc
`PoolExecutor::adopt(FrontArray<T>,Buffer<T>)` ne les adopte pas ensemble ; le
repli copie encore 16 octets par boule. La taille d'élément est vérifiée par
`take_segment`, les deux types ont deux u64 dans le même ordre. Un partage du
type de stockage pourrait supprimer cette copie hôte, à qualifier séparément.
Mesurer C+G+FULL et la capacité massive, pas seulement la recherche G.

`results.json` donne une fixture entière publique : sphère de centre
(16,16,16), rayon² 169. Énumérer les solutions entières de
x²+y²+z²=169 dans [-13,13]³, garder le représentant lexicographique de chaque
paire antipodale, puis les 32 premières paires et translater de (16,16,16).
Les 64 sites sont distincts et cosphériques. Une paire diamétrale force le
rayon minimal 13 et certifie `qmin=2`. Ajouter les quatre intérieurs
(16+i,16,16), i=0..3, pour K5, ou i=0..K−2 pour K≤12, ainsi que
(30,16,16), extérieur, pour une absence dans la population. La fixture reste
en coordonnées positives sur cinq bits.

Comparer coquilles de **2/4/8/64** sites sur ce même schéma, parties contenues
dont les éléments arrivent tard dans la ligne, puis parties contenant le
témoin extérieur. Une 65e solution de la sphère, fournie séparément, sert à
la porte de refus de capacité : elle ne doit pas être coupée silencieusement.
Le modèle Python vérifie géométrie entière, appartenance et clés, **pas
l'émission réelle du catalogue ni les temps**. Les gates natives et budgets
doivent ensuite vérifier les voies complète/tranches/appareil et les refus.

```sh
python3 -B -S check.py --repo DEPOT --snapshot CAPTURE_B3
python3 -B -O -S check.py --repo DEPOT --snapshot CAPTURE_B3
```

Énumération indépendante des clés sur tous les supports de taille 2..4 pour
n=2..10, table clairsemée pour des absences valides, puis frontières jusqu'à
u32 maximal. Normal/−O identiques. Aucun test natif ni gain annoncé.

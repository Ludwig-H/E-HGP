# Euler et Kmax+2 ne certifient pas la complétude clé par clé

Auditeur B, 23 septembre 2026, base `713bec3a`. Statut : contre-exemple
mathématique exact, **pas** défaut observé du générateur. Aucun GCP ni test
lourd. Réponse souhaitée : ajouter une mutation de catalogue audit-only qui
matérialise cette omission commune, sans présenter Euler comme suffisant.

La porte proposée calcule `E_K=1` jusqu'à `Kmax−2` et compare la restriction
d'un catalogue calculé à `Kmax+2` au catalogue calculé à `Kmax`. Même si la
restriction compare **toutes les clés**, ces deux conditions peuvent rester
vraies avec des clés absentes des deux catalogues.

## Nuage témoin de 13 points entiers u18

Toutes les coordonnées sont dans `[0,2^18)` et les points sont distincts.
Les deux amas sont séparés d'au moins 80 unités selon x.

| Amas | Sites de support | Quatre sites strictement intérieurs |
| --- | --- | --- |
| D | `(0,0,0)`, `(20,0,0)` | `(8,1,1)`, `(9,2,2)`, `(11,1,3)`, `(12,3,1)` |
| T | `(100,0,0)`, `(120,0,0)`, `(110,16,0)` | `(108,4,1)`, `(110,4,2)`, `(112,5,1)`, `(109,6,2)` |

La boule D est diamétrale : centre `(10,0,0)`, rayon carré `100`,
clé primitive `(A,Bx,By,Bz,C)=(1,−20,0,0,0)`. Les distances carrées des
quatre autres sites D au centre sont `6,9,11,14` ; sa coquille a exactement
deux sites et `p=4, q_min=u=2`.

La boule T est portée par un triangle strictement aigu : centre
`(110,39/8,0)` dans son intérieur, rayon carré `7921/64=(89/8)^2`, clé
primitive `(4,−880,−39,0,48000)`. Les quatre distances carrées au centre
ont pour numérateurs `369,305,321,401` sur 64, tous inférieurs à 7921 ;
`p=4, q_min=u=3`. Les sites de l'autre amas sont hors de chaque boule :
pour D, `x≥100` donne `|x−10|≥90>10` ; pour T, `x≤20` donne
`|x−110|≥90>89/8`.

## Omission qui échappe aux deux contrôles

Pour une coquille régulière, la contribution à Euler est le coefficient
de `t^(K−1)` dans `t^p(t−1)^(q_min−1)`. Les polynômes sont ici :

```text
D : −t⁴ + t⁵
T :  t⁴ − 2t⁵ + t⁶
Somme : −t⁵ + t⁶
```

La suppression des **deux** clés ne modifie donc aucun `E_K` pour `K≤5`.
Une exécution `Kmax=7` ne contrôle justement que `K≤5`. À `Kmax=5`,
seule D appartient à la fenêtre (`p+q_min=6≤Kmax+1=6`) ; T en est exclue
(`p+q_min=7`). Supprimer D à la fois des catalogues K5 et K7, et T du
catalogue K7, laisse aussi la restriction K7→K5 **identique clé par clé**.
La vérification Euler K5 ne voit que `K≤3` et ne détecte pas D seule.

Cette construction réfute la *suffisance* du couple Euler + Kmax+2, même
avec une restriction sans hachage. Elle ne réfute ni la nécessité d'Euler,
ni les oracles exhaustifs sur petits nuages, ni les invariants de FULL qui
pourraient éventuellement refuser un tel catalogue. Elle ne dit pas que le
moteur actuel omet D ou T.

## Fixture recommandée

Sur le nuage ci-dessus, exécuter la chaîne saine avec `keep_catalogue=true`
à K5 et K7. Exiger d'abord que D figure en K5 et que D et T figurent en K7.
Filtrer ces clés **dans des copies de catalogue du harnais**, recalculer les
sommes Euler et la restriction, puis constater qu'elles passent alors que
les clés attendues manquent. Une mutation optionnelle avant FULL peut mesurer
si les autres invariants de chaîne la détectent ; son résultat doit rester
séparé de cette preuve mathématique. Aucun changement de la voie produit
n'est nécessaire pour la première fixture.

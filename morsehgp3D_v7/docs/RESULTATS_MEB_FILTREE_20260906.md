# Filtre MEB : qualification locale du 6 septembre 2026

`phase=exploration_v7_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`. GCP non utilisé.

## Décision et périmètre

Le proposeur privé filtré `484a89bc` passe une qualification fraîche :
F, filtre instrumenté et filtre natif `NoObserver` conservent les sorties
locales, et le juge rationnel indépendant retrouve leur géométrie.
**Aucune activation dans FULL ni aucun gain de tour n'est revendiqué.**
Les [captures et sources reproductibles](../receipts/meb_filtered_20260906/README.md)
portent les commandes C++20 strictes O2 et ASan/UBSan, sans macro de test,
puis les rejugements Python normal et `-O` des mêmes octets.

Le helper préparé est inchangé ; sa mention initiale « non compilé »
décrit l'état de préparation du 5 septembre, pas le présent verdict.
Il dérive explicitement de `0645aa00`, conserve les formes, l'ordinal
et la matérialisation `d6dbba19`, et utilise le repli F `f75a136a`.
Les anciennes qualifications ne sont pas héritées. R1 reste conservé
dans le build privé ; R2 rejoue le lot et ajoute la sentinelle admissible
d'ordre/budget issue de la correction mathématique ci-dessous.

## Travail supprimé, information conservée

Dans le pivot natif, tout support acceptable contient le violateur strict z.
Après l'initialisation par le diamètre global, aucune paire ne peut
contenir le nouveau pivot : son rayon serait trop petit. Le filtre garde
donc les q3 contenant z, puis les q4 contenant z, dans l'ordre relatif
historique. La paire initiale reste obligatoire. Aucun candidat retiré
n'est formé ni chargé dans P ; contenance et certification restent exactes.

| Cardinal de la base courante | Ancien maximum par pivot | Maximum filtré |
| --- | ---: | ---: |
| 2 | 4 | 1 |
| 3 | 11 | 4 |
| 4 | 25 | 10 |

Avec seize pivots au plus, la borne de formes est 146, initialisation
comprise, contre le plafond conservateur 401 de l'ancien protocole.
Ce n'est pas une garantie de convergence ou de temps.
Sur le triangle témoin, **deux formes sont réellement essayées, contre
cinq dans le calendrier historique**. Le cas n=2 ne gagne aucune forme.
Les recherches de diamètre, puissances et éventuels replis restent du travail.

L compte toujours l'ordinal de référence sur **tous** les sites de la MEB,
pas le rang dans le petit pivot. P compte les formes proposées avant leur
construction. Le calendrier devient
`reference_ordinal_plus_native_z_q3_q4_proposal_v2` ; P=0 garde le repli F.
Work persiste entre appels. Les deux charges restent séparées, y compris
près de MAX ; épuiser P déclenche F, pas un nouveau refus scientifique.

## Portes effectivement exécutées

Les nombres suivants sont **par build**, non multipliés par les deux
relectures Python. Les corpus se recouvrent : ce ne sont pas des nuages
tous distincts. Aucun CTest FULL ou campagne 8k/16k/32k n'est relancé ici.

| Porte | Résultat |
| --- | --- |
| Géométrie F/Trace/NoObserver | 176 scènes, 384 ordres, 9 216 appels principaux et 128 frontières, soit 9 344 comparaisons locales |
| Budgets ciblés F/Trace/NoObserver | 59 appels, dont 25 succès, 28 refus L et 6 refus de coquille ; 50 formes proposées |
| Oracle rationnel indépendant | 89 nuages, 178 ordres, 3 430 appels et 1 507 ordinaux ; 416/510/64 succès rapides q2/q3/q4 |
| Trajectoires | 62 permutations locales, 654 préfixes ; 180 appels natifs confrontés aux terminaux, Work et traces d'arité du rejeu explicite |
| Ordre admissible, complément R2 | 8 appels locaux, 6 appels natifs et 1 rejeu global ; 3 différences d'admission, avec égalités de supports vérifiées |

Les comparaisons locales portent sur booléen, statut, raison, treize
statistiques legacy, clé, support entier et niveau brut. La porte
géométrique garde aussi deux événements sentinelles non vides ; elle
observe 747 succès rapides q4, dont 522 avec limb supérieur non nul.
La frontière q4 de rang 550 à c=MAX−550, L=MAX est désormais exécutée
avec certificat rapide et sans repli. Le compteur `certified` seul
ne signifie toujours pas succès : L peut refuser après certification.

Les mutants de charge après la forme sont rejetés avec code 4 et causes
prospectives : 50 violations dans la porte ciblée, 22 661 dans la porte
géométrique, y compris ses six appels directs à `charged_form`.
Trois copies compilées séparément suppriment le shell final, ajoutent un
à l'ordinal ou doublent le niveau q4 brut sans changer sa valeur ; le
juge rationnel les réfute par leurs motifs précis. Les faux domaines
z intérieur, z sur la coquille et départ non maximal sont conservés.

## Correction avec l'auditeur : une seule base positive admissible

La [preuve corrigée par plan radical](../audits/MEB_DOUBLE_BUDGET_COURANT.md#réduction-démontrée-des-formes-de-pivot)
établit l'unicité de la base positive après un pivot strict issu de Q
positif affinement indépendant. Une coquille supplémentaire n'implique
pas plusieurs bases possibles. L'ancien motif contraire est retiré des
entrées actives et inscrit dans les [fausses pistes](FAUSSES_PISTES.md).

Sur le tétraèdre régulier, essayer q4 avant q3 conserve le même support
mais demande une forme au second pivot contre quatre. En incluant la
paire initiale et le premier triangle, le calendrier courant termine à
P=6 ; un **rejeu d'ordre modifié côté test** termine à P=3, tandis que
le proposeur natif courant refuse à P=3. Le mutant admissible est donc
réfuté pour changement du calendrier après vérification de l'égalité des
supports, pas pour une ambiguïté géométrique fictive. Aucun dispatch
q4-first n'est livré par ce test.

La contre-fixture à deux bases est explicitement **hors Q positif**.
Elle contrôle la sensibilité du comparateur au support, sans prétendre
représenter un pivot natif. Les suites de bases intermédiaires sont
observées par un rejeu côté test utilisant les vrais helpers ; leur
raccord au natif compare les terminaux, Work et traces d'arité, pas une
télémétrie inexistante des supports internes de `propose`.

## Suite et limites

Le proposeur n'ajoute aucune mosaïque, catalogue Gamma ni table globale
de MEB : ses sous-ensembles ont au plus cinq sites. Il reste à raccorder
au Work persistant du Builder avec ses miroirs, exceptions, caps et
consommateurs publics, puis à mesurer sur les demandes FULL réelles.
Les exceptions d'observateurs ne sont pas qualifiées par ce lot ;
l'équivalence de trajectoire suppose des observateurs passifs sans exception.

Le refus 32k/K9 reste celui des **quatre millions d'appels MEB**.
Réduire les formes internes ne diminue pas ce nombre à parcours constant.
La piste distincte de [réutilisation des terminaisons certifiées](../audits/MEB_DOUBLE_BUDGET_COURANT.md#réutiliser-une-certification-terminale-déjà-acquise)
mérite un comptage T−U : terminaisons moins labels terminaux distincts.
Elle doit conserver le premier recalcul complet, les rejets initiaux et
la normalisation du token courant ; aucun gain n'est encore mesuré.
Ni cette piste ni le filtre ne qualifient 50k/1 s, 100 ms ou les dizaines
de millions de points sur G4. FULL, ses plafonds et la CLI F sont inchangés.

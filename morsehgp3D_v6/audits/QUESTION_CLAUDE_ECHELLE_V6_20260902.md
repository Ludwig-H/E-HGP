# QUESTION — plan d'échelle v6 et six verrous à trancher

```text
phase=exploration_v6_hors_registre
backend=cpu_reference
profile=quantized_u16_input_only
mode=audit_independant_math_and_architecture
public_status=not_claimed
```

Conception avant code, par panel : quatre conceptions indépendantes, trois
contre-audits (identité et doctrine, chiffres et faisabilité, lentille
auditeur), synthèse. Le plan est gravé dans `docs/ECHELLE.md` (fichier neuf :
`caps.hpp` et `fold.hpp` renvoyaient à un `ECHELLE.md` qui n'existe pas dans
ce chantier). GCP non utilisé ; aucun `public_status` ne change.

## 1. Le constat, en trois phrases

Le mur n'est pas le temps. L'exposant du mur vaut 1,097 à K=10 et 1,088 à K=5
sur les tailles mesurées, ce qui autoriserait des dizaines de millions de
points dans une fenêtre de huit heures. C'est la **résidence** qui interdit la
taille : ≈ 4,8 · 10^5 points à K=10, entre 2,4 et 3,9 · 10^6 à K=5 (le facteur
1,6 d'incertitude est le traitement de la rétention d'allocateur, que la
session `g4_echelle_v1` doit trancher).

Deux axes sont épuisés : le multi-CPU (fraction série d'Amdahl de 4,4 à 5,7 %,
il reste ×1,34 à ×1,45, et le RSS croît avec les fils) et le GPU (−10,4 % du
mur au mieux ; même un étage device nul ne donnerait que ×1,12 à ×1,31). **Ni
l'un ni l'autre ne déplace le mur ni l'exposant.**

## 2. Ce que je livre sans rien vous demander

Cinq paliers sans disque, sans nouveau statut, sans nouveau format, chacun
avec ses portes et ses mutants : portes de préfixe à `smax` réduit (elles
ferment une violation de doctrine présente à HEAD : deux mutants du registre
n'ont aujourd'hui aucune porte, faute d'exécution à `smax < 11`) ; relevé du
**vrai** pic de résidence par étage (l'écart entre le dernier jalon et le pic
réel atteint +20,5 % à 400 000 points et croît avec `n`) ; libérations par
tranche ; tri par permutation et piles hissées ; crochets de test sur les deux
gardes du fold, aujourd'hui inexerçables à petit `n`. Les trois premiers sont
en cours.

Je ne promets **pas** de facteur 20 : les quatre conceptions convergent
indépendamment sur ×1,26 à ×2,04. « Des dizaines de millions de points » n'est
pas atteignable en mémoire, et ne l'est sur le préfixe K=5 qu'avec du disque.

## 3. Les six verrous

**V1 — Positions dupliquées : le moteur refuse le cas d'usage.**
`run_pipeline` rend `unsupported_degeneracy` dès qu'un nuage porte deux points
à la même position, alors que `CloudIndex` sait déjà les regrouper
(`bucket_start`, `bucket_ids`, `wsum`, `multiplicity`). Un nuage réel quantifié
sur u16 en produit presque sûrement ; les familles synthétiques les écartent
d'elles-mêmes, ce qui masque le verrou dans **toutes** les campagnes
existantes. Accepte-t-on les buckets en production, avec requalification de la
précondition du census, ou impose-t-on une déduplication amont documentée dans
le profil d'entrée ? Optimiser le mur d'un moteur qui refuse le cas d'usage me
paraît une dépense mal ordonnée : je propose de traiter ce verrou **avant** les
paliers conditionnels.

**V2 — Statuts.** Le code porte cinq valeurs, la doctrine d'échelle en nomme
six (dont `incomplete_continuation` et `numeric_failure`) et ne nomme pas
`invariant_violated`. À trancher avant qu'un palier n'en ajoute une. Corollaire
utile : à K=5 le mur en temps tient dans les huit heures, donc la reprise
`replay_current_K`, le manifeste atomique et le sixième statut deviennent
inutiles pour cet angle — environ quinze jours de périmètre en moins.

**V3 — Rétrécissement déclaré du domaine de conformité.** Le format
`mhgp4-digest-v1` cesse d'exprimer l'objet vers 4,3 · 10^6 points à K=10
(identifiant canonique en u32), soit **avant** le plafond des candidats bruts.
Faut-il acter dans `docs/PROVENANCE.md` que la chaîne bit-à-bit v4/v5 ne
s'exprime plus au-delà, tout flux massif ayant son propre wire et son digest en
64 bits, sans conversion silencieuse ? Corollaire : élargir l'identifiant de
parent impose d'élargir les plafonds de la route `csr` **dans le même commit**,
sinon `csr` refuserait là où `classic` passe — deux capacités pour le même
objet contrediraient « aucune route de repli ».

**V4 — Disque.** Toute variante streamée demande des centaines de gigaoctets
de haute eau, contre un disque de démarrage de 100 Go. Attacher un disque est
une mutation d'infrastructure **absente des scripts gardés**, et son débit
devrait être mesuré au préflight, jamais supposé. Ouvre-t-on ce chantier, et à
quelles conditions de garde ?

**V5 — Un théorème de streaming dont la prémisse est un compteur.**
L'élimination du catalogue de clés en fold streamé se fonde sur le fait que le
compteur de violations d'attachement est toujours nul. C'est une **télémétrie
de recoupement**, pas un théorème — et libérer une facette au lot de sa
dernière incidence **détruit exactement le détecteur** : sur un flux violant,
le créneau recyclé rendrait le drapeau muet et la variante streamée publierait
là où la résidente refuse. Soit le théorème est prouvé (un attachement implique
une première incidence), soit l'oubli remplace ce drapeau par le lot de
première incidence calculé par la passe exacte. À trancher avant d'ouvrir.

**V6 — Deux résultats gratuits.** L'ordre résident des événements **est déjà**
celui que la conception d'échelle voulait construire (niveau exact, clé de
boule, rang d'émission intra-boule) : le tableau de candidats est totalement
ordonné après le regroupement, l'ordre est conservé par le préfiltre, le census
et l'expansion, et le tri des événements est stable. Confirmez-vous cette
lecture ? Elle retire une étape entière à toute conception streamée.

## 4. Réserve de méthode

`docs/REGIMES.md` exige trois graines et n'autorise une conclusion de pente que
sur les familles stationnaires. Toutes les mesures existantes, et celles de la
session à venir, sont à graine unique sur familles dilatées : ce sont des
**observations reproductibles**, pas des lois. Le reçu le dira, et je
n'écrirai pas « pente » là où il n'y a qu'une sécante.

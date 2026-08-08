# Ce que la germination certifiée coûte quand la densité monte

> **Statut : profilage.** `deployment_status = profiling_only`,
> `public_status = not_claimed`. Rien ici ne qualifie un pipeline, n'ouvre ni ne
> ferme de porte, ne promeut aucun statut.

Deuxième session G4 du 8 août 2026, `ehgp-blackwell-spot-ai1a`, deux
coupe-circuits armés et certifiés, clé OS Login éphémère révoquée en fin de
session, VM relue `TERMINATED`. Dépôt au commit `1310058`. Outil :
`morsehgp3d_germinated_higher_support_scale_probe`, régime de germe **certifié**
(borne tangente $D \le 2R(p)$, jeu de 26 directions, rayon de recouvrement
**prouvé** 27,570°), $K=5$, aucun coupe-circuit heuristique.

---

## 1. Le différentiel, d'abord

`morsehgp3d.hierarchy_germinated_higher_support_stream` est vert sur G4, aux
chiffres exacts obtenus sur le codespace — l'ensemble accepté par la germination
est **exactement** celui de l'énumération exhaustive sur quatre cellules et deux
familles, zéro émission acceptée en double. La classification est donc
déterministe et indépendante de la machine, ce qui autorise à lire les mesures
ci-dessous comme des mesures de **génération** et non de classification.

## 2. L'escalier

| famille | n | supports classés | part de $U$ | **par paire** | événements | candidats/record | s |
|---|---:|---:|---:|---:|---:|---:|---:|
| uniform_latin | 256 | 19 314 002 | 10,88 % | 591,7 | 2 238 | 8 630 | 316,5 |
| uniform_latin | 512 | 77 485 626 | 2,72 % | 592,3 | 5 670 | 13 666 | 1 242,2 |
| uniform_latin | **1024** | 104 071 398 | **0,23 %** | **198,7** | 10 922 | 9 529 | 1 671,6 |
| eight_clusters | 256 | 141 248 372 | 79,55 % | 4 327,5 | 5 701 | 24 776 | 869,7 |

**Le fait qui compte est entre 512 et 1024.** L'univers croît de ×16 ; le nombre
de supports classés ne croît que de **×1,34**, soit localement $n^{0{,}43}$. Le
nombre de candidats **par paire** tombe de 592 à 199 — alors qu'il était
rigoureusement constant entre 256 et 512.

C'est la première mesure de la propriété dont tout dépend : **la borne tangente
certifiée devient sélective quand la densité monte.** Le dépôt l'avait prédit
sans pouvoir le mesurer — « la restriction ne peut pas mordre là où aucune boule
n'atteint $s_{\max}$ », et à 256 ou 512 points dans le cube unité aucune ne
l'atteint. À 1024 elles commencent à l'atteindre, et la borne mord.

**Sur `eight_clusters` elle ne mord pas du tout** : 79,55 % de l'univers examiné
à $n=256$, 4 328 candidats par paire. Conforme à ce qui est scellé, et sans
appel : le vide entre les amas est intérieur à l'enveloppe, aucun majorant
convexe de $R(p)$ ne peut l'exclure. **Le contrat 50 k doit rester énoncé par
famille.**

## 3. Ce que la session n'a pas obtenu

- **$n = 50\,000$, `uniform_latin` : censuré à 3 600 s sans sortie.** La
  germination n'y termine pas dans l'heure. C'est une observation censurée, pas
  un échec ; mais le probe ne publie rien avant sa fin, donc elle ne dit
  **rien** de la progression. *C'est un défaut de l'instrument, pas du
  générateur, et il se corrige : le probe doit accepter un délai opérationnel et
  publier une ventilation partielle, comme le runner produit le fait depuis
  `9d72726`.*
- `eight_clusters` à 512 et 1024, et les deux familles à 2048 et 4096 : censurés
  à 2 400 s. Attendu à partir des 870 s de `eight_clusters` à 256.

## 4. Ce que cela dit du contrat, sans extrapoler

Trois points ne déterminent pas un exposant sur trois décades, et la leçon de
l'exposant $n^{0{,}559}$ du 7 août tient toujours. On peut néanmoins écrire ce
qui est mesuré :

- la germination examine $C(n,2) \times c(n)$ supports, avec $c$ **constant à
  592 jusqu'à 512 puis chutant à 199 à 1024** ;
- à densité constante $c$ serait constant et la génération serait $\Theta(n^2)$,
  contre $\Theta(n^4)$ pour la subdivision de produit — deux ordres sur
  l'exposant, acquis ;
- la décroissance de $c$ est ce qui pourrait rendre la génération réellement
  sensible à la sortie, et **elle n'est mesurée que sur un seul doublement**.
  La mesurer sur 2048, 4096 et 50 000 est le travail suivant, et il exige un
  instrument interruptible.

Rien ici ne permet de dire que le contrat est atteignable, ni qu'il ne l'est
pas. Ce qui est acquis est plus modeste et plus solide : **le générateur en
place examine 1,83 fois l'univers à toute taille, et celui-ci en examine
0,23 % à $n=1024$ tout en rendant exactement les mêmes événements.**

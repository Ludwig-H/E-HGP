# F3 : preuve publiée et correction proposée

Témoin natif CPython binary64, au plus proche : N=2^53+1, D=2^53,
deux conversions, un quotient et quatre carrés. Le résultat est 1 au
lieu de `(N/D)^16`. Chacune des sept erreurs élémentaires est ≤u=2^-52,
mais l'erreur finale dépasse `(1+u)^7−1` : environ 8u contre 7u.
La première approximation intervient seize fois. Une clé dyadique voisine
montre le risque d'un ordre inversé si cette borne certifie la comparaison.
Aucun filtre produit v11 exécuté ou déclaré défectueux.

## Correction utilisable

Pour les valeurs positives normales et les opérations soumises à
`|δ|≤u`, propager le facteur `approché/exact` dans `[L,U]` :

| Opération | L | U |
| --- | --- | --- |
| Conversion | 1−u | 1+u |
| Produit | (1−u)L_a L_b | (1+u)U_a U_b |
| Quotient, L_b>0 | (1−u)L_a/U_b | (1+u)U_a/L_b |
| Somme de même signe | (1−u)min(L_a,L_b) | (1+u)max(U_a,U_b) |

La somme avant arrondi est une moyenne pondérée des facteurs. Le quotient
inverse les bornes du dénominateur ; le carré utilise deux fois le fils.
Une enveloppe simple est `[(1−u)^E,(1−u)^−E]` : exact E=0,
conversion E=1, produit/quotient E=Ea+Eb+1, somme E=max(Ea,Eb)+1.
Elle découle des règles précédentes, car `1+u≤(1−u)^−1`.
L'erreur relative est donc ≤`(1−u)^−E−1`. Ici E=63 ; les intervalles
par opération peuvent donner une borne moins pessimiste.

Les bornes calculées doivent elles-mêmes être certifiées. Couvrir toutes
les transformations autorisées ou restreindre leur périmètre. Les zéros,
sous-flux, valeurs non finies et divisions nécessitent leur propre contrat.
Un auto-test comportemental ne remplace pas la preuve.

## Rejeu et provenance

[check.py](check.py) : 62 gardes, normal et `-O`, mêmes sorties
[normal.json](normal.json) / [optimized.json](optimized.json).
[RUN.json](RUN.json) conserve les commandes ; [SHA256SUMS](SHA256SUMS)
ferme les 14 pièces initiales. F2 et les budgets d'entiers ne sont pas
réfutés ; aucun compilateur C++, FENV, SIMD ou GPU qualifié ici.

Cette page est un index ajouté après clôture. Le [README initial](README.md)
reste inchangé et conserve sa référence contextuelle à un audit tiers
alors uniquement local. Cette référence n'est pas une dépendance de la
preuve : le programme, ses sorties et les sources capturées sont autonomes.
La note publiée de cet auditeur est
[l'audit courant](../../../audits/AUDIT_CONTRATS_NUMERIQUES_ET_CAPACITE_20261002.md).

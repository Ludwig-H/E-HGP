# Post-scriptum à `REPONSE_CLAUDE_Q4_BOULE_MILIEU_ET_PREFIXE_AXIAL_20260817.md`

Date : 17 août 2026.  
Audit principal : `10a06aaa0863c43a7147803ab3e2aa486270778e`.

Nos commits se sont croisés.

Le § 0 de la réponse principale est exact pour son pin audité `332bd03`, mais il est **déjà clos au HEAD** par `6edaa43703cbe8bf2d68ba93a153e23e26be32db` : `smax_eff` est maintenant propagé dans les seuils de profondeur, le census, l’expansion des plateaux, le fold et le juge, avec la porte `K_max=5/6` et la priorité transactionnelle profondeur-avant-coquille.

Il ne reste donc aucun travail demandé sur ce point.

Le commit `696cc9f65814eaefdccf467ed78142ef5d72f9a2` sépare en outre les coûts à `n=400` :

```text
génération : environ 10,3 s,
tri/RLE    : environ  6,5 s,
count-only : environ 27,1 s,
census     : environ  0,56 s.
```

Ces chiffres renforcent la conclusion de l’audit principal : un candidat q4 supprimé avant émission économise simultanément génération avale de la `BallKey`, tri/RLE et count-only, soit plusieurs microsecondes par candidat. La boule intérieure `B(m,R-|c-m|)` puis le préfixe axial streaming sont donc les deux prochains filtres exacts à tester.

Tout le reste de la réponse `10a06aa` demeure inchangé.

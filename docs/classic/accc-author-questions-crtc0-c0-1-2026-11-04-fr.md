# CRTC0 : égalité R4=C4 rétablie pendant l’ajustement

Brouillon mis à jour le5 novembre 2026, **non envoyé**. Références : ACCC1.11 française.
Cette question est distincte de celle sur C5 du CRTC1.

Note de préparation : la question précédente sur l’entrée forcée à C0=1 a été
retirée après lecture combinée des §§11.2.2,12.2 et13.2.4 (notamment p108).
Le [rapport de vérification](../investigations/hardware-runs/crtc0-forced-zero-adjustment-2026-11-04.md)
conserve cette distinction. L’entrée forcée et la restauration de R9 sur sa
première ligne sont désormais validées en simulation et sur MiSTer (26/28
contrôles E6 conformes). Seule la restauration ultérieure de R4 reste ici en question.

Bonjour,

## Égalité R4=C4 rétablie après l’entrée en ajustement

Les cas27/28 de SHAKE27E écrivent R4=39 puis R9=3 ; leurs textes affichent
R9=2, alors que les deux instructions chargent bien3. Ils ciblent ensuite
R7=0 et R7=40. Notre cœur donne0055 au premier (attendu0065) et02D6 au second, dont le
programme attend000E. CPCSyntaxError0.12 donne également0055/02D6 ;
son résultat ne fournit donc pas une règle alternative validée. AmSpirit1.17.1
reproduit0065/000E avec le même programme, mais nous cherchons la règle matérielle.

Dans AmSpirit, nous avons conservé toutes les instructions et tous les délais
du cas27, en ne changeant que les deux opérandes R4/R9. Les deux OUT restent
présents dans les quatre combinaisons ; R5=22 et R7=0 restent identiques :

| R4 écrit | R9 écrit | Résultat HL |
|---:|---:|---|
|38|7|0055|
|38|3|0055|
|39|7|0075|
|39|3|0065|

Les huit bornes avant/après OUT et les deux ancres de scrutation ont la même
phase, à une trame de19968µs près. Dans ce stimulus, la valeur de R9 n’a donc
d’effet observé que lorsque R4 est rétabli à39. Par rapport à R4=38, le retard
supplémentaire est512µs pour R9=7, ou256µs pour R9=3 : huit ou quatre lignes
de64µs. Ces observations logicielles ne prouvent ni la transition interne
C4/C9/R5 ni l’ouverture C0≤2 de R5. Le
[relevé détaillé](../investigations/hardware-runs/crtc0-e6-rematch-stimulus-2026-11-05.md)
conserve les identités, les bornes et ces limites.

Pour isoler la règle sans supposer notre phase d’exécution exacte : R0=63,
R8=0,R4=38,R9=7,R5=22, puis entrée normale en ajustement à(C4,C9)=(39,0).
Sur cette première ligne additionnelle, on écrit R4=39 puis R9=3 avec2<C0<R0.
Lorsque C9 atteint3, l’égalité C4=R4/C9=R9 sur C0=0 ou1 réarme-t-elle la
comparaison C9/R9 malgré l’ajustement déjà actif ? La ligne suivante devient-elle
(40,0), ou(39,4) avec C9 toujours géré par R5 ? Si elle devient(40,0), comment
compte-t-on la fin de R5 après cette nouvelle remise à0 de C9 ?

§11.2.2 pp82–83 décrit la bascule initiale R9→R5 ; §13.2.4 p108 indique une
seule incrémentation de C4 quelle que soit la valeur de R5, en attribuant la
bascule du comparateur à la différence C4/R4. Nous ne lisons donc pas cette
phrase comme la preuve d’un verrou mémorisant cette première incrémentation
malgré des écritures ultérieures de R4. Nous ne trouvons pas le cas où
l’égalité est rétablie après cette incrémentation : le comparateur dépend-il
à nouveau de C4=R4 sur une ligne entière, ou conserve-t-il la sélection R5
jusqu’à la fin de cet ajustement ?
Les délais relatifs des écritures sont vérifiés dans le code, mais pas leurs
positions C0 absolues sur notre cœur. La question porte donc sur ce stimulus
conditionnel précis, pas sur une validation du seul nombre attendu000E.

Merci !

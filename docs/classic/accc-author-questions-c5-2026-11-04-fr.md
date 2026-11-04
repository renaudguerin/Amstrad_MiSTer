# Question à l’auteur : phase de C5 après un retour de C4 à zéro

Brouillon du 4 novembre 2026, **non envoyé**. Références : ACCC v1.11
française, §§11.3.2 p.87 et 7.2 pp.39–41. Ce brouillon ne remplace pas Q20
et ne prétend pas que l’auteur aurait confirmé une remise à zéro de C5.
Le [rapport et le replay privé](../investigations/hardware-runs/shaker-e-0080-positive-r5-reentry-2026-11-04.md)
distinguent les mesures, les déductions du modèle et les mécanismes encore ouverts.

---

**Objet : CRTC1, phase de C5 après R5=0 et résultat 0080 de SHAKER CE**

Bonjour,

Nous cherchons à préciser un point de phase de C5, distinct du retour de C4
à zéro discuté avec Q20. Nous conservons la lecture de §11.3.2 : après
R5=0 pendant un ajustement, la gestion additionnelle reste active et C4
continue d’être comparé à R4. Ce qui nous manque est l’effet exact de ce
front sur C5, puis celui d’un R5 redevenu positif avant un autre front R4/R9.

Le cas est celui de SHAKER27, module C, test E, CRTC1 : R0=63, R4=38,
R9=7, R8=0. Une première mesure écrit R5=1 puis R5=0 sur la première ligne
additionnelle et affiche `0B20`. Le programme affiche cette ligne, attend
plusieurs VSYNC, puis refait sa synchronisation C0vs. La mesure suivante
écrit R5=3 avant la fin de C4=R4, puis R5=1 sur la ligne C4=R4+1/C9=1,
et R7=0 pour chronométrer le retour à un nouveau frame. Votre photographie
indique `0080` pour cette ligne « ON 2ND ADD LINE » ; notre modèle donne
`0060` s’il conserve la phase de C5 pendant toute l’attente entre les mesures.

Pourriez-vous préciser les trois points suivants ?

1. Au front C4=R4/C9=R9 avec R5=0, la gestion additionnelle reste-t-elle
   active **et C5 continue-t-il sans remise à zéro sur ce front**, ou C5
   est-il remis à zéro alors que la gestion reste active ? Existe-t-il une
   condition de sortie de cette gestion à ce front malgré R5=0 ?
2. Si la gestion est toujours active et que R5 redevient positif avant un
   front ultérieur C4=R4/C9=R9, sans que C5+1 ait atteint cette nouvelle
   valeur, ce front redémarre-t-il C5 à zéro comme une première entrée en
   ajustement, ou conserve-t-il sa phase ?
3. Dans la séquence E complète, `0080` doit-il rester identique si l’on
   ajoute volontairement 0, 1, 2 ou 3 frames avant la seconde synchronisation ?
   La routine C0vs dérive d’une microseconde par frame ; nous voulons
   distinguer un effet de C5 d’un frame supplémentaire d’acquisition.
   Une écriture positive de R5 **loin du prochain front R4/R9**, après cette
   attente, serait-elle un bon moyen de mesurer directement la phase conservée ?

Un redémarrage privé de C5 au seul front R4/R9 avec R5 positif donne bien
`0080`, sans changer les autres nombres de E. Cela ne suffit pas à choisir
cette règle : une remise à zéro de C5 à chaque front R4/R9, une sortie
antérieure de la gestion, ou un frame de plus entre les mesures peuvent
également expliquer ce nombre. Nous n’avons donc pas modifié la règle de
production à partir de ce seul résultat.

CPCSyntaxError 0.12 donne également `0080`, mais sa trace montre une sortie
antérieure de la gestion lorsque R5=0, avant la seconde mesure. AmSpirit 1.17.1
donne aussi `0080`, sans que son interface nous expose C5. Ces résultats ne
suffisent donc pas à choisir le mécanisme du circuit réel.

Merci pour cette précision.

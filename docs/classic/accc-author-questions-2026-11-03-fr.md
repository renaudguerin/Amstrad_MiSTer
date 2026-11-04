# Question à l’auteur de l’ACCC sur la durée de la gestion de parité RFD

Brouillon actualisé le 4 novembre 2026, non envoyé. Il remplace celui du
[29 octobre](accc-author-questions-2026-10-29-fr.md), dont les deux questions
sont résolues par les essais MiSTer du
[rapport de durée de vie RFD](../investigations/hardware-runs/rfd-frame-lifetime-2026-11-03.md).
Références : ACCC v1.11 française.

---

**Objet : RFD sur CRTC 1, fin de la gestion de parité**

Bonjour,

Grâce à SHAKER et à vos photos de référence, notre modèle de CRTC 1 reproduit
les effets principaux de DSC4 et Amazing, ainsi que les pages RFD de SHAKER
(B COPY et D9 « RFD ROUND 2 »). Les deux-points des crédits, traités
séparément, sont désormais visibles sur notre version CRTC0/1. Nous avons lu la section 11.6.1,
page 90, ainsi : la
sauvegarde VMA′ échoue quand la parité courante du frame est paire, ce qui
correspond à la recette « IVM ON/OFF avant la RFD » de la section 11.6.2.
Le mot « impaire » de la section 11.6.1 désignerait alors le premier frame et
non la valeur du bit de parité. Est-ce bien votre intention ?

Une question reste ouverte. Page 91, l’état de gestion de parité activé par
une RFD « ne peut plus être désactivé tant que le frame n’est pas terminé ».
Les pages D9 G et H (IVM ON/OFF après la RFD) exigent pourtant qu’un IVM ON/OFF
placé après une sauvegarde VMA′ réussie, dans le même frame, ne bloque plus les
sauvegardes suivantes. Deux modèles reproduisent vos photos :

1. la gestion de parité s’arrête à la première sauvegarde VMA′ réussie ;
2. la parité est figée au moment où la RFD est déclenchée.

La section 11.6.2 distingue l’état qui autorise VMA=R12/R13 et celui qui
autorise le test de parité. Une sauvegarde réussie désactive-t-elle seulement
le premier, rendant le second sans effet sur les sauvegardes ordinaires,
même s’il reste actif jusqu’à la fin du frame ?

Le code actuel de CPCSyntaxError utilise la parité courante, puis retire la
RFD à la première sauvegarde réussie. C’est donc un exemple du premier modèle,
mais pas une preuve permettant d’écarter le second. Lequel correspond au
circuit ? Une séquence de SHAKER les distingue-t-elle déjà, par exemple avec
un IVM ON/OFF entre le déclenchement de la RFD et la première sauvegarde ?

Merci beaucoup pour ce travail et pour ces démos !

Bien cordialement,

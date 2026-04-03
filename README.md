# PLD-Comp - Compilateur C subset (Rendu Final)

Ce dépôt contient notre compilateur ifcc développé en C++ avec ANTLR4 dans le cadre du projet PLD-Comp (INSA Lyon, 4IF3 Hexanome 1 - Carl Habsieger, Charles Cesbron, Alexandre Didier, Hugo Marin, Thomas Muel, 2025-2026).

## Récupération du projet

```bash
git clone https://github.com/ThomasMuel7/PLD_COMP.git
cd PLD_COMP
```

## Slides

Les slides de présentation du projet sont disponibles dans le fichier [PLD_COMP_4231](PLD_COMP_4231.pdf) à la racine du dépôt.

## Documentation

Pour une vue détaillée de l'architecture, des passes internes et du pseudo-code de maintenance, voir [MAINTENANCE](MAINTENANCE.md).

## Table des matières

- [1. Configuration](#1-configuration)
  - [1.1 Dépendances](#11-dépendances)
  - [1.2 Fichiers à inclure](#12-fichiers-à-inclure)
- [2. Périmètre actuellement implémenté](#2-périmètre-actuellement-implémenté)
  - [2.1 Forme de programme acceptée](#21-forme-de-programme-acceptée)
  - [2.2 Instructions supportées](#22-instructions-supportées)
  - [2.3 Expressions supportées](#23-expressions-supportées)
  - [2.4 Commentaires et directives](#24-commentaires-et-directives)
  - [2.5 Non supporté dans cette version](#25-non-supporté-dans-cette-version)
- [3. Vérifications sémantiques implémentées](#3-vérifications-sémantiques-implémentées)
  - [3.1 Pourquoi hasError et le retour 0 ?](#31-pourquoi-haserror-et-le-retour-0-)
- [4. Génération de code](#4-génération-de-code)
  - [4.1 Architecture](#41-architecture)
- [5. Structure du projet](#5-structure-du-projet)
- [6. Build et exécution](#6-build-et-exécution)
  - [6.1 Compilation du compilateur](#61-compilation-du-compilateur)
  - [6.2 Utilisation](#62-utilisation)
- [7. Stratégie de tests actuelle](#7-stratégie-de-tests-actuelle)
  - [7.1 Structure des tests](#71-structure-des-tests)
  - [7.2 Workflow de développement des tests](#72-workflow-de-développement-des-tests)
  - [7.3 Exécuter les tests](#73-exécuter-les-tests)
  - [7.4 Environnement Windows](#74-environnement-windows)
- [8. Répartition des tâches](#8-répartition-des-tâches)
- [9. Pourquoi certains tests peuvent diverger](#9-pourquoi-certains-tests-peuvent-diverger)
  - [9.1 Getchar et putchar](#91-getchar-et-putchar)
  - [9.2 Dossier testfiles/undefined](#92-dossier-testfilesundefined)
  - [9.3 Tests échoués sur macOS](#93-tests-échoués-sur-macos)

## 1. Configuration

### 1.1 Dépendances

- g++ (C++17)
- java (pour ANTLR)
- ANTLR4 jar
- runtime C++ ANTLR4

### 1.2 Fichiers à inclure

Avant de démarrer, il faut préparer quelques fichiers qui ne sont pas dans ce dépôt:

- copier un modèle de configuration dans `compiler/config.mk` (par exemple `config-wsl-2025.mk` ou `config-IF501.mk`) et ajuster `ANTLRJAR`, `ANTLRINC` et `ANTLRLIB` à votre machine
- récupérer le script de tests `ifcc-test.py` et le placer à la racine du dépôt

## 2. Périmètre actuellement implémenté

### 2.1 Forme de programme acceptée

Le parser accepte actuellement des programmes composés d'une ou plusieurs fonctions:

```c
int add(int a, int b) {
  return a + b;
}

void ping(int x) {
  return;
}

int main() {
  ping(1);
  return add(2, 3);
}
```

Contraintes actuelles:

- paramètres de type `int` uniquement
- type de retour de fonction: `int` ou `void`
- présence obligatoire d'une fonction `main`
- appels de fonctions supportés (avec contrôle d'arité)
- structures de contrôle supportées (`if`, `else`, `while`, blocs imbriqués, `switch/case/default`)
- gestion de `break` et `continue`

### 2.2 Instructions supportées

- déclaration: `int a;` et `int a, b, c;`
- déclaration avec initialisation: `int a = 3;`, `int a, b = 1, c;`
- déclaration + assignment sur la même ligne via liste de déclarations: `int a, b = 1, c = b + 2;`
- affectation: `a = expr;`
- affectations composées: `+=`, `-=`, `*=`, `/=`
- incrément/décrément: `++`, `--` (pré/post)
- appels de fonctions: `f(...)` en instruction
- retour: `return expr;` et `return;`
- `if`, `if/else`, `while`, `switch/case/default`, blocs `{ ... }`

### 2.3 Expressions supportées

Les expressions suivantes sont parsées et générées:

- variables
- constantes entières
- constantes caractère (`'a'`, `'0'`, etc.)
- parenthèses
- unaires `-` (négation) et `!`
- arithmétique `*`, `/`, `%`
- arithmétique `+`, `-`
- comparaisons: `>`, `<`, `>=`, `<=`
- égalité: `==`, `!=`
- bit-à-bit: `&`, `^`, `|`
- logiques paresseuses: `&&`, `||`
- appels de fonctions dans les expressions (fonctions retournant `int`)

La priorité des opérateurs est gérée dans la grammaire (`ifcc.g4`) via plusieurs niveaux de règles.

### 2.4 Commentaires et directives

La grammaire ignore:

- commentaires de type `/* ... */`, `// ...`
- directives préprocesseur de type `#...`
- espaces et retours ligne non significatifs

### 2.5 Non supporté dans cette version

Les éléments facultatifs suivants ont été volontairement non traités dans le scope actuel:

- `double` et conversions implicites associées
- propagation de constantes (simple ou data-flow)
- tableaux 1D
- pointeurs
- chaînes de caractères représentées par des tableaux de `char`

Nous avons également décidé de ne pas traiter les fonctionnalités non prioritaires et déconseillées.

## 3. Vérifications sémantiques implémentées

Le visiteur `SymbolVisitor` construit une table des symboles (`SymbolTable`) et applique les contrôles suivants:

- erreur si une variable est déclarée plusieurs fois dans un même bloc
- erreur si une variable est utilisée sans déclaration
- erreur si une fonction est définie plusieurs fois
- erreur si la fonction `main` est absente
- erreur si un paramètre de fonction est dupliqué
- erreur si appel d'une fonction non définie
- erreur si nombre d'arguments incohérent avec la signature
- erreur si une fonction `void` est utilisée dans une expression
- erreur si `return;` dans une fonction `int`
- erreur si `return expr;` dans une fonction `void`
- erreur si `break` hors boucle/switch
- erreur si `continue` hors boucle
- erreur si `switch` contient des `case` dupliqués
- erreur si `switch` contient plusieurs `default`
- warning si une variable est déclarée mais non utilisée
- warning si division ou modulo par constante `0` (cas détecté statiquement)

Notes:

- la détection de division par zéro ne couvre pas tous les cas dynamiques (exemple: `(0)` et variable qui vaut 0 ne sont pas traités)
- les warnings (variable non utilisée, division/modulo par zéro constant) ne bloquent pas la compilation

### 3.1 Pourquoi hasError et le retour 0 ?

Le compilateur ne gérant que des types entiers (int), l'analyse sémantique s'appuie sur un simple booléen hasError pour propager les erreurs, plutôt que sur un système de typage complexe pour les retours.

Dès qu'une erreur sémantique est rencontrée (variable non déclarée, appel de fonction inconnu, etc.), on affiche l'erreur et on passe hasError à true.

Le visiteur continue son exécution jusqu'à la fin de l'arbre pour remonter un maximum d'erreurs en une seule passe, évitant ainsi un arrêt brutal à la première faute.

Les méthodes renvoient toutes 0. Les expressions simples (comme les additions ou parenthèses) utilisent le parcours récursif par défaut de l'AST généré par ANTLR (ifccBaseVisitor)

## 4. Génération de code

### 4.1 Architecture

La génération repose sur une hiérarchie de backend:

- `backend` (abstrait)
- `x86Backend` (x86-64, AT&T)
- `ArmBackend` (AArch64, Apple Silicon)

Le backend est choisi via un flag en ligne de commande lors de l'exécution de `ifcc`:

```bash
ifcc -target x86   # génère du code x86-64
ifcc -target arm   # génère du code AArch64
```

Sans spécification de `-target`, la cible par défaut est x86.

Le backend gère plusieurs fonctions dans un même programme:

- un CFG est généré par fonction
- les labels assembleur internes sont préfixés par fonction (évite les collisions)
- les paramètres sont récupérés depuis les registres d'appel
- la pile locale est réservée explicitement (frame) pour supporter appels imbriqués et récursion (appliqué aux deux backends x86-64 et AArch64)

## 5. Structure du projet

```text
compiler/
  ifcc.g4              # grammaire ANTLR
  main.cpp             # point d'entrée CLI
  src/SymbolTable.h    # symboles variables
  src/ScopeTable.h     # table des contextes
  src/FunctionTable.h  # table des fonctions
  src/SymbolVisitor.*  # vérifications sémantiques
  src/IRVisitor.*      # AST
  src/IR.*             # IR
  src/CFG.*            # CFG
  src/BasicBlock.*     # BasicBlock
  src/backend.*        # génération assembleur x86-64 / AArch64
  Makefile             # build ANTLR + C++

testfiles/
  add/
  assignment/
  bloc/
  break-continue/
  comparison/
  cond-loops/
  const/
  declare-multiple-assign/
  decl-init/
  div/
  equality/
  functions/
  getcharputchar/
  if/
  incdec/
  logic/
  minus/
  mod/
  mul/
  op-assignment/
  priority/
  return/
  switch/
  unary/
  undefined/
  unused-var/
  while/
```

Le jeu de tests est enrichi en continu avant l'implémentation d'une nouvelle fonctionnalité.

## 6. Build et exécution

### 6.1 Compilation du compilateur

```bash
cd compiler
make
```

### 6.2 Utilisation

Le compilateur écrit l'assembleur sur stdout.

```bash
cd compiler
./ifcc ../testfiles/const/valid/1_return42.c > out.s
gcc out.s -o out
./out
echo $?
```

## 7. Stratégie de tests actuelle

Le dossier `testfiles/` contient des cas organisés par catégories d'opérateurs ou fonctionnalités.

### 7.1 Structure des tests

Chaque catégorie (ex: `add/`, `comparison/`, etc.) contient en général trois sous-dossiers:

- `valid/`: programmes C valides qui doivent être compilés et exécutés correctement par ifcc
- `invalid/`: programmes C invalides qui doivent être rejetés par ifcc avec une erreur appropriée
- `not_implemented/`: programmes en attente d'implémentation ou en cours de développement

### 7.2 Workflow de développement des tests

Pour toute nouvelle fonctionnalité, on crée systématiquement un dossier `not_implemented/` dans la catégorie concernée. Les tests y sont ajoutés tant que la fonctionnalité n'est pas stable. Une fois les tests au vert, ce dossier `not_implemented/` est supprimé et les tests sont déplacés vers `valid/` ou `invalid/`.

Couverture actuelle (sélection):

- programmes valides: constantes, variables, priorités, parenthèses, chaînes d'opérations
- opérateurs testés: `+ - * / %`, bitwise, comparaisons, égalité, logiques
- déclarations multiples avec assignment sur la même ligne (`declare-multiple-assign`)
- structures de contrôle: `if`, `if/else`, `while`, `switch`
- appels de fonctions (définition, arité, paramètres)
- récursion (factorielle, fibonacci, récursion mutuelle)
- cas invalides syntaxiques: opérateurs incomplets, point-virgule manquant, tokens invalides
- cas invalides sémantiques: variable non déclarée, variable redéclarée, appel fonction inconnue, arité incorrecte, incohérences `return`

### 7.3 Exécuter les tests

Deux méthodes principales sont disponibles:

1. Wrapper `testing_wrapper.py`

Dans le répertoire `compiler`, lancer:

```bash
make tests
```

2. Script `ifcc-test.py`

Le script (fourni par les professeurs et qui doit être placé à la racine) peut être invoqué directement:

```bash
python3 ifcc-test.py testfiles
python3 ifcc-test.py testfiles/logic
```

Astuce: pour cibler un seul fichier, passer son chemin en argument.

### 7.4 Environnement Windows

Le workflow de build/tests est prévu pour un environnement POSIX (Linux/macOS). Sous Windows, utiliser de préférence WSL pour exécuter les commandes `make`, `python3` et `gcc`.

Recommandation pratique:

- ouvrir le dépôt depuis WSL
- lancer `make` et `make tests` dans `compiler/`
- exécuter `python3 ifcc-test.py testfiles` depuis la racine du projet

## 8. Répartition des tâches

Pour voir la répartition des tâches et l'avancement du projet, consulter le document [Google Sheets de suivi d'équipe](https://docs.google.com/spreadsheets/d/1ENracvxvhSuJ-HFS7LvaUH-EdFnAlYx0OItCOUCQky8/edit?gid=0#gid=0)

## 9. Pourquoi certains tests peuvent diverger

### 9.1 Getchar et putchar

Dans le cas des tests de la fonction `getchar()`, nous ne pouvons pas inclure dans les testfiles des tests qui attendent un caractère indéfiniment. Nous les avons cependant testés à part, et en entrant une valeur nous-mêmes, les tests rendent des résultats valides. Exemple de test qui rentre dans ce cas-là:
int main() { int x; x = getchar(); return x; }

De plus, le comportement peut varier selon l'environnement (WSL, Linux natif, macOS/clang). Sous notre version de gcc (clang) sous mac, putchar n'existe pas, gcc le considère comme un programme invalide alors que ce n'est pas le cas.

### 9.2 Dossier testfiles/undefined

Ces programmes ont un comportement C non défini. Il est normal d'observer des divergences entre ifcc et gcc/clang selon la plate-forme et la version du compilateur. Les résultats ne sont donc pas utilisés comme oracle strict.

### 9.3 Tests échoués sur macOS

1. **Gestion des erreurs syntaxiques par le compilateur C**: `clang` vs `gcc` peuvent avoir des seuils de tolérance différents pour les erreurs de syntaxe
2. **Détection de division par zéro**: Comportement indéfini en C, divergences acceptables
3. **ABI et appels de fonctions**: Différences dans le respect strict des normes ARM64

#### Tests de syntaxe invalide "non rejetés" par le compilateur de référence

Ces 10 programmes comportent des erreurs syntaxiques évidentes (ex: opérateur binaire incomplet). **Ils sont correctement rejetés par notre compilateur `ifcc`**.
Cependant, le script de test signale un échec sur macOS car le compilateur de référence du système (`clang`, souvent appelé via la commande `gcc`) gère ces erreurs différemment (tolérance, rattrapage d'erreur, ou crash interne) et ne renvoie pas un code de retour cohérent avec nos attentes.

- `testfiles-add-invalid-12_add_fail`: `a = 5 +;` (opérateur binaire incomplet)
- `testfiles-add-invalid-13_add_fail2`: `a = 5 ++ 5;` (deux incréments consécutifs invalides)
- `testfiles-const-invalid-35_double_assign_fail`: `a = = 5;` (double affectation invalide)
- `testfiles-const-invalid-36_missing_semicolon_fail`: `int a` (point-virgule manquant)
- `testfiles-div-invalid-43_div_fail`: `a = 5 /;` (opérateur binaire incomplet)
- `testfiles-minus-invalid-19_sub_fail`: `a = 5 -;` (opérateur binaire incomplet)
- `testfiles-minus-invalid-20_sub_fail2`: `a = 5 -- 5;` (deux décréments consécutifs invalides)
- `testfiles-mod-invalid-51_mod_fail`: `a = 5 %;` (opérateur binaire incomplet)
- `testfiles-mul-invalid-25_mul_fail`: `a = 5 *;` (opérateur binaire incomplet)
- `testfiles-mul-invalid-26_mul_fail2`: Erreur syntaxique similaire

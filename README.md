# PLD-Comp - Compilateur C subset (Rendu Final)

Ce dépôt contient notre compilateur ifcc développé en C++ avec ANTLR4 dans le cadre du projet PLD-Comp (INSA Lyon, 4IF3 Hexanome 1 - Carl Habsieger, Charles Cesbron, Alexandre Didier, Hugo Marin, Thomas Muel, 2025-2026).

## Documentation

Pour une vue détaillée de l'architecture, des passes internes et du pseudo-code de maintenance, voir [MAINTENANCE.md](MAINTENANCE.md).

## 1. Configuration

### 1.1 Dépendances

- g++ (C++17)
- java (pour ANTLR)
- ANTLR4 jar
- runtime C++ ANTLR4

### 1.2 Fichiers à inclure

Avant de démarrer il faut préparer quelques fichiers qui ne sont pas dans ce repo:

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
- paramètres types `int` uniquement
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
- bit-a-bit: `&`, `^`, `|`
- logiques paresseuses: `&&`, `||`
- appels de fonctions dans les expressions (fonctions retournant `int`)

La priorité des opérateurs est gérée dans la grammaire (`ifcc.g4`) via plusieurs niveaux de règles.

### 2.4 Commentaires et directives

La grammaire ignore:

- commentaires de type `/* ... */`, `// ...`
- directives préprocesseur de type `#...`
- espaces et retours ligne non significatifs

### 2.5 Non supporté dans cette version

Les elements facultatifs suivants ont été volontairement non traité dans le scope actuel:

- `double` et conversions implicites associees
- propagation de constantes (simple ou data-flow)
- tableaux 1D
- pointeurs
- chaines de caractères representees par des tableaux de `char`
- métadonnées internes legacy liées aux pointeurs/tableaux (supprimées des structures)

Nous avons également décidé de ne pas traiter les fonctionnalités non prioritaires et déconseillées.

## 3. Vérifications sémantiques implémentées

Le visiteur `SymbolVisitor` construit une table des symboles (`SymbolTable`) et applique les contrôles suivants:

- erreur si une variable est déclarée plusieurs fois dans un même bloc
- erreur si une variable est utilisée sans déclaration
- erreur si une affectation vise une variable non déclarée
- erreur si une fonction est definie plusieurs fois
- erreur si la fonction `main` est absente
- erreur si un paramêtre de fonction est dupliqué
- erreur si appel d'une fonction non definie
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

- la detection de division par zero ne couvre pas tous les cas dynamiques (exemple : (0) et variable qui vaut 0 n'est pas traité)
- les warnings (variable non utilisée, division/modulo par zero constant) ne bloquent pas la compilation

## 4. Génération de code

### 4.1 Architecture

La generation repose sur une hiérarchie de backend:

- `backend` (abstrait)
- `x86Backend` (x86-64, AT&T)
- `ArmBackend` (AArch64, Apple Silicon)

Le backend est choisi via un flag en ligne de commande lors de l'exécution de `ifcc`:

```bash
ifcc -target x86   # génère du code x86-64
ifcc -target arm   # génère du code AArch64
```

Sans specification de `-target`, la cible par défaut est x86.

Le backend gère plusieurs fonctions dans un même programme:

- un CFG est génère par fonction
- les labels assembleur internes sont prefixes par fonction (évite les collisions)
- les paramètres sont recuperes depuis les registres d'appel
- la pile locale x86 est réservée explicitement (frame) pour supporter appels imbriqués et récursion

## 5. Structure du projet

```text
compiler/
  ifcc.g4              # grammaire ANTLR
  main.cpp             # point d'entrée CLI
  src/SymbolTable.h    # symboles variables + signatures de fonctions
  src/SymbolVisitor.*  # vérifications sémantiques
  src/IRVisitor.*      # AST -> IR/CFG
  src/backend.*        # generation assembleur x86-64 / AArch64
  Makefile             # build ANTLR + C++
  config*.mk           # config machine (ANTLR jar/runtime)

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

Chaque categorie (ex: `add/`, `comparison/`, etc.) contient en general trois sous-dossiers:

- `valid/`: programmes C valides qui doivent être compiles et executes correctement par ifcc
- `invalid/`: programmes C invalides qui doivent être rejetes par ifcc avec une erreur appropriee
- `not_implémentéd/`: programmes en attente d'implémentation ou en cours de développément

### 7.2 Workflow de développément des tests

Les nouveaux tests sont initialement places dans `not_implémentéd/` d'une categorie. Une fois l'implémentation terminée et le comportement valide, le test est déplacé vers `valid/` ou `invalid/`.

Couverture actuelle (sélection):

- programmes valides: constantes, variables, priorités, parenthèses, chaines d'operations
- opérateurs testés: `+ - * / %`, bitwise, comparaisons, égalité, logiques paresseux
- déclarations multiples avec assignment sur la même ligne (`declare-multiple-assign`)
- structures de contrôle: `if`, `if/else`, `while`, `switch`
- appels de fonctions (definition, arité, paramètres)
- récursion (factorielle, fibonacci, récursion mutuelle)
- cas invalides syntaxiques: opérateurs incomplets, point-virgule manquant, tokens invalides
- cas invalides sémantiques: variable non déclarée, variable redéclarée, appel fonction inconnue, arité incorrecte, incohérences `return`

### 7.3 Exécuter les tests

Deux méthodes principales sont disponibles:

1) Wrapper `testing_wrapper.py`

Dans le répertoire `compiler`, lancer:

```bash
make tests
```

2) Script `ifcc-test.py`

Le script (fourni par les professeurs, placé à la racine, non versionné) peut être invoqué directement:

```bash
python3 ifcc-test.py testfiles
python3 ifcc-test.py testfiles/logic
```

Astuce: pour cibler un seul fichier, passer son chemin en argument.

## 8. Répartition des tâches

Pour voir la répartition des tâches et l'avancement du projet, consulter le document [Google Sheets de suivi d'équipe](https://docs.google.com/spreadsheets/d/1ENracvxvhSuJ-HFS7LvaUH-EdFnAlYx0OItCOUCQky8/edit?gid=0#gid=0)

## 9. Pourquoi certains tests peuvent diverger

### 9.1 `getchar()` / `putchar()`

Dans le cas des tests de la fonction getchar() nous ne pouvons pas inclure dans les testfiles des tests qui attendent un char indefiniement. Nous les avons cependant testés en apparté et en entrant une valeur nous même, les tests rendent des resultats valides. Exemple de test qui rentre dans ce cas la :
int main() { int x; x = getchar(); return x; }


De plus le comportement peut varier selon l'environnement (WSL, Linux natif, macOS/clang). Sous notre version de gcc (clang) sous mac putchar n'existe pas, gcc le considere comme un programme invalide alors que ce n'est pas le cas.

### 9.2 Dossier `testfiles/undefined`

Ces programmes ont un comportement C non defini. Il est normal d'observer des divergences entre ifcc et gcc/clang selon plate-forme et version de compilateur.

### 9.3 `testfiles-div-invalid-37_division_0` / `testfiles-div-invalid-37_division_0`
Pour une raison qui nous est inconnue, ces tests marchent sous linux/wsl mais pas sous mac (C'est probablement du au fait que notre gcc sous mac n'est pas exactement le même que celui sous linux)

De manière plus général, selon l'outil systeme utilise (`gcc` vs `clang`, version libc, ABI), certains verdicts de tests peuvent differer tout en restant cohérents avec les limites du langage C non defini ou avec des extensions de compilateur.



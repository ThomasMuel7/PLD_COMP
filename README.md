# PLD-Comp - Compilateur C subset (Rendu Final)

Ce depot contient notre compilateur ifcc developpe en C++ avec ANTLR4 dans le cadre du projet PLD-Comp (INSA Lyon, 4IF3 Hexanome 1 - Carl Habsieger, Charles Cesbron, Alexandre Didier, Hugo Marin, Thomas Muel, 2025-2026).

## 1. Configuration

### 1.1 Dependances

- g++ (C++17)
- java (pour ANTLR)
- ANTLR4 jar
- runtime C++ ANTLR4

### 1.2 Fichiers a inclure

Avant de demarrer il faut preparer quelques fichiers qui ne sont pas dans ce repo:

- copier un modele de configuration dans `compiler/config.mk` (par exemple `config-wsl-2025.mk` ou `config-IF501.mk`) et ajuster `ANTLRJAR`, `ANTLRINC` et `ANTLRLIB` a votre machine
- recuperer le script de tests `ifcc-test.py` et le placer a la racine du depot

## 2. Perimetre actuellement implemente

### 2.1 Forme de programme acceptee

Le parser accepte actuellement des programmes composes d'une ou plusieurs fonctions:

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
- parametres types `int` uniquement
- type de retour de fonction: `int` ou `void`
- presence obligatoire d'une fonction `main`
- appels de fonctions supportes (avec controle d'arite)
- structures de controle supportees (`if`, `else`, `while`, blocs imbriques, `switch/case/default`)
- gestion de `break` et `continue`

### 2.2 Instructions supportees

- declaration: `int a;` et `int a, b, c;`
- declaration avec initialisation: `int a = 3;`, `int a, b = 1, c;`
- affectation: `a = expr;`
- affectations composees: `+=`, `-=`, `*=`, `/=`
- increment/decrement: `++`, `--` (pre/post)
- appels de fonctions: `f(...)` en instruction
- retour: `return expr;` et `return;`
- `if`, `if/else`, `while`, `switch/case/default`, blocs `{ ... }`

### 2.3 Expressions supportees

Les expressions suivantes sont parsees et generees:

- variables
- constantes entieres
- constantes caractere (`'a'`, `'0'`, etc.)
- parentheses
- unaires `-` (negation) et `!`
- arithmétique `*`, `/`, `%`
- arithmétique `+`, `-`
- comparaisons: `>`, `<`, `>=`, `<=`
- egalite: `==`, `!=`
- bit-a-bit: `&`, `^`, `|`
- logiques paresseuses: `&&`, `||`
- appels de fonctions dans les expressions (fonctions retournant `int`)

La priorite des operateurs est geree dans la grammaire (`ifcc.g4`) via plusieurs niveaux de regles.

### 2.4 Commentaires et directives

La grammaire ignore:

- commentaires de type `/* ... */`, `// ...`
- directives preprocesseur de type `#...`
- espaces et retours ligne non significatifs

### 2.5 Non supporte dans cette version

Les elements facultatifs suivants ont été volontairement non traité dans le scope actuel:

- `double` et conversions implicites associees
- propagation de constantes (simple ou data-flow)
- tableaux 1D
- pointeurs
- chaines de caracteres representees par des tableaux de `char`
- metadonnees internes legacy liees aux pointeurs/tableaux (supprimees des structures)

Nous avons également décidé de ne pas traiter les fonctionnalités non prioritaires et déconseillées.

## 3. Verifications semantiques implementees

Le visiteur `SymbolVisitor` construit une table des symboles (`SymbolTable`) et applique les controles suivants:

- erreur si une variable est declaree plusieurs fois dans un meme bloc
- erreur si une variable est utilisee sans declaration
- erreur si une affectation vise une variable non declaree
- erreur si une fonction est definie plusieurs fois
- erreur si la fonction `main` est absente
- erreur si un parametre de fonction est duplique
- erreur si appel d'une fonction non definie
- erreur si nombre d'arguments incoherent avec la signature
- erreur si une fonction `void` est utilisee dans une expression
- erreur si `return;` dans une fonction `int`
- erreur si `return expr;` dans une fonction `void`
- erreur si `break` hors boucle/switch
- erreur si `continue` hors boucle
- erreur si `switch` contient des `case` dupliques
- erreur si `switch` contient plusieurs `default`
- warning si une variable est declaree mais non utilisee
- warning si division ou modulo par constante `0` (cas detecte statiquement)

Notes:

- la detection de division par zero ne couvre pas tous les cas dynamiques (exemple : (0) et variable qui vaut 0 n'est pas traité)
- les warnings (variable non utilisee, division/modulo par zero constant) ne bloquent pas la compilation

## 4. Generation de code

### 4.1 Architecture

La generation repose sur une hierarchie de backend:

- `backend` (abstrait)
- `x86Backend` (x86-64, AT&T)
- `ArmBackend` (AArch64, Apple Silicon)

Le backend est choisi via un flag en ligne de commande lors de l'execution de `ifcc`:

```bash
ifcc -target x86   # genere du code x86-64
ifcc -target arm   # genere du code AArch64
```

Sans specification de `-target`, la cible par defaut est x86.

Le backend gere plusieurs fonctions dans un meme programme:

- un CFG est genere par fonction
- les labels assembleur internes sont prefixes par fonction (evite les collisions)
- les parametres sont recuperes depuis les registres d'appel
- la pile locale x86 est reservee explicitement (frame) pour supporter appels imbriques et recursion

## 5. Structure du projet

```text
compiler/
  ifcc.g4              # grammaire ANTLR
  main.cpp             # point d'entree CLI
  src/SymbolTable.h    # symboles variables + signatures de fonctions
  src/SymbolVisitor.*  # verifications semantiques
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

Le jeu de tests est enrichi en continu avant l'implementation d'une nouvelle fonctionnalite.

## 6. Build et execution

### 6.1 Compilation du compilateur

```bash
cd compiler
make
```

### 6.2 Utilisation

Le compilateur ecrit l'assembleur sur stdout.

```bash
cd compiler
./ifcc ../testfiles/const/valid/1_return42.c > out.s
gcc out.s -o out
./out
echo $?
```

## 7. Strategie de tests actuelle

Le dossier `testfiles/` contient des cas organises par categories d'operateurs ou fonctionnalites.

### 7.1 Structure des tests

Chaque categorie (ex: `add/`, `comparison/`, etc.) contient en general trois sous-dossiers:

- `valid/`: programmes C valides qui doivent etre compiles et executes correctement par ifcc
- `invalid/`: programmes C invalides qui doivent etre rejetes par ifcc avec une erreur appropriee
- `not_implemented/`: programmes en attente d'implementation ou en cours de developpement

### 7.2 Workflow de developpement des tests

Les nouveaux tests sont initialement places dans `not_implemented/` d'une categorie. Une fois l'implementation terminee et le comportement valide, le test est deplace vers `valid/` ou `invalid/`.

Couverture actuelle (selection):

- programmes valides: constantes, variables, priorites, parentheses, chaines d'operations
- operateurs testes: `+ - * / %`, bitwise, comparaisons, egalite, logiques paresseux
- structures de controle: `if`, `if/else`, `while`, `switch`
- appels de fonctions (definition, arite, parametres)
- recursion (factorielle, fibonacci, recursion mutuelle)
- cas invalides syntaxiques: operateurs incomplets, point-virgule manquant, tokens invalides
- cas invalides semantiques: variable non declaree, variable redeclaree, appel fonction inconnue, arite incorrecte, incoherences `return`

### 7.3 Executer les tests

Deux methodes principales sont disponibles:

1) Wrapper `testing_wrapper.py`

Dans le repertoire `compiler`, lancer:

```bash
make tests
```

2) Script `ifcc-test.py`

Le script (fourni par les professeurs, place a la racine, non versionne) peut etre invoque directement:

```bash
python3 ifcc-test.py testfiles
python3 ifcc-test.py testfiles/logic
```

Astuce: pour cibler un seul fichier, passer son chemin en argument.

## 8. Repartition des taches

Pour voir la repartition des taches et l'avancement du projet, consulter le document [Google Sheets de suivi d'equipe](https://docs.google.com/spreadsheets/d/1ENracvxvhSuJ-HFS7LvaUH-EdFnAlYx0OItCOUCQky8/edit?gid=0#gid=0)

## 9. Pourquoi certains tests peuvent diverger

### 9.1 `getchar()` / `putchar()`

Dans le cas des tests de la fonction getchar() nous ne pouvons pas inclure dans les testfiles des tests qui attendent un char indefiniement. Nous les avons cependant testés en apparté et en entrant une valeur nous même, les tests rendent des resultats valides. Exemple de test qui rentre dans ce cas la :
int main() { int x; x = getchar(); return x; }


De plus le comportement peut varier selon l'environnement (WSL, Linux natif, macOS/clang). Sous notre version de gcc (clang) sous mac putchar n'existe pas, gcc le considere comme un programme invalide alors que ce n'est pas le cas.

### 9.2 Dossier `testfiles/undefined`

Ces programmes ont un comportement C non defini. Il est normal d'observer des divergences entre ifcc et gcc/clang selon plate-forme et version de compilateur.

### 10.3 `testfiles-div-invalid-37_division_0` / `testfiles-div-invalid-37_division_0`
Pour une raison qui nous est inconnue, ces tests marchent sous linux/wsl mais pas sous mac (C'est probablement du au fait que notre gcc sous mac n'est pas exactement le même que celui sous linux)

De manière plus général, selon l'outil systeme utilise (`gcc` vs `clang`, version libc, ABI), certains verdicts de tests peuvent differer tout en restant coherents avec les limites du langage C non defini ou avec des extensions de compilateur.

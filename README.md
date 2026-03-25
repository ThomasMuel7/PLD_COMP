# PLD-Comp - Compilateur C subset (Rendu intermédiaire)

Ce dépôt contient notre compilateur `ifcc` développé en C++ avec ANTLR4 dans le cadre du projet PLD-Comp (INSA Lyon, 4IF3 Hexanome 1 (Carl Habsieger, Charles Cesbron, Alexandre Didier, Hugo Marin, Thomas Muel), 2025-2026).

## 1. Configuration

### 1.1 Dépendances

- `g++` (C++17)
- `java` (pour ANTLR)
- ANTLR4 jar
- runtime C++ ANTLR4

### 1.2 Fichiers à inclure

Avant de démarrer il faut préparer quelques fichiers qui **ne sont pas dans ce repo** :

- copiez un modèle de configuration dans `compiler/config.mk` (par exemple `config-wsl-2025.mk` ou `config-IF501.mk`) et ajustez les variables `ANTLRJAR`, `ANTLRINC` et `ANTLRLIB` à votre machine.
- récupérez le script de tests `ifcc-test.py` et placez‑le à la racine du dépôt.

## 2. Périmètre actuellement implémenté

### 2.1 Forme de programme acceptée

Le parser accepte actuellement des programmes composés d'une ou plusieurs fonctions :

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

Contraintes actuelles :

- paramètres typés `int` uniquement
- type de retour de fonction : `int` ou `void`
- présence obligatoire d'une fonction `main`
- appels de fonctions supportés (avec contrôle d'arité)
- structures de contrôle supportées (`if`, `else`, `while`, blocs imbriqués)

### 2.2 Instructions supportées

- déclaration : `int a;` et `int a, b, c;`
- affectation : `a = expr;`
- affectations composées : `+=`, `-=`, `*=`, `/=`
- appels de fonctions : `f(...)` en instruction
- retour : `return expr;` et `return;`
- `if`, `if/else`, `while`, blocs `{ ... }`

### 2.3 Expressions supportées

Les expressions suivantes sont parsées et générées :

- variables
- constantes entières
- constantes caractère (`'a'`, `'0'`, etc.)
- parenthèses
- unaires `-` (négation)
- unaire `!`
- `*`, `/`, `%`
- `+`, `-`
- comparaisons : `>`, `<`, `>=`, `<=`
- égalité : `==`, `!=`
- bit-à-bit : `&`, `^`, `|`
- logiques parresseuses : `&&`, `||`
- appels de fonctions dans les expressions (pour les fonctions retournant `int`)

La priorité des opérateurs est gérée dans la grammaire (`ifcc.g4`) via plusieurs niveaux de règles.

### 2.4 Commentaires et directives

La grammaire ignore :

- commentaires de type `/* ... */`, `// ... \n`
- directives préprocesseur de type `#...`
- espaces

## 3. Vérifications sémantiques implémentées

Le visiteur `SymbolVisitor` construit une table des symboles (`SymbolTable`) et applique les contrôles suivants :

- erreur si une variable est déclarée plusieurs fois
- erreur si une variable est utilisée sans déclaration
- erreur si une affectation vise une variable non déclarée
- erreur si une fonction est définie plusieurs fois
- erreur si la fonction `main` est absente
- erreur si un paramètre de fonction est dupliqué
- erreur si appel d'une fonction non définie
- erreur si nombre d'arguments incohérent avec la signature
- erreur si une fonction `void` est utilisée dans une expression
- erreur si `return;` dans une fonction `int`
- erreur si `return expr;` dans une fonction `void`
- warning si une variable est déclarée mais non utilisée
- warning si division ou modulo par constante `0` (cas détecté statiquement)

Notes :

- la détection de division par zéro ne couvre pas tous les cas dynamiques (ex. : variable valant 0 ou (0) ou (5-5))
- les warnings (variable non utilisée, division par 0) ne bloquent pas la compilation

## 4. Génération de code

### 4.1 Architecture
La génération repose sur une hiérarchie :

backend (abstrait)
├── x86Backend   (x86-64, AT&T)
└── ArmBackend   (AArch64, Apple Silicon)

Le backend utilisé est choisi via un flag en ligne de commande lors de l’exécution de ifcc :
ifcc -target x86   # génère du code x86-64
ifcc -target arm   # génère du code AArch64
Selon ce flag, le compilateur instancie automatiquement le backend correspondant et génère le code assembleur adapté à l’architecture cible.
Sans spécification de `-target`, la cible par défaut est `x86`.
En utilisant `make tests`, `testing_wrapper.py` détecte automatiquement l'architecture utilisée par le shell.

Le backend gère maintenant plusieurs fonctions dans un même programme :

- un CFG est généré par fonction
- les labels assembleur internes sont préfixés par fonction (évite les collisions)
- les paramètres sont récupérés depuis les registres d'appel
- la pile locale x86 est réservée explicitement (frame) pour supporter les appels imbriqués et la récursion

## 5. Structure du projet

```text
compiler/
    ifcc.g4              # grammaire ANTLR
    main.cpp             # point d'entrée CLI
    src/SymbolTable.h    # symboles variables + signatures de fonctions
    src/SymbolVisitor.*  # vérifications sémantiques
    src/IRVisitor.*      # AST -> IR/CFG
    src/backend.*        # génération assembleur x86-64 / AArch64
    Makefile             # build ANTLR + C++
    config*.mk           # config machine (ANTLR jar/runtime)

testfiles/
    add/                 # tests pour l'addition
        invalid/         # tests invalides (doivent échouer)
        not_implemented/ # tests en cours d'implémentation
        valid/           # tests valides (doivent réussir)
    comparison/          # tests pour les comparaisons
        ...
    const/               # tests pour les constantes
        ...
    div/                 # tests pour la division
        ...
    equality/            # tests pour l'égalité
        ...
    logic/               # tests pour les opérateurs logiques
        ...
    minus/               # tests pour la soustraction
        ...
    mod/                 # tests pour le modulo
        ...
    mul/                 # tests pour la multiplication
        ...
    op-assignment/       # tests pour les opérateurs d'affectation
        ...
    priority/            # tests pour la priorité des opérateurs
        ...
    unary/               # tests pour les opérateurs unaires
        ...
    functions/           # tests liés aux fonctions
        ...
    # Le jeu de tests est enrichi en continu et avant l'implémentation d'une nouvelle fonctionnalité pour le compilateur
```

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
./ifcc ../testfiles/1_return42.c > out.s
gcc out.s -o out
./out
echo $?
```

## 7. Stratégie de tests actuelle

Le dossier `testfiles/` contient des cas organisés par catégories d'opérateurs ou fonctionnalités.

### 7.1 Structure des tests

Chaque catégorie (ex. : `add/`, `comparison/`, etc.) contient trois sous-dossiers :
- `valid/` : programmes C valides qui doivent être compilés et exécutés correctement par `ifcc`.
- `invalid/` : programmes C invalides qui doivent être rejetés par `ifcc` avec une erreur appropriée.
- `not_implemented/` : programmes en attente d'implémentation ou en cours de développement.

### 7.2 Workflow de développement des tests

Les nouveaux tests sont initialement placés dans le dossier `not_implemented/` d'une catégorie donnée. Une fois que l'implémentation correspondante est terminée et que le test passe (ou échoue comme attendu), le fichier est déplacé vers `valid/` ou `invalid/` selon le cas.

Couverture actuelle (sélection) :

- programmes valides : constantes, variables, priorités, parenthèses, chaînes d'opérations
- opérateurs testés : `+ - * / %`
- structures de contrôle : `if`, `if/else`, `while`
- appels de fonctions (définition, arité, paramètres)
- récursion (factorielle, fibonacci, récursion mutuelle)
- cas invalides syntaxiques : tokens invalides, opérateurs incomplets, point-virgule manquant
- cas invalides sémantiques : variable non déclarée, variable redéclarée, appel fonction inconnue, arité incorrecte, incohérences `return`
- cas de robustesse : division/modulo par zéro (constante et via variable)

Exemples de fichiers :

- valides : `1_return42.c`, `14_combine_op.c`, `32_operator_priority.c`, `45_combine_all_ops.c`, `55_char.c`
- valides fonctions : `functions/valid/04_fibonacci_recursive.c`, `functions/valid/05_factorial_recursive.c`
- invalides fonctions : `functions/invalid/01_undefined_function.c`, `functions/invalid/03_wrong_arity_too_many.c`

### 7.3 Exécuter les tests

Deux méthodes principales sont disponibles :

1. **Wrapper `testing_wrapper.py`**  
   Dans le répertoire `compiler`, construisez d'abord le compilateur puis lancez la cible :

   ```bash
   cd compiler
   make tests          # compile et invoque ifcc-test.py sur le dossier testfiles
   ```

2. **Script `ifcc-test.py`**  
   Le script, fourni par les professeurs et placé à la racine (non versionné), peut être invoqué directement :

   ```bash
   python3 ifcc-test.py testfiles       # exécute tous les cas
   python3 ifcc-test.py testfiles/logic # ou une catégorie précise
   ```

> **Astuce :** pour cibler un seul fichier, passez simplement son chemin en argument.



## 8. Avancement par rapport au backlog PLD-Comp

État estimé :

- 4.1 Prise en main du squelette : fait
- 4.2 Compilateur v0 (`return constante`) : fait
- 4.3 Base de tests : en place et enrichie (123 tests)
- 4.4 Variables en mémoire : fait (offsets pile)
- 4.5 Table des symboles + vérifications statiques : fait
- 4.6 Compilateur bout en bout pour langage restreint : fait
- 4.7 Expressions arithmétiques : fait pour le cœur attendu (+ extensions déjà codées)
- 4.8 Structures de contrôle (`if`, `while`) : fait
- 4.9 Appels de fonctions et paramètres : fait
- 4.10 Fonctions `int/void` + cohérence des `return` : fait
- 4.11 Récursion : fait (validée par tests dédiés)

## 9. Répartition des tâches

Pour voir la répartition des tâches et l'avancement du projet, consultez le document Google Sheets suivant :  
[Répartition des tâches PLD-Comp](https://docs.google.com/spreadsheets/d/1ENracvxvhSuJ-HFS7LvaUH-EdFnAlYx0OItCOUCQky8/edit?gid=0#gid=0)

## 10. Pourquoi certains tests ne fonctionnent pas

### 10.1 getchar(), putchar()
Dans le cas des tests de la fonction getchar() nous ne pouvons pas inclure dans les testfiles des tests qui attendent un char indefiniement. Nous les avons cependant testés en apparté et en entrant une valeur nous même, les tests rendent des resultats valides. 
Exemple de test qui rentre dans ce cas la : 

int main() {
    int x;
    x = getchar();
    return x;
}

### 10.2 Dossier de tests "testfiles/undefined"

Ces programmes n'ont pas de comportement défini et peuvent passer la compilation gcc ou pas (gcc accepte des codes illogiques par legacy).

### 10.3 testfiles-div-invalid-37_division_0 et testfiles-div-invalid-37_division_0

Pour une raison qui nous est inconnue, ces tests marchent sous linux/wsl mais pas sous mac (C'est probablement du au fait que notre gcc sous mac n'est pas exactement le même que celui sous linux) 

### 10.4 testfiles-while-invalid-05_while_double_semicolon

Nous n'avons pas pris en compte la possibilité d'avoir une instruction vide. Donc le fait d'avoir deux ";" qui se suivent n'est pas une erreur pour gcc mais ça en est une pour notre compilateur.

### 10.5 testfiles-functions-invalid-05_int_function_empty_return

Cas volontairement invalide selon nos règles sémantiques strictes : `return;` est rejeté dans une fonction `int`.
GCC peut accepter ce code en extension/legacy, ce qui peut créer un désalignement des verdicts dans le script de comparaison.

### 10.6 testfiles-functions-invalid-06_void_function_returns_value

Cas volontairement invalide selon nos règles sémantiques strictes : `return expr;` est rejeté dans une fonction `void`.
GCC peut accepter ce code en extension/legacy, ce qui peut créer un désalignement des verdicts dans le script de comparaison.
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

Le parser accepte actuellement des programmes de la forme :

```c
int main() {
    // statements
    return // ...
}
```

Contraintes actuelles :

- une seule fonction `main`
- pas de paramètres de fonction
- pas de `if`, `else`, `while`, appels de fonctions, ni blocs imbriqués

### 2.2 Instructions supportées

- déclaration : `int a;` et `int a, b, c;`
- affectation : `a = expr;`
- retour : `return expr;`

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
- warning si une variable est déclarée mais non utilisée
- warning si division ou modulo par constante `0` (cas détecté statiquement)

Notes :

- la détection de division par zéro ne couvre pas tous les cas dynamiques (ex. : variable valant 0 ou (0) ou (5-5))
- les warnings (variable non utilisée, division par 0) ne bloquent pas la compilation

## 4. Génération de code

`CodeGenVisitor` produit de l'assembleur x86-64 (syntaxe AT&T) sur la sortie standard.

Éléments implémentés :

- étiquette d'entrée `main` (et `_main` sur macOS)
- prologue/épilogue minimal (`pushq %rbp`, `movq %rsp, %rbp`, `popq %rbp`, `ret`)
- stockage des variables locales par offsets négatifs relatifs à `%rbp`
- évaluation des expressions dans `%eax`
- opérations arithmétiques et bit-à-bit
- division/modulo via `cltd` + `idivl`
- comparaisons avec production de booléen `0/1`

## 5. Structure du projet

```text
compiler/
    ifcc.g4              # grammaire ANTLR
    main.cpp             # point d'entrée CLI
    SymbolTable.h        # structure des symboles
    SymbolVisitor.*      # vérifications sémantiques
    CodeGenVisitor.*     # génération assembleur x86-64
    Makefile             # build ANTLR + C++
    config*.mk           # config machine (ANTLR jar/runtime)
    testing_wrapper.py   # script de test automatisé

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
    # Total : 123 programmes C de test (valides + invalides)
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

Le dossier `testfiles/` contient actuellement `123` cas, organisés par catégories d'opérateurs ou fonctionnalités.

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
- cas invalides syntaxiques : tokens invalides, opérateurs incomplets, point-virgule manquant
- cas invalides sémantiques : variable non déclarée, variable redéclarée
- cas de robustesse : division/modulo par zéro (constante et via variable)

Exemples de fichiers :

- valides : `1_return42.c`, `14_combine_op.c`, `32_operator_priority.c`, `45_combine_all_ops.c`, `55_char.c`
- invalides : `2_invalid_program.c`, `4_invalid_program_variable_not_declared.c`, `6_invalid_program_variable_declared_twice.c`, `36_missing_semicolon_fail.c`, `37_division_0.c`

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

## 9. Répartition des tâches

Pour voir la répartition des tâches et l'avancement du projet, consultez le document Google Sheets suivant :  
[Répartition des tâches PLD-Comp](https://docs.google.com/spreadsheets/d/1ENracvxvhSuJ-HFS7LvaUH-EdFnAlYx0OItCOUCQky8/edit?gid=0#gid=0)

## 10. Pourquoi certains tests ne fonctionnent pas

### 10.1 Dossier de tests "testfiles/undefined"

Ces programmes n'ont pas de comportement défini et peuvent passer la compilation gcc ou pas (gcc accepte des codes illogiques par legacy).

#### 10.2.1 Règle du "Maximal Munch" dans GCC

GCC concatène les deux tirets de `5 -- 5` en un seul token `--` (decrément). Appliqué à la constante `5`, cela génère une erreur : on ne peut pas décrémenter une lvalue non modifiable.

#### 10.2.2 Pourquoi la grammaire `ifcc.g4` l'accepte

La grammaire a une nouvelle règle pour `--` à la plus haute priorité; cependant, le parser ne connaît pas, ce qui renvoie également une erreur.

### 10.3 testfiles-div-invalid-37_division_0 et testfiles-div-invalid-37_division_0

Pour une raison qui nous est inconnue, ces tests marchent sous linux/wsl mais pas sous mac (C'est probablement du au fait que notre gcc sous mac n'est pas exactement le même que celui sous linux) 

### 10.4 testfiles-while-invalid-05_while_double_semicolon

Nous n'avons pas pris en compte la possibilité d'avoir une instruction vide. Donc le fait d'avoir deux ";" qui se suivent n'est pas une erreur pour gcc mais ça en est une pour notre compilateur.
# PLD-Comp - Compilateur C subset (Rendu intermediaire)

Ce depot contient notre compilateur `ifcc` developpe en C++ avec ANTLR4 dans le cadre du projet PLD-Comp (INSA Lyon, 4IF, 2025-2026).

## 1. Perimetre actuellement implemente

### 1.1 Forme de programme acceptee

Le parser accepte actuellement des programmes de la forme:

```c
int main() {
		// statements
}
```

Contraintes actuelles:

- une seule fonction `main`
- pas de parametres de fonction
- pas de `if`, `else`, `while`, appels de fonctions, ni blocs imbriques

### 1.2 Instructions supportees

- declaration: `int a;` et `int a, b, c;`
- affectation: `a = expr;`
- retour: `return expr;`

### 1.3 Expressions supportees

Les expressions suivantes sont parsees et generees:

- variables
- constantes entieres
- constantes caractere (`'a'`, `'0'`, etc.)
- parentheses
- unaire `-` (negation)
- unaire `!`
- `*`, `/`, `%`
- `+`, `-`
- comparaisons: `>`, `<`, `>=`, `<=`
- egalite: `==`, `!=`
- bit-a-bit: `&`, `^`, `|`

La priorite des operateurs est geree dans la grammaire (`ifcc.g4`) via plusieurs niveaux de regles.

### 1.4 Commentaires et directives

La grammaire ignore:

- commentaires de type `/* ... */`
- directives preprocesseur de type `#...`

## 2. Verifications semantiques implementees

Le visiteur `SymbolVisitor` construit une table des symboles (`SymbolTable`) et applique les controles suivants:

- erreur si une variable est declaree plusieurs fois
- erreur si une variable est utilisee sans declaration
- erreur si une affectation vise une variable non declaree
- warning si une variable est declaree mais non utilisee
- erreur si division ou modulo par constante `0` (cas detecte statiquement)

Notes:

- la detection de division par zero ne couvre pas tous les cas dynamiques (ex: variable valant 0)
- les warnings (variable non utilisee) ne bloquent pas la compilation

## 3. Generation de code

`CodeGenVisitor` produit de l'assembleur x86-64 (syntaxe AT&T) sur la sortie standard.

Elements implementes:

- etiquette d'entree `main` (et `_main` sur macOS)
- prologue/epilogue minimal (`pushq %rbp`, `movq %rsp, %rbp`, `popq %rbp`, `ret`)
- stockage des variables locales par offsets negatifs relatifs a `%rbp`
- evaluation des expressions dans `%eax`
- operations arithmetiques et bit-a-bit
- division/modulo via `cltd` + `idivl`
- comparaisons avec production de booleen `0/1`

## 4. Structure du projet

```text
compiler/
	ifcc.g4              # grammaire ANTLR
	main.cpp             # point d'entree CLI
	SymbolTable.h        # structure des symboles
	SymbolVisitor.*      # verifications semantiques
	CodeGenVisitor.*     # generation assembleur x86-64
	Makefile             # build ANTLR + C++
	config*.mk           # config machine (ANTLR jar/runtime)

testfiles/
	54 programmes C de test (valides + invalides)
```

## 5. Build et execution

### 5.1 Dependances

- `g++` (C++17)
- `java` (pour ANTLR)
- ANTLR4 jar
- runtime C++ ANTLR4

### 5.2 Configuration

Le `Makefile` inclut `compiler/config.mk`.

Verifier les trois variables:

- `ANTLRJAR`
- `ANTLRINC`
- `ANTLRLIB`

Des exemples sont fournis:

- `compiler/config-wsl-2025.mk`
- `compiler/config-IF501.mk`

### 5.3 Compilation du compilateur

```bash
cd compiler
make
```

### 5.4 Utilisation

Le compilateur ecrit l'assembleur sur stdout.

```bash
cd compiler
./ifcc ../testfiles/1_return42.c > out.s
gcc out.s -o out
./out
echo $?
```

## 6. Strategie de tests actuelle

Le dossier `testfiles/` contient actuellement `54` cas.

Couverture actuelle (selection):

- programmes valides: constantes, variables, priorites, parentheses, chaines d'operations
- operateurs testes: `+ - * / %`
- cas invalides syntaxiques: tokens invalides, operateurs incomplets, point-virgule manquant
- cas invalides semantiques: variable non declaree, variable redeclaree
- cas de robustesse: division/modulo par zero (constante et via variable)

Exemples de fichiers:

- valides: `1_return42.c`, `14_combine_op.c`, `32_operator_priority.c`, `45_combine_all_ops.c`, `55_char.c`
- invalides: `2_invalid_program.c`, `4_invalid_program_variable_not_declared.c`, `6_invalid_program_variable_declared_twice.c`, `36_missing_semicolon_fail.c`, `37_division_0.c`

## 7. Avancement par rapport au backlog PLD-Comp

Etat estime:

- 4.1 Prise en main du squelette: fait
- 4.2 Compilateur v0 (`return constante`): fait
- 4.3 Base de tests: en place et enrichie (54 tests)
- 4.4 Variables en memoire: fait (offsets pile)
- 4.5 Table des symboles + verifications statiques: fait
- 4.6 Compilateur bout en bout pour langage restreint: fait
- 4.7 Expressions arithmetiques: fait pour le coeur attendu (+ extensions deja codees)
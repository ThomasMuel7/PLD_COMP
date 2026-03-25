# IFCC - Sous-ensemble C simplifie (int uniquement)

Ce depot contient un compilateur C simplifie base sur ANTLR4 et C++.
Le perimetre a ete volontairement reduit pour ne garder que les fonctionnalites essentielles du projet.

## Perimetre supporte

Le compilateur ne supporte que:

- type de base `int` (32 bits)
- variables locales et parametres `int`
- constantes entieres et constantes caractere (`'a'`, `'0'`, ...)
- operations arithmetiques `+`, `-`, `*`, `/`, `%`
- operations bit-a-bit `|`, `&`, `^`
- comparaisons `==`, `!=`, `<`, `>`, `<=`, `>=`
- operations unaires `!` et `-`
- declarations partout dans les blocs
- affectation simple et composee (`=`, `+=`, `-=`, `*=`, `/=`)
- pre/post increment et decrement (`++x`, `x++`, `--x`, `x--`)
- appels `putchar(...)` et `getchar()`
- definition de fonctions avec type de retour `int` ou `void`
- verification de coherence des appels de fonctions (existence + arite)
- blocs `{ ... }`, portees lexicales et shadowing
- structures de controle `if`, `else`, `while`
- `return` dans les fonctions
- `switch/case/default`
- `break` et `continue`
- initialisation a la declaration, y compris declarateurs alternes sur une meme ligne
  - exemple: `int a, b = 1, c;`

## Ce qui n'est pas supporte (parmis les facultatifs car nous n'avons pas essayé d'implémenter les déconseillé et non prioritaire)

- `double` et conversions implicites associees
- propagation de constantes (locale ou data-flow)
- tableaux 1D
- pointeurs
- chaines de caracteres comme tableaux de `char`

## Verifications semantiques effectuees

Le compilateur detecte:

- variable utilisee sans declaration
- variable redeclaree dans un meme scope
- fonction appelee mais non definie
- arite d'appel incorrecte
- usage d'une fonction `void` dans une expression
- incoherence `return` (`return;` dans `int`, `return expr;` dans `void`)
- `break` hors boucle/switch
- `continue` hors boucle
- `switch` avec `case` dupliques
- `switch` avec plusieurs `default`

Le compilateur signale aussi en warning:

- variable declaree mais jamais utilisee
- division/modulo par zero lorsqu'un zero constant est detectable statiquement

## Architecture du compilateur

- Grammaire: `compiler/ifcc.g4`
- Analyse semantique: `compiler/src/SymbolVisitor.h`, `compiler/src/SymbolVisitor.cpp`
- Generation IR/CFG: `compiler/src/IRVisitor.h`, `compiler/src/IRVisitor.cpp`
- Backend assembleur: `compiler/src/backend.h`, `compiler/src/backend.cpp`
- Point d'entree CLI: `compiler/main.cpp`

## Build

Depuis WSL:

```bash
cd compiler
make
```

## Utilisation

```bash
cd compiler
./ifcc ../testfiles/const/valid/1_return42.c > out.s
gcc out.s -o out
./out
echo $?
```

## Tests

Les tests conserves couvrent uniquement le perimetre supporte.
Les suites `testfiles/pointers` et `testfiles/arrays` ont ete retirees.

Execution (depuis la racine du projet) :

```bash
python3 ifcc-test.py testfiles/add testfiles/assignment testfiles/bloc testfiles/break-continue testfiles/comparison testfiles/cond-loops testfiles/const testfiles/decl-init testfiles/div testfiles/equality testfiles/functions testfiles/getcharputchar testfiles/if testfiles/incdec testfiles/logic testfiles/minus testfiles/mod testfiles/mul testfiles/op-assignment testfiles/return testfiles/switch testfiles/unary testfiles/unused-var testfiles/while
```

## Cibles backend

Le backend est conserve avec reciblage multiple (x86, MSP430, ARM) dans l'architecture du projet.
Le front-end simplifie reste centree sur le langage int-only decrit ci-dessus.

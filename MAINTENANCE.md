# MAINTENANCE

Ce document decrit la version simplifiee du compilateur IFCC.
Objectif: maintenir un compilateur robuste sur un sous-ensemble C 

## 1. Pipeline complet

Le pipeline est strictement separe en 4 etages:

1. Parsing (ANTLR)
- fichier: `compiler/ifcc.g4`
- sortie: AST ANTLR

2. Analyse semantique
- fichiers: `compiler/src/SymbolVisitor.h`, `compiler/src/SymbolVisitor.cpp`
- sortie: `SymbolTable`, `FunctionTable`, erreurs/warnings semantiques

3. Generation IR + CFG
- fichiers: `compiler/src/IRVisitor.h`, `compiler/src/IRVisitor.cpp`
- sortie: liste de CFG (un CFG par fonction)

4. Backend
- fichiers: `compiler/src/backend.h`, `compiler/src/backend.cpp`
- sortie: assembleur cible

Point d'entree:
- `compiler/main.cpp`

## 2. Front-end: grammaire simplifiee

Fichier: `compiler/ifcc.g4`

### 2.1 Types et declarations

- types de fonctions: `int`, `void`
- parametres: `int VAR`
- declarateur local: `VAR` ou `VAR = expr`
- declaration multiple autorisee: `int a, b = 1, c;`

### 2.2 Expressions supportees

- parentheses
- pre/post inc-dec sur variable
- unaires `!` et `-`
- binaire `* / %`
- binaire `+ -`
- comparaisons `< <= > >=`
- egalite `== !=`
- bitwise `& ^ |`
- logiques paresseux `&& ||`
- affectation `= += -= *= /=` sur variable
- constante `INT`, `CHAR`
- variable
- appel de fonction

### 2.3 Expressions retirees explicitement

La grammaire ne contient plus:

- adressage `&x`
- dereferencement `*p`
- acces tableau `a[i]`
- ecriture indirecte `*p = ...`
- ecriture tableau `a[i] = ...`

C'est la barriere principale qui empeche les regressions pointeurs/tableaux.

## 3. Structures de donnees

### 3.1 VariableInfo

Fichier: `compiler/src/SymbolTable.h`

Champs utilises dans la version simplifiee:

- `index`: offset pile
- `isUsed`: marque si la variable a ete lue/ecrite
- `declLine`: ligne de declaration pour warning unused

Champs historiques conserves pour compatibilite interne backend:

- `isPointer`, `isArray`, `arrayLength`, `byteSize`

Dans ce mode simplifie, ces champs sont renseignes de maniere neutre (`false`, `0`, `4`).

### 3.2 FunctionInfo

- `returnType`: `Int` ou `Void`
- `arity`: nombre de parametres
- `paramUniqueNames`: noms internes suffixes
- `paramIsPointer`: conserve pour compatibilite backend, rempli en `false`

### 3.3 ScopeTable

`ScopeTable = vector<map<string,string>>`

- chaque map: nom source -> nom interne unique
- push/pop a l'entree/sortie de bloc
- resolution en partant du scope le plus interne

## 4. SymbolVisitor en detail

Fichier: `compiler/src/SymbolVisitor.cpp`

Le `SymbolVisitor` fait toute la coherence semantique du langage.

### 4.1 Etat global du visiteur

- `table`: table des variables
- `functionTable`: signatures des fonctions
- `scopeTable`: pile de scopes
- `currentOffset`: offset pile courant
- `uniqueVarId`: suffixe pour nommage unique
- `hasError`: drapeau d'erreur semantique
- `currentFunctionName`: fonction en cours
- `currentFunctionReturnType`: type de retour attendu
- `loopDepth`, `switchDepth`: controle contextuel de `break`/`continue`

### 4.2 Helper critiques

- `resolveVariable(name)`
  - role: trouver la variable visible la plus proche
  - comportement: scan du scope interne vers externe
  - retour: nom unique interne ou chaine vide

- `lookupVariableInfo(name)`
  - role: acces direct a `VariableInfo` depuis le nom source
  - utilise pour validations fines

- `anyToExprType(Any)`
  - role: convertir le retour des visiteurs d'expression en type interne
  - types internes: `TYPE_INT`, `TYPE_INVALID`

### 4.3 Fonction par fonction

- `visitProg`
  - predeclare toutes les fonctions (nom, type retour, arite)
  - detecte doublons de fonctions
  - impose presence de `main`
  - visite ensuite chaque fonction
  - lance `checkUnusedVariables` en fin de passe

- `checkUnusedVariables`
  - parcourt la table des symboles
  - emet un warning par variable jamais utilisee
  - inclut la ligne de declaration quand disponible

- `visitFunction_decl`
  - initialise contexte de fonction
  - ouvre le scope des parametres
  - ajoute chaque parametre en variable locale interne
  - reserve 4 octets par parametre
  - enregistre les noms uniques des parametres
  - visite le bloc de la fonction

- `visitBlock`
  - push/pop de scope
  - visite sequentielle des statements

- `visitDeclare_stmt`
  - verifie redeclaration locale
  - cree nom unique
  - reserve 4 octets en pile
  - si initialiseur present: verifie type expression et marque variable comme utilisee

- `visitReturn_stmt`
  - en fonction `void`: interdit `return expr;`
  - en fonction `int`: interdit `return;`
  - verifie que `return expr` est de type entier

- `visitVarExpr`
  - verifie declaration
  - marque la variable utilisee
  - retourne type `TYPE_INT`

- `visitAssignExpr`
  - verifie declaration lhs
  - visite rhs
  - verifie compatibilite (int-only)
  - retourne `TYPE_INT` (comme en C)

- `visitMultDivModExpr`
  - verifie types entiers des 2 operandes
  - warning si division/modulo par zero constant
  - retourne `TYPE_INT`

- `visitAddSubExpr`, `visitCompareExpr`, `visitEqualExpr`, `visitLogicBit*`, `visitLogic*`, `visitUnitaryExpr`
  - meme schema: visite operandes, controle type int-only, retourne `TYPE_INT`

- `visitPreIncDecVarExpr`, `visitPostIncDecVarExpr`
  - exige variable declaree
  - marque variable utilisee
  - retourne `TYPE_INT`

- `visitCallExpr`
  - cas builtins:
    - `putchar`: arite 1
    - `getchar`: arite 0
  - cas fonctions utilisateur:
    - fonction definie
    - arite correcte
    - fonction `void` non utilisable comme expression
  - visite tous les arguments et force type entier

- `visitBreak_stmt`, `visitContinue_stmt`
  - `break`: autorise seulement en boucle/switch
  - `continue`: autorise seulement en boucle

- `visitWhile_stmt`
  - condition de type entier
  - gestion de `loopDepth`

- `visitSwitch_stmt`
  - expression switch de type entier
  - detection `case` dupliques
  - detection multi `default`
  - gestion de `switchDepth`

## 5. IRVisitor en detail

Fichier: `compiler/src/IRVisitor.cpp`

Le `IRVisitor` transforme l'AST valide en IR lineaire, groupe par basic blocks dans un CFG.

### 5.1 Etat global du visiteur

- `cfgs`: tous les CFG produits
- `cfg`: CFG courant
- `current_bb`: bloc courant
- `bb_epilogue`: bloc de sortie fonction
- `table`: symboles (pour offsets des temporaires)
- `functionTable`: signatures connues
- `scopeTable`: resolution des noms pendant generation
- `currentOffset`: allocation pile des temporaires
- `tempCounter`: generation de `tmp0`, `tmp1`, ...
- `uniqueVarId`: suffixe de noms internes
- `breakTargets`, `continueTargets`: piles de cibles de controle

### 5.2 Helpers critiques

- `resolveVariable(name)`
  - meme strategie que le `SymbolVisitor`

- `createTemp()`
  - alloue un temporaire entier (4 octets)
  - enregistre l'offset en table symbole

- `gen_unique_id(ctx)`
  - derive un suffixe unique ligne/colonne
  - evite collisions de labels de basic blocks

### 5.3 Fonction par fonction

- `visitProg`
  - visite chaque fonction

- `visitFunction_decl`
  - cree CFG
  - cree bloc `prologue`, `body`, `epilogue`
  - mappe les parametres sur noms uniques
  - visite le bloc fonction
  - injecte un `return 0` implicite si aucun return explicite ne termine le flux

- `visitBlock`
  - push/pop scope
  - visite statements
  - stoppe la visite locale si le bloc courant est deja termine (sortie branchee)

- `visitDeclare_stmt`
  - enregistre noms uniques de variables locales
  - genere `copy` si initialiseur present

- `visitAssignExpr`
  - `=` -> IR `copy`
  - `+= -= *= /=` -> operation en place
  - retourne la variable lhs (semantique expression)

- `visitConstExpr`
  - cree un temporaire
  - genere `ldconst`

- `visitVarExpr`
  - retourne la variable resolue

- `visitPreIncDecVarExpr` / `visitPostIncDecVarExpr`
  - genere `ldconst 1`
  - puis `add`/`sub`
  - version post retourne l'ancienne valeur via temporaire copie

- `visitUnitaryExpr`
  - `-` -> IR `neg`
  - `!` -> IR `not_`

- `visitMultDivModExpr`, `visitAddSubExpr`, `visitCompareExpr`, `visitEqualExpr`, `visitLogicBit*`
  - visite lhs/rhs
  - cree un temporaire destination
  - emet l'operation IR correspondante

- `visitLogicANDExpr` (court-circuit)
  - evalue lhs
  - branche vers bloc false (resultat 0) ou bloc rhs
  - evalue rhs seulement si lhs vrai
  - fusion en bloc end

- `visitLogicORExpr` (court-circuit)
  - evalue lhs
  - branche vers bloc true (resultat 1) ou bloc rhs
  - evalue rhs seulement si lhs faux
  - fusion en bloc end

- `visitCallExpr`
  - construit la liste parametres IR (`func`, `dest`, `args...`)
  - emet instruction `call`
  - retourne `dest`

- `visitReturn_stmt`
  - si `return expr`: calcule `expr` et emet `ret expr`
  - si `return;`: emet `ret 0`
  - branche vers `bb_epilogue`

- `visitIf_stmt`
  - cree `bb_cond`, `bb_then`, optionnel `bb_else`, `bb_end`
  - condition stockee dans `test_var_name`
  - connecte les sorties conditionnelles

- `visitWhile_stmt`
  - cree `bb_cond`, `bb_body`, `bb_end`
  - pousse cibles `break/continue`
  - boucle sur `bb_cond`

- `visitBreak_stmt` / `visitContinue_stmt`
  - branche vers cible top de pile

- `visitSwitch_stmt`
  - construit chaine de dispatch comparant valeur switch a chaque case
  - supporte default
  - gere fallthrough
  - gere `break` via `breakTargets`

## 6. Backend: implications de la simplification

Le backend conserve du code historique multi-cas, mais en pratique:

- toutes les valeurs generees par front-end simplifie sont des entiers
- `paramIsPointer` reste a `false`
- les operations memoire indirecte (`addr`, `rmem`, `wmem`) ne sont plus emises

Ceci permet de garder un backend stable sans complexifier le front-end.

## 7. Politique de tests

Les tests conserves sont alignes sur le perimetre supporte.

Retire:

- `testfiles/pointers/`
- `testfiles/arrays/`
- tests relies explicitement a ces features dans d'autres categories

Conserve:

- operations int
- controle de flux
- fonctions
- I/O std (`putchar/getchar`)
- declarations/initialisations
- warnings `unused`

## 8. Commandes de maintenance

Build complet:

```bash
cd compiler
make clean
make -j4
```

Execution d'un sous-ensemble de tests supportes:

```bash
python3 ifcc-test.py testfiles/add testfiles/assignment testfiles/bloc testfiles/break-continue testfiles/comparison testfiles/cond-loops testfiles/const testfiles/decl-init testfiles/div testfiles/equality testfiles/functions testfiles/getcharputchar testfiles/if testfiles/incdec testfiles/logic testfiles/minus testfiles/mod testfiles/mul testfiles/op-assignment testfiles/return testfiles/switch testfiles/unary testfiles/unused-var testfiles/while
```

## 9. Regle d'evolution

Si une fonctionnalite est ajoutee:

1. Etendre la grammaire de maniere minimale
2. Etendre d'abord SymbolVisitor (regles semantiques)
3. Etendre ensuite IRVisitor (generation)
4. Ajouter tests `valid` + `invalid`
5. Mettre a jour README + MAINTENANCE

Ordre inverse interdit (ne jamais ajouter IR sans garde semantique explicite).

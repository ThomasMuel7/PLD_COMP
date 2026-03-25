# MAINTENANCE

Ce fichier est volontairement une documentation technique complète du projet.
Il peut être lu comme un manuel d'architecture + pseudo-code de référence.

Objectif principal:
- expliquer clairement le rôle de chaque module
- expliciter les variables d'état importantes
- donner un pseudo-code pour chaque fonction clé des passes sémantique et IR

---

## 1) Vue d'ensemble du compilateur

Le compilateur suit 4 étapes strictes.

1. Parsing
- Fichier: `compiler/ifcc.g4`
- Entrée: source C simplifié
- Sortie: AST ANTLR

2. Vérification sémantique
- Fichiers: `compiler/src/SymbolVisitor.h`, `compiler/src/SymbolVisitor.cpp`
- Entrée: AST
- Sortie: erreurs/warnings + tables (`SymbolTable`, `FunctionTable`)

3. Génération IR + CFG
- Fichiers: `compiler/src/IRVisitor.h`, `compiler/src/IRVisitor.cpp`
- Entrée: AST valide + tables sémantiques
- Sortie: un CFG par fonction, rempli en IR

4. Backend
- Fichiers: `compiler/src/backend.h`, `compiler/src/backend.cpp`
- Entrée: CFG/IR
- Sortie: assembleur final

Point d'entrée runtime du compilateur:
- `compiler/main.cpp`

---

## 2) Langage supporté (scope officiel)

### 2.1 Types et fonctions
- types de retour: `int`, `void`
- paramètres: uniquement `int x`
- arité max vérifiée: 6 arguments

### 2.2 Déclarations
- déclaration simple: `int a;`
- déclaration avec init: `int a = expr;`
- déclaration multiple: `int a, b = 1, c;`
- déclaration + assignment dans la même liste: `int a, b = 1, c = b + 2;`
- déclaration possible dans tout bloc

### 2.3 Expressions supportées
- constantes: `INT`, `CHAR`
- variable
- appel de fonction
- affectations: `=`, `+=`, `-=`, `*=`, `/=`
- unaires: `!`, `-`
- pré/post inc-dec: `++x`, `x++`, `--x`, `x--`
- binaires arithmétiques: `+ - * / %`
- comparaisons: `< <= > >= == !=`
- bitwise: `& ^ |`
- logiques court-circuit: `&& ||`

### 2.4 Contrôle de flux supporté
- `if / else`
- `while`
- `switch / case / default`
- `break`, `continue`
- `return`

### 2.5 Features explicitement non supportées (parmi les facultatifs)
- pointeurs (`*`, `&`)
- tableaux (`a[i]`, déclaration tableau)
- doubles
- propagation de constantes

Nous avons décidé de ne pas implémenter les fonctionnalités non prioritaires et déconseillées.

---

## 3) Structures de données

### 3.1 VariableInfo (`compiler/src/SymbolTable.h`)

```text
index       : offset pile de la variable
isUsed      : true si la variable est lue/écrite
declLine    : ligne de déclaration (pour warning unused)
```

Utilite pratique:
- `index` sert au placement stack
- `isUsed` + `declLine` servent au warning de fin d'analyse

### 3.2 FunctionInfo

```text
returnType       : Int ou Void
arity            : nombre de paramètres
paramUniqueNames : noms internes scopes des paramètres
```

Utilite pratique:
- verification d'appels (existence + arité)
- mapping cohérent entre front-end sémantique et IR/backend

### 3.3 ScopeTable

Definition logique:
`ScopeTable = vector<map<nom_source, nom_unique>>`

Utilite pratique:
- gere le masquage de variables (shadowing)
- permet une resolution de nom O(nombre_de_scopes)

Pseudo-code de resolution:

```text
resolve(name):
  pour i de dernier_scope à premier_scope:
    si name existe dans scope[i]:
      retourner scope[i][name]
  retourner not_found
```

---

## 4) SymbolVisitor: specification détaillée

`SymbolVisitor` est la passe de cohérence sémantique.
Chaque visiteur d'expression retourne un type logique interne:
- `TYPE_INT`
- `TYPE_INVALID`

### 4.1 Variables d'état (membres)

```text
table                     : SymbolTable globale des variables uniques
functionTable             : signatures des fonctions
scopeTable                : pile de scopes
currentOffset             : offset pile courant (décroît de 4 en 4)
uniqueVarId               : compteur pour suffixes _0, _1, _2
hasError                  : drapeau global d'erreur sémantique
currentFunctionName       : nom de la fonction en cours
currentFunctionReturnType : type attendu de return
loopDepth                 : profondeur while
switchDepth               : profondeur switch
```

### 4.2 Helpers

#### `resolveVariable(originalName)`
But: trouver le nom interne visible depuis le scope courant.

Pseudo-code:

```text
resolveVariable(name):
  pour i de scopeTable.size-1 à 0:
    si name dans scopeTable[i]:
      retourner scopeTable[i][name]
  retourner ""
```

#### `lookupVariableInfo(originalName)`
But: obtenir `VariableInfo*` en partant d'un nom source.

Pseudo-code:

```text
lookupVariableInfo(name):
  unique = resolveVariable(name)
  si unique == "": retourner null
  si unique absent de table: retourner null
  retourner &table[unique]
```

#### `checkUnusedVariables()`
But: emettre les warnings `unused` en fin de passe.

Pseudo-code:

```text
checkUnusedVariables():
  pour chaque (uniqueName, info) dans table:
    si info.isUsed == false:
      afficher warning avec nom de base et ligne
```

### 4.3 Fonctions visitées, une par une

#### `visitProg`
Responsabilité:
- prédéclarer toutes les signatures de fonction
- détectér doublons
- imposer présence de `main`
- visiter ensuite chaque fonction
- lancer le scan `unused`

Pseudo-code:

```text
visitProg(ctx):
  pour fn dans programme:
    si fn.nom deja dans functionTable:
      erreur doublon
    sinon:
      functionTable[fn.nom] = {returnType, arity, [], []}

  si main absente:
    erreur

  pour fn dans programme:
    visit(fn)

  checkUnusedVariables()
```

#### `visitFunction_decl`
Responsabilité:
- initialiser le contexte fonction
- créer le scope des paramètres
- allouer les paramètres en locals internes

Pseudo-code:

```text
visitFunction_decl(ctx):
  currentFunctionName = ctx.nom
  currentFunctionReturnType = parseReturnType(ctx.type)
  push scope local fonction

  reset paramUniqueNames pour cette signature

  pour param dans ctx.params:
    si param.nom deja dans scope courant:
      erreur
      continue

    unique = param.nom + "_" + uniqueVarId++
    scopeTable.top[param.nom] = unique

    currentOffset -= 4
    table[unique] = {index=currentOffset, isUsed=false, declLine=ligne}

    enregistrer unique dans functionTable

  visit(ctx.block)
  pop scope
```

#### `visitBlock`
Responsabilité:
- ouvrir/fermer un scope lexical

Pseudo-code:

```text
visitBlock(ctx):
  push scope
  pour stmt dans ctx.stmts:
    visit(stmt)
  pop scope
```

#### `visitDeclare_stmt`
Responsabilité:
- parcourir la liste des éléments de déclaration (`declare_elmt`)
- deleguer la validation/creation variable a `visitDeclare_elmt`

Pseudo-code:

```text
visitDeclare_stmt(ctx):
  pour elmt dans ctx.declare_elmt:
    visit(elmt)
```

#### `visitDeclare_elmt`
Responsabilité:
- gérer un élément `VAR` simple
- gérer un élément `assign_stmt` (déclaration + assignment dans la même ligne)

Pseudo-code:

```text
visitDeclare_elmt(ctx):
  si ctx est VAR:
    registerVariable(VAR, line)
    return

  si ctx est assign_stmt:
    registerVariable(VAR, line)
    visit(assign_stmt)
```

#### `visitAssign_stmt`
Responsabilité:
- typer/verifier l'initialisation de type `VAR = expr` a l'intérieur d'une déclaration.

Pseudo-code:

```text
visitAssign_stmt(lhs, rhs):
  unique = resolveVariable(lhs)
  tRhs = visit(rhs)
  si unique introuvable: erreur
  si tRhs incompatible: erreur
  table[unique].isUsed = true
  return TYPE_INT
```

#### `visitReturn_stmt`
Responsabilité:
- valider la cohérence `return` avec le type de la fonction courante.

Pseudo-code:

```text
visitReturn_stmt(ctx):
  si fonction void:
    si expr presente: erreur
    retourner

  # fonction int
  si expr absente: erreur
  sinon:
    t = visit(expr)
    si t non entier: erreur
```

#### `visitParensExpr`, `visitConstExpr`, `visitVarExpr`
Responsabilité:
- propagation des types de base.

Pseudo-code:

```text
visitParensExpr: return visit(expr)
visitConstExpr : return TYPE_INT

visitVarExpr(name):
  unique = resolveVariable(name)
  si unique introuvable: erreur; return TYPE_INVALID
  table[unique].isUsed = true
  return TYPE_INT
```

#### `visitAssignExpr`
Responsabilité:
- verifier lhs déclarée
- verifier rhs de type entier
- marquer lhs comme utilisée

Pseudo-code:

```text
visitAssignExpr(lhs, op, rhs):
  unique = resolveVariable(lhs)
  tRhs = visit(rhs)

  si unique introuvable: erreur; return TYPE_INVALID
  si tRhs incompatible: erreur

  table[unique].isUsed = true
  return TYPE_INT
```

#### `visitMultDivModExpr`
Responsabilité:
- verifier types des 2 operandes
- warning division/modulo par zero constant

Pseudo-code:

```text
visitMultDivModExpr(lhs, op, rhs):
  t1 = visit(lhs)
  t2 = visit(rhs)
  si t1 ou t2 incompatibles: erreur

  si rhs est constante ET (op == / OU op == % ) ET rhs == 0:
    warning

  return TYPE_INT
```

#### `visitPreIncDecVarExpr`, `visitPostIncDecVarExpr`
Responsabilité:
- exiger une variable déclarée, marquer usage.

Pseudo-code:

```text
visitPre/PostIncDecVarExpr(var):
  unique = resolveVariable(var)
  si introuvable: erreur; return TYPE_INVALID
  table[unique].isUsed = true
  return TYPE_INT
```

#### `visitUnitaryExpr`, `visitAddSubExpr`, `visitCompareExpr`, `visitEqualExpr`, `visitLogicBitANDExpr`, `visitLogicBitXORExpr`, `visitLogicBitORExpr`, `visitLogicANDExpr`, `visitLogicORExpr`
Responsabilité:
- schema homogene: verifier types des operandes, retourner int.

Pseudo-code commun:

```text
visitBinaryLike(lhs, rhs):
  t1 = visit(lhs)
  t2 = visit(rhs)
  si t1/t2 incompatibles: erreur
  return TYPE_INT

visitUnaryLike(expr):
  t = visit(expr)
  si t incompatible: erreur
  return TYPE_INT
```

#### `visitCallExpr`
Responsabilité:
- verifier contraintes d'appel builtins/user
- verifier arité
- interdire utilisation d'une fonction void comme expression
- verifier types d'arguments

Pseudo-code:

```text
visitCallExpr(funcName, args):
  argc = args.size
  si argc > 6: erreur

  si funcName == putchar:
    si argc != 1: erreur
  sinon si funcName == getchar:
    si argc != 0: erreur
  sinon:
    si fonction absente: erreur
    sinon si arité != argc: erreur
    si returnType == void ET appel utilise comme expression:
      erreur

  pour arg dans args:
    t = visit(arg)
    si t incompatible: erreur

  return TYPE_INT
```

#### `visitBreak_stmt`, `visitContinue_stmt`
Pseudo-code:

```text
visitBreak_stmt:
  si loopDepth == 0 ET switchDepth == 0: erreur

visitContinue_stmt:
  si loopDepth == 0: erreur
```

#### `visitWhile_stmt`
Pseudo-code:

```text
visitWhile_stmt(cond, body):
  t = visit(cond)
  si t incompatible: erreur
  loopDepth++
  visit(body)
  loopDepth--
```

#### `visitSwitch_stmt`
Pseudo-code:

```text
visitSwitch_stmt(expr, parts):
  t = visit(expr)
  si t incompatible: erreur

  switchDepth++
  seenCases = {}
  hasDefault = false

  pour part dans parts:
    si part est case:
      value = valeur case
      si value dans seenCases: erreur
      ajouter value
    sinon si part est default:
      si hasDefault: erreur
      hasDefault = true
    sinon si part est statement:
      visit(statement)

  switchDepth--
```

---

## 5) IRVisitor: specification détaillée

`IRVisitor` prend un AST semantiquement valide et produit l'IR execute par le backend.

### 5.1 Variables d'état (membres)

```text
cfgs            : tableau de CFG, un par fonction
cfg             : CFG courant
current_bb      : basic block courant
bb_epilogue     : block epilogue de la fonction courante
table           : SymbolTable partagee (variables + temporaires)
functionTable   : signatures connues
scopeTable      : resolution nom source -> nom unique
currentOffset   : offset pile courant pour temporaires
tempCounter     : compteur tmp0, tmp1, ...
uniqueVarId     : compteur suffixe pour noms locals
breakTargets    : pile de cibles break
continueTargets : pile de cibles continue
```

### 5.2 Helpers

#### `createTemp()`
But:
- reserver un entier temporaire
- l'enregistrer dans `table`

Pseudo-code:

```text
createTemp():
  name = "tmp" + tempCounter++
  currentOffset -= 4
  table[name] = {index=currentOffset, isUsed=true, declLine=0}
  return name
```

#### `resolveVariable(name)`
But:
- retrouver le nom unique d'une variable source.
- fallback: renvoyer `name` si non trouve (utile pour robustesse generation).

#### `gen_unique_id(ctx)`
But:
- fabriquer des labels uniques de blocs via ligne+colonne.

### 5.3 Fonctions visitées, une par une

#### `visitProg`

```text
visitProg(ctx):
  pour chaque fonction:
    visit(fonction)
```

#### `visitFunction_decl`
Responsabilité:
- créer CFG + blocs de base
- initialiser mapping des paramètres
- visiter le corps
- injecter un `ret 0` si le flot ne termine pas explicitement

Pseudo-code:

```text
visitFunction_decl(ctx):
  cfg = nouveau CFG(nom, estVoid)
  créer bb_prologue, bb_body, bb_epilogue
  connect prologue -> body
  current_bb = body

  push scope paramètres
  pour chaque param:
    unique = param.nom + "_" + uniqueVarId++
    scopeTable.top[param.nom] = unique
    cfg.paramVarNames.push(unique)
  visit(block)

  si current_bb n'a pas deja de sortie:
    tmp = createTemp(); ldconst tmp, 0; ret tmp; jump epilogue

  pop scope
```

#### `visitBlock`

```text
visitBlock(ctx):
  push scope
  pour stmt dans ctx:
    visit(stmt)
    si current_bb deja termine (sortie posee):
      break
  pop scope
```

#### `visitDeclare_stmt`

```text
visitDeclare_stmt(ctx):
  pour elmt dans ctx.declare_elmt:
    visit(elmt)
```

#### `visitDeclare_elmt`

```text
visitDeclare_elmt(ctx):
  si elmt = VAR:
    unique = VAR + "_" + uniqueVarId++
    scopeTable.top[VAR] = unique
    return

  si elmt = assign_stmt(VAR = expr):
    unique = VAR + "_" + uniqueVarId++
    scopeTable.top[VAR] = unique
    visitAssign_stmt(VAR = expr)
```

#### `visitAssign_stmt`

```text
visitAssign_stmt(lhs, rhs):
  r = visit(rhs)
  l = resolve(lhs)
  emit copy l, r
  return l
```

#### `visitAssignExpr`

```text
visitAssignExpr(lhs, op, rhs):
  r = visit(rhs)
  l = resolve(lhs)
  si op == "=": emit copy l, r
  sinon si op == "+=": emit add l, l, r
  sinon si op == "-=": emit sub l, l, r
  sinon si op == "*=": emit mul l, l, r
  sinon si op == "/=": emit div l, l, r
  return l
```

#### `visitConstExpr`, `visitVarExpr`, `visitParensExpr`

```text
visitConstExpr(c):
  t = createTemp()
  emit ldconst t, valeur(c)
  return t

visitVarExpr(v):
  return resolve(v)

visitParensExpr(e):
  return visit(e)
```

#### `visitPreIncDecVarExpr`, `visitPostIncDecVarExpr`

```text
visitPreIncDecVarExpr(var, op):
  v = resolve(var)
  one = createTemp(); emit ldconst one, 1
  si op == ++: emit add v, v, one
  sinon: emit sub v, v, one
  return v

visitPostIncDecVarExpr(var, op):
  v = resolve(var)
  old = createTemp(); emit copy old, v
  one = createTemp(); emit ldconst one, 1
  si op == ++: emit add v, v, one
  sinon: emit sub v, v, one
  return old
```

#### `visitUnitaryExpr`

```text
visitUnitaryExpr(op, e):
  x = visit(e)
  d = createTemp()
  si op == "-": emit neg d, x
  sinon: emit not_ d, x
  return d
```

#### `visitAddSubExpr`, `visitMultDivModExpr`, `visitCompareExpr`, `visitEqualExpr`, `visitLogicBitANDExpr`, `visitLogicBitORExpr`, `visitLogicBitXORExpr`
Schema commun:

```text
visitBinary(op, lhs, rhs):
  l = visit(lhs)
  r = visit(rhs)
  d = createTemp()
  emit operation(op) d, l, r
  return d
```

#### `visitLogicANDExpr` (court-circuit)

Pseudo-code:

```text
visitLogicANDExpr(lhs, rhs):
  dest = createTemp()
  zero = createTemp(); ldconst zero, 0
  left = visit(lhs)
  leftBool = createTemp(); cmp_ne leftBool, left, zero

  créer bb_rhs, bb_false, bb_end
  branch sur leftBool: vrai->bb_rhs, faux->bb_false

  bb_false: ldconst dest, 0; jump bb_end
  bb_rhs  : right = visit(rhs)
            rightBool = createTemp(); cmp_ne rightBool, right, zero
            copy dest, rightBool
            jump bb_end

  current_bb = bb_end
  return dest
```

#### `visitLogicORExpr` (court-circuit)

Pseudo-code:

```text
visitLogicORExpr(lhs, rhs):
  dest = createTemp()
  zero = createTemp(); ldconst zero, 0
  left = visit(lhs)
  leftBool = createTemp(); cmp_ne leftBool, left, zero

  créer bb_true, bb_rhs, bb_end
  branch sur leftBool: vrai->bb_true, faux->bb_rhs

  bb_true: ldconst dest, 1; jump bb_end
  bb_rhs : right = visit(rhs)
           rightBool = createTemp(); cmp_ne rightBool, right, zero
           copy dest, rightBool
           jump bb_end

  current_bb = bb_end
  return dest
```

#### `visitCallExpr`

```text
visitCallExpr(func, args):
  dest = createTemp()
  params = [func, dest]
  pour arg dans args:
    params.push(visit(arg))
  emit call params
  return dest
```

#### `visitReturn_stmt`

```text
visitReturn_stmt(ctx):
  si expr presente:
    v = visit(expr)
  sinon:
    v = createTemp(); ldconst v, 0
  emit ret v
  jump bb_epilogue
  return v
```

#### `visitIf_stmt`

```text
visitIf_stmt(cond, thenStmt, elseStmt?):
  créer bb_cond, bb_then, bb_end, (optionnel bb_else)
  jump vers bb_cond
  bb_cond.test = visit(cond)
  branch bb_cond -> bb_then / bb_else(ou bb_end)

  bb_then: visit(thenStmt); si pas de sortie, jump bb_end
  si else:
    bb_else: visit(elseStmt); si pas de sortie, jump bb_end

  current_bb = bb_end
```

#### `visitWhile_stmt`

```text
visitWhile_stmt(cond, body):
  créer bb_cond, bb_body, bb_end
  jump vers bb_cond
  bb_cond.test = visit(cond)
  branch bb_cond -> bb_body / bb_end

  push breakTargets(bb_end)
  push continueTargets(bb_cond)

  bb_body: visit(body)
  si pas de sortie: jump bb_cond

  pop continueTargets
  pop breakTargets
  current_bb = bb_end
```

#### `visitBreak_stmt`, `visitContinue_stmt`

```text
visitBreak_stmt:
  si breakTargets non vide: jump breakTargets.top

visitContinue_stmt:
  si continueTargets non vide: jump continueTargets.top
```

#### `visitSwitch_stmt`
Responsabilité:
- evaluer l'expression switch une fois
- créer une chaine de dispatch (cmp_eq) vers chaque case
- supporter default
- supporter fallthrough et break

Pseudo-code simplifié:

```text
visitSwitch_stmt(expr, parts):
  switchVal = visit(expr)
  bb_end = new block

  extraire toutes les labels case/default dans des blocks dedies
  créer un premier block de dispatch

  pour chaque case i:
    cst = ldconst(caseValue)
    cond = cmp_eq(switchVal, cst)
    branch cond -> caseBlock[i], nextDispatchOrDefaultOrEnd

  push breakTargets(bb_end)

  parcourir parts dans l'ordre source:
    quand label rencontre, positionner current_bb sur son block
    sur chaque stmt, visit(stmt)
    laisser le fallthrough si aucun jump explicite

  si dernier block actif sans sortie: jump bb_end
  pop breakTargets
  current_bb = bb_end
```

---

## 6) Backend et consequences du mode int-only

Le backend reste compatible avec des structures historiques plus larges, mais le front-end actuel n'emet que:
- des valeurs entières
- des operations arith/logiques/comparaison entières
- des sauts de contrôle classiques

---

## 7) Tests et exécution

### 7.1 Build

```bash
cd compiler
make clean
make -j4
```

### 7.2 Regression recommandee (scope supporté)

```bash
python3 ifcc-test.py testfiles
```

Note:
- `testfiles/undefined` fait partie de la validation mais ne donne jamais le bon résultat (comportements C non definis).

---

## 8) Regles d'evolution (ordre obligatoire)

Si une nouvelle feature est ajoutee:

1. Etendre `ifcc.g4` minimalement
2. Ajouter/adapter les règles sémantiques dans `SymbolVisitor`
3. Ajouter la generation IR dans `IRVisitor`
4. Ajouter tests `valid` et `invalid`
5. Mettre à jour README + ce fichier

Regle forte:
- ne jamais ajouter de génération IR sans barrière sémantique explicite.

---

## 9) Exemple fil-rouge (du source au CFG)

Objectif de cette section:
- montrer le chemin d'un mini programme dans toutes les passes
- nommer les fonctions qui interviennent et ce qu'elles produisent

Programme exemple:

```c
int add(int x, int y) {
    int z = x + y;
    return z;
}

int main() {
    int a = 2;
    int b = 3;
    if (a < b) {
        a += 10;
    }
    return add(a, b);
}
```

### 9.1 Parsing (ANTLR)

Points cle:
- `prog` contient 2 `function_decl`
- la déclaration `int z = x + y;` est un `declare_stmt` contenant un `declare_elmt(assign_stmt)`
- `a += 10` est un `AssignExpr` (operateur compose)
- `return add(a, b)` est un `Return_stmt` avec `CallExpr`

### 9.2 Passe sémantique (`SymbolVisitor`)

Ordre logique:

```text
visitProg
  -> predeclare add/main dans functionTable
  -> visitFunction_decl(add)
     -> scope params x,y
     -> visitDeclare_stmt(z = x + y)
        -> visitAddSubExpr(x,y)
     -> visitReturn_stmt(return z)
  -> visitFunction_decl(main)
     -> visitDeclare_stmt(a = 2)
     -> visitDeclare_stmt(b = 3)
     -> visitIf_stmt
        -> visitCompareExpr(a < b)
        -> visitAssignExpr(a += 10)
     -> visitReturn_stmt(return add(a,b))
        -> visitCallExpr(add,[a,b])
  -> checkUnusedVariables
```

Effet concret sur les tables:
- `functionTable[add] = {returnType=Int, arity=2, ...}`
- `functionTable[main] = {returnType=Int, arity=0, ...}`
- `table` contient des noms uniques (`x_0`, `y_1`, `z_2`, `a_3`, `b_4`, ...)
- chaque lecture/écriture marque `isUsed = true`

### 9.3 Passe IR (`IRVisitor`)

Idee generale:
- un CFG par fonction
- chaque expression cree des temporaires (`tmpN`)
- chaque contrôle (`if`, `while`, `switch`) cree des basic blocks relies

Pseudo-trace simplifiée pour `main`:

```text
entry -> prologue -> body

body:
  t0 = ldconst 2
  copy a_3, t0
  t1 = ldconst 3
  copy b_4, t1
  t2 = cmp_lt a_3, b_4
  branch t2 -> if_then / if_end

if_then:
  t3 = ldconst 10
  add a_3, a_3, t3
  jump if_end

if_end:
  t4 = call add, dest=t4, args=[a_3,b_4]
  ret t4
  jump epilogue
```

Résultat:
- CFG `main` avec blocs `body`, `if_then`, `if_end`, `epilogue`
- IR strictement entier, sans operations mémoire indirecte


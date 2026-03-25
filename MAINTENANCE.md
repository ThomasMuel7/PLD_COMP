# MAINTENANCE

Ce document est un guide de maintenance complet pour le compilateur IFCC de ce depot.
Il est ecrit pour permettre a une nouvelle personne de:
- comprendre rapidement l'architecture,
- localiser chaque responsabilite,
- modifier une fonctionnalite sans casser le reste,
- diagnostiquer les erreurs de parsing, semantique, IR, ou backend.

---

## 1) Vue d'ensemble du pipeline

Le compilateur suit 4 grandes etapes:

1. Parsing (ANTLR)
- Entree: source C simplifiee.
- Sortie: arbre syntaxique (AST ANTLR).
- Fichiers: `compiler/ifcc.g4`, `compiler/generated/*`.

2. Analyse semantique
- Entree: AST.
- Sortie: tables de symboles/fonctions + validations de type/contexte.
- Fichiers: `compiler/src/SymbolVisitor.h`, `compiler/src/SymbolVisitor.cpp`, `compiler/src/SymbolTable.h`, `compiler/src/ScopeTable.h`.

3. Generation IR + CFG
- Entree: AST + infos semantiques.
- Sortie: CFG par fonction + suite d'instructions IR par basic block.
- Fichiers: `compiler/src/IRVisitor.h`, `compiler/src/IRVisitor.cpp`, `compiler/src/IR.h`, `compiler/src/IR.cpp`, `compiler/src/BasicBlock.h`, `compiler/src/BasicBlock.cpp`, `compiler/src/CFG.h`, `compiler/src/CFG.cpp`.

4. Backend (x86_64 / AArch64)
- Entree: CFG + symbol table.
- Sortie: assembleur cible.
- Fichiers: `compiler/src/backend.h`, `compiler/src/backend.cpp`.

Point d'entree global:
- `compiler/main.cpp`.

---

## 2) Fichiers et roles (inventaire)

### 2.1 Racine

- `ifcc-test.py`
  - Harness principal de test.
  - Compare GCC et IFCC sur compilation, edition de liens, execution.
  - Supporte mode multi-fichiers et mode single-file (`-S`, `-c`, `-o`).

- `testing_wrapper.py`
  - Wrapper local de lancement des tests.
  - Detecte architecture, injecte `-target x86|arm` via un script temporaire.

- `README.md`
  - Documentation utilisateur/projet.

- `testfiles/`
  - Corpus de tests valides/invalides par categorie.

### 2.2 Dossier compiler

- `compiler/main.cpp`
  - Orchestration globale du compilateur.

- `compiler/ifcc.g4`
  - Grammaire ANTLR: lexer + parser.

- `compiler/generated/*`
  - Code ANTLR genere automatiquement (lexer/parser/visiteurs).

- `compiler/src/*`
  - Coeur du compilateur (semantique, IR, CFG, backend).

- `compiler/Makefile`
  - Build du compilateur, generation ANTLR, cible tests, clean.

- `compiler/config.mk`
  - Variables locales de chemin ANTLR (jar/include/lib).

---

## 3) Point d'entree detaille: compiler/main.cpp

Fonction: `main(int argc, const char **argv)`

Responsabilites:
1. Parse les arguments CLI:
- `-target x86|arm`.
- fichier d'entree C.

2. Lit le fichier source en memoire (`stringstream in`).

3. Lance ANTLR:
- `ANTLRInputStream`, `ifccLexer`, `CommonTokenStream`, `ifccParser`.
- Point de depart: `axiom`.

4. Coupe le pipeline si erreurs de syntaxe.

5. Lance analyse semantique:
- Instancie `SymbolVisitor`.
- Visite `tree->prog()`.
- Coupe si `visitor.hasError`.

6. Lance generation IR:
- Instancie `IRVisitor(visitor.table, visitor.functionTable, visitor.currentOffset)`.
- Visite `tree->prog()`.

7. Choisit backend selon cible:
- `x86Backend` ou `ArmBackend`.

8. Genere assembleur:
- `backendInstance->translate()`.

Remarque maintenance:
- Aucun `delete` explicite des allocations dynamiques (CFG/BB/IR/backend). Acceptable pour un outil CLI court, mais fuite memoire structurelle si integration long-running.

---

## 4) Grammaire: compiler/ifcc.g4

### 4.1 Regles structurelles

- `axiom : prog EOF;`
- `prog : function_decl+;`
- `function_decl : type VAR '(' param_list? ')' block;`
- `type : 'int' | 'void';`
- `param : 'int' ('*')* VAR;`

### 4.2 Instructions (`stmt`)

`stmt` supporte:
- declaration `declare_stmt`
- `return_stmt`
- `break_stmt`
- `continue_stmt`
- `switch_stmt`
- expression `expr ';'`
- bloc imbrique
- `if_stmt`
- `while_stmt`

### 4.3 Declarations

- `declare_stmt : 'int' declarator (',' declarator)* ';';`
- `declarator : ('*')* VAR ('[' INT ']')?;`

Permet:
- int scalaire,
- pointeur (`int *p`),
- tableau 1D (`int a[10]`),
- multi-level pointer syntaxiquement (`int **pp`) via repetition de `*`.

Important:
- La semantique interne du projet est surtout binaire int/pointeur (pas de modelisation fine multi-niveaux).

### 4.4 Switch/case/default

- `switch_stmt : 'switch' '(' expr ')' '{' switch_part* '}';`
- `switch_part : case_label | default_label | stmt;`
- `case_label : 'case' (INT | CHAR) ':';`
- `default_label : 'default' ':';`

Le design `switch_part` alterne labels et statements dans un flux unique, utile pour reproduire le fallthrough.

### 4.5 Expressions (priorites)

Ordre (haut vers bas dans la regle `expr`):
- parentheses,
- address-of `&VAR`,
- deref assign `*expr OP expr`,
- array assign `VAR[expr] OP expr`,
- pre inc/dec `++VAR` / `--VAR`,
- post inc/dec `VAR++` / `VAR--`,
- unaires `!`, `-`, `*`,
- acces tableau `VAR[expr]`,
- `* / %`,
- `+ -`,
- comparaisons relationnelles,
- egalite/inegalite,
- bitwise `& ^ |`,
- logique `&& ||`,
- assignation `VAR OP expr`,
- constantes,
- variable,
- appel de fonction.

Attention maintenance:
- L'ordre des alternatives ANTLR impacte la resolution de formes ambigu es (notamment autour de `*` deref vs multiplication).

### 4.6 Lexer

- `VAR`, `INT`, `CHAR`.
- commentaires `/* ... */` et `//...` ignores.
- directives preprocesseur ignorees.

---

## 5) Modele de donnees semantique

### 5.1 SymbolTable.h

#### struct `VariableInfo`
- `int index`: offset pile relatif a rbp (x86) / conversion sp (arm).
- `bool isUsed`: marque usage (potentiellement pour warning, non exploite pleinement).
- `bool isPointer`: variable pointeur logique.
- `bool isArray`: variable tableau 1D.
- `int arrayLength`: taille logique du tableau.
- `int byteSize`: taille reservee en pile (4, 8, ou 4*N).

#### enum `ReturnType`
- `Int`
- `Void`

#### struct `FunctionInfo`
- `ReturnType returnType`
- `int arity`
- `vector<string> paramUniqueNames`
- `vector<bool> paramIsPointer`

#### alias
- `using SymbolTable = map<string, VariableInfo>`
- `using FunctionTable = map<string, FunctionInfo>`

### 5.2 ScopeTable.h

`ScopeTable` = `vector<map<string,string>>`
- chaque map relie nom source -> nom unique interne.
- gere masquage (shadowing) par pile de scopes.

---

## 6) Analyse semantique: SymbolVisitor

Fichiers:
- `compiler/src/SymbolVisitor.h`
- `compiler/src/SymbolVisitor.cpp`

### 6.1 Variables membres (etat global de visite)

- `SymbolTable table`: table des variables uniques.
- `FunctionTable functionTable`: signatures des fonctions.
- `ScopeTable scopeTable`: pile de scopes actifs.
- `int currentOffset`: offset pile courant (decroit).
- `int uniqueVarId`: suffixe pour noms uniques.
- `bool hasError`: drapeau global d'erreur semantique.
- `string currentFunctionName`: fonction analysee.
- `ReturnType currentFunctionReturnType`: type de retour de la fonction courante.
- `int loopDepth`: profondeur d'imbrication des boucles.
- `int switchDepth`: profondeur d'imbrication des switch.

### 6.2 Helpers internes

- `resolveVariable(originalName)`:
  - cherche depuis le scope le plus interne vers l'externe.
  - retourne nom unique ou chaine vide.

- `lookupVariableInfo(originalName)`:
  - combine resolution de nom + acces a `table`.

- Helpers locaux cpp:
  - `TYPE_INT`, `TYPE_PTR`, `TYPE_INVALID`.
  - `anyToExprType(Any)` pour extraire robustement un type expression.
  - `parseReturnType(string)`.

### 6.3 Visiteurs (fonctions) et responsabilites

- `visitProg`
  - pre-declare toutes les fonctions (signature + arite + pointeur params).
  - detecte duplication de nom de fonction.
  - exige presence de `main`.
  - puis visite chaque fonction.

- `visitFunction_decl`
  - initialise contexte fonction courant.
  - ouvre scope parametres.
  - alloue offsets et entrees table pour les params.
  - renseigne `paramUniqueNames` et `paramIsPointer` dans `functionTable`.
  - visite le bloc de fonction.

- `visitBlock`
  - push/pop de scope lexical.

- `visitDeclare_stmt`
  - traite chaque declarateur.
  - detecte taille tableau invalide (`<=0`).
  - interdit combinaison pointeur+tableau dans ce projet.
  - detecte redeclaration dans meme scope.
  - calcule `byteSize` et met a jour `currentOffset`.

- `visitReturn_stmt`
  - contraintes `void` vs `int`.
  - interdit `return expr` dans `void`.
  - interdit `return;` dans `int`.
  - interdit retour pointeur dans fonction int.

- `visitVarExpr`
  - verifie declaration.
  - marque usage.
  - type retourne: ptr si variable pointeur/tableau, sinon int.

- `visitAssignExpr`
  - verifie declaration lhs.
  - interdit assignation directe d'un tableau.
  - pour `OP!=` (+= etc), interdit operations composees sur pointeur.
  - force compatibilite type lhs/rhs selon pointeur/int.

- `visitArrayAssignExpr`
  - lhs doit etre tableau ou pointeur.
  - index doit etre int.
  - rhs doit etre int.

- `visitDerefAssignExpr`
  - lhs de `*lhs = rhs` doit etre pointeur.
  - rhs doit etre int.

- `visitMultDivModExpr`
  - interdit operandes pointeur.
  - warning division/modulo par zero si detecte statiquement sur constante.

- `visitAddrExpr`
  - `&var`: var doit etre declaree.
  - retourne type pointeur.

- `visitPreIncDecVarExpr` / `visitPostIncDecVarExpr`
  - variable doit exister.
  - interdit ++/-- sur pointeur et tableau.
  - retourne int.

- `visitArrayAccessExpr`
  - base doit etre tableau ou pointeur.
  - index int.
  - resultat int.

- `visitUnitaryExpr`
  - `*expr` requiert pointeur.
  - `-` et `!` requierent int.

- `visitConstExpr` / `visitParensExpr`
  - garantissent que constantes et parenthesages remontent un type `int` explicite.

- `visitCompareExpr` / `visitLogicBitANDExpr` / `visitLogicBitXORExpr` / `visitLogicBitORExpr` / `visitLogicANDExpr` / `visitLogicORExpr`
  - valident les operandes et remontent un type `int` explicite.

- Correctif semantique important
  - le dereferencement unaire est strict: `*expr` exige un vrai pointeur.
  - des cas invalides comme `*5` (donc `5 ** 5`) sont maintenant rejetes.

- `visitAddSubExpr`
  - int +/- int => int.
  - ptr +/- int ou int + ptr => ptr.
  - ptr +/- ptr => erreur (non supporte).

- `visitEqualExpr`
  - verifie compatibilite de types (hors invalid).
  - retourne int booleen.

- `visitCallExpr`
  - limite a 6 arguments.
  - regles speciales `putchar` (1 arg) et `getchar` (0 arg).
  - fonction utilisateur doit etre definie et bonne arite.
  - interdit usage d'une fonction void comme expression.
  - verifie type de chaque argument vs `paramIsPointer`.

- `visitBreak_stmt`
  - autorise seulement dans boucle ou switch.

- `visitContinue_stmt`
  - autorise seulement dans boucle.

- `visitWhile_stmt`
  - condition doit etre int.
  - gere `loopDepth` pour validite de break/continue.

- `visitSwitch_stmt`
  - expression switch doit etre int.
  - detecte case duplique.
  - detecte multiples default.
  - gere `switchDepth`.

---

## 7) IR, CFG, Basic Blocks

### 7.1 IR.h / IR.cpp

Classe `IRInstr`:
- membre `BasicBlock *bb`
- membre `Operation op`
- membre `vector<string> params`

Enum `Operation`:
- `ldconst`, `copy`
- arithmetique: `add`, `sub`, `mul`, `div`, `mod`
- memoire/pointeur: `addr`, `rmem`, `wmem`
- appel: `call`
- comparaisons: `cmp_eq`, `cmp_lt`, `cmp_le`, `cmp_gt`, `cmp_ge`, `cmp_ne`
- logique bitwise: `and_`, `or_`, `xor_`
- unaires: `neg`, `not_`
- retour: `ret`

Convention `params`:
- 3-operandes: `dest, lhs, rhs`
- `ldconst`: `dest, immediate`
- `ret`: `var`
- `call`: `funcName, dest, arg1, arg2, ...`

### 7.2 BasicBlock.h / BasicBlock.cpp

Classe `BasicBlock`:
- `BasicBlock *exit_true`
- `BasicBlock *exit_false`
- `string label`
- `CFG *cfg`
- `vector<IRInstr*> instrs`
- `string test_var_name`

Methodes:
- constructeur: ajoute automatiquement le block a `cfg->blocks`.
- `add_IRInstr(op, params)`.
- `add_exit(exit_true, exit_false)`.

Semantique des sorties:
- deux sorties: branchement conditionnel sur `test_var_name`.
- une sortie: jump inconditionnel.
- zero sortie: chute vers epilogue (selon generation backend).

### 7.3 CFG.h / CFG.cpp

Classe `CFG`:
- `BasicBlock *entry`
- `vector<BasicBlock*> blocks`
- `string functionName`
- `bool isVoidReturn`
- `vector<string> paramVarNames`
- `vector<bool> paramIsPointer`

Methodes:
- constructeur `CFG(functionName, isVoidReturn)`.
- `set_entry`.

---

## 8) Generation IR: IRVisitor

Fichiers:
- `compiler/src/IRVisitor.h`
- `compiler/src/IRVisitor.cpp`

### 8.1 Variables membres

- `vector<CFG*> cfgs`: toutes les fonctions.
- `CFG *cfg`: CFG courant.
- `BasicBlock *current_bb`: block d'insertion courant.
- `BasicBlock *bb_epilogue`: block epilogue de la fonction courante.
- `SymbolTable &table`: table des symboles (reference, partagee).
- `const FunctionTable &functionTable`: signatures fonctions.
- `ScopeTable scopeTable`: renommage local IR.
- `int currentOffset`: offset pile courant pour temporaires.
- `int tempCounter`: compteur `tmpN`.
- `int uniqueVarId`: compteur noms uniques pour variables utilisateur.
- `map<string,bool> tempIsPointer`: type pointeur des temporaires.
- `vector<BasicBlock*> breakTargets`: pile de cibles break.
- `vector<BasicBlock*> continueTargets`: pile de cibles continue.

### 8.2 Helpers

- `resolveVariable`: resolution lexicale, fallback sur nom brut.
- `gen_unique_id(ctx)`: suffixe stable base sur ligne/colonne pour labels.
- `isPointerValue(name)`: regarde `tempIsPointer` puis `table`.
- `createTemp(isPointer=false)`:
  - cree `tmpN`.
  - reserve 4 ou 8 octets.
  - met a jour `table` et `tempIsPointer`.

### 8.3 Visiteurs expression/affectation

- `visitAssignExpr`
  - `=` -> `copy`
  - `+= -= *= /=` -> operation sur lhs en place.

- `visitArrayAssignExpr`
  - calcule adresse element: base + index*4.
  - base tableau: convertie en pointeur via `addr`.
  - `=` -> `wmem`
  - compose -> `rmem`, operation, `wmem`.

- `visitDerefAssignExpr`
  - `*ptr = v` -> `wmem`.
  - compose -> read-modify-write via `rmem/wmem`.

- `visitAddSubExpr`
  - int/int: add/sub simple.
  - ptr/int: scale rhs par 4 puis add/sub sur pointeur.
  - int+ptr: symetrique sur add.
  - ptr/ptr: passe par sub brut (la semantique rejette en amont normalement).

- `visitMultDivModExpr`
  - mapping vers `mul/div/mod`.

- `visitConstExpr`
  - alloue temp + `ldconst`.
  - CHAR converti via code ascii du caractere.

- `visitVarExpr`
  - variable scalaire: retourne nom unique.
  - tableau: retourne pointeur base (`addr` vers temp pointeur).

- `visitAddrExpr`
  - `&var` -> `addr` dans temp pointeur.

- `visitPreIncDecVarExpr`
  - modifie variable puis retourne la variable.

- `visitPostIncDecVarExpr`
  - copie ancienne valeur, modifie variable, retourne ancienne valeur.

- `visitArrayAccessExpr`
  - calcule adresse element puis `rmem`.

- `visitUnitaryExpr`
  - `*expr` -> `rmem`.
  - `-expr` -> `neg`.
  - `!expr` -> `not_`.

- `visitCompareExpr` / `visitEqualExpr`
  - genere cmp_* adequat, retourne temp booleen int.

- `visitLogicBitANDExpr` / `visitLogicBitORExpr` / `visitLogicBitXORExpr`
  - mapping direct vers `and_` / `or_` / `xor_`.

- `visitLogicANDExpr` / `visitLogicORExpr`
  - short-circuit avec creation de blocks.
  - normalise booleens via comparaison `!= 0`.

- `visitCallExpr`
  - fabrique `params = [func, dest, args...]`.
  - ajoute IR `call`.

### 8.4 Visiteurs controle

- `visitFunction_decl`
  - cree `prologue`, `body`, `epilogue`.
  - enregistre params dans cfg (`paramVarNames`, `paramIsPointer`).
  - visite bloc.
  - ajoute retour implicite `0` si aucun exit deja fixe.

- `visitBlock`
  - push/pop scope.
  - stop sur statement qui clot deja le flow (`exit_true != nullptr`).

- `visitReturn_stmt`
  - sans expr -> temp=0.
  - ajoute `ret` puis branche vers epilogue.

- `visitIf_stmt`
  - cree blocks cond/then/(else)/end.
  - condition via `test_var_name`.

- `visitWhile_stmt`
  - cree cond/body/end.
  - push/pop `breakTargets` et `continueTargets`.

- `visitBreak_stmt`
  - jump vers `breakTargets.back()` si present.

- `visitContinue_stmt`
  - jump vers `continueTargets.back()` si present.

- `visitSwitch_stmt`
  - evalue expression switch.
  - cree block end + blocks de case/default.
  - cree chaine de dispatch `cmp_eq` case par case.
  - gere fallthrough explicite par chaining des blocks labels.
  - push/pop cible break vers fin du switch.

---

## 9) Backend: emission assembleur

Fichiers:
- `compiler/src/backend.h`
- `compiler/src/backend.cpp`

### 9.1 Classe abstraite `backend`

Membres:
- `vector<CFG*> cfgs`
- `SymbolTable symbolTable` (copie)

Methodes:
- `translate()` pure virtuelle.
- `generate(IRInstr*)` pure virtuelle.

### 9.2 x86Backend

Methodes principales:
- `computeFrameSize()`:
  - cherche offset minimum dans symbol table.
  - aligne stack frame sur 16.

- `getOffset(varName)`:
  - retourne `index(%rbp)`.

- `loadBinaryOperands(instr)`:
  - charge op1 en `%eax`, op2 en `%ebx` (32 bits).

- `saveResultEax(instr)`:
  - stocke `%eax` vers destination.

- `translate()`:
  - emet label global par fonction.
  - prologue `pushq/movq/subq`.
  - map params registres SysV (`edi, esi...` ou `rdi, rsi...` pour pointeurs).
  - emet code de chaque IR du block.
  - emet branches selon `exit_true/exit_false`.
  - epilogue `leave; ret`.

- `generate(instr)`:
  - map complet IR -> asm x86.
  - 64 bits pour valeurs pointeur (`movq`, `cmpq`, etc).
  - 32 bits pour int (`movl`, `cmpl`, etc).
  - memoire indirecte:
    - `rmem`: charge adresse pointeur, lit `(%rax)`.
    - `wmem`: charge adresse pointeur, ecrit `(%rax)`.

### 9.3 ArmBackend (AArch64)

Methodes analogues:
- `computeFrameSize()`.
- `getOffset(varName)`: convertit offset relatif en offset positif depuis SP courant.
- `loadBinaryOperands` et `saveResultEax` (w0/w1).
- `translate()`:
  - prologue `sub sp, sp, #frameSize`.
  - params dans `w0..w7` ou `x0..x7` selon pointeur.
  - branches `beq`/`b`.
  - epilogue `add sp, sp, #frameSize; ret`.

- `generate(instr)`:
  - map IR -> instructions AArch64.
  - pointeurs en registres `x*`.
  - int en `w*`.
  - `addr`: `add x8, sp, #offset`.
  - `rmem/wmem`: dereference via `x8`.

### 9.4 ABI et limites

- Arguments supportes: 6 max cote semantique (meme si ARM peut plus en registres).
- Return: backend stocke valeur retour en int (`eax` / `w0`) meme pour usages pointeurs non officiellement supportes comme return type.

---

## 10) Build et regeneration

### 10.1 Makefile

Cibles importantes:
- `make ifcc` ou `make all`.
- Generation ANTLR auto si `ifcc.g4` modifie.
- `make tests` appelle `python3 ../testing_wrapper.py`.
- `make clean` supprime `build`, `generated`, `ifcc`.

### 10.2 config.mk

Doit definir:
- `ANTLRJAR`
- `ANTLRINC`
- `ANTLRLIB`

Sans ces chemins, compilation impossible.

---

## 11) Tests et validation

### 11.1 ifcc-test.py

Logique cle:
- rebuild `compiler/ifcc` si necessaire (`make --question`, puis `make`).
- mode multiple:
  - copie chaque test dans `ifcc-test-output/<job>/input.c`.
  - compile GCC et IFCC,
  - link et execute si valides,
  - compare les sorties d'execution.

Verdicts:
- GCC invalide + IFCC invalide => OK.
- GCC invalide + IFCC valide => FAIL.
- GCC valide + IFCC invalide => FAIL.
- les 2 valides mais execution differente => FAIL.

### 11.2 testing_wrapper.py

Ajoute une couche utilitaire:
- detecte arch.
- injecte `-target` de facon transparente via renommage `ifcc -> ifcc_real`.
- restaure toujours l'executable apres test (bloc `finally`).

---

## 12) Cartographie des fonctionnalites (de bout en bout)

### 12.1 Pointeurs

- Grammaire:
  - declaration `int *p`
  - `&var`, `*expr`, `*expr = rhs`

- Semantique:
  - types int/pointeur verifies.
  - interdiction ++/-- sur pointeurs.
  - affectation pointeur <- pointeur uniquement.

- IR:
  - `addr`, `rmem`, `wmem`.

- Backend:
  - moves/comparaisons 64 bits pour pointeurs.

### 12.2 Tableaux 1D

- Grammaire:
  - `int a[N]`, `a[i]`, `a[i] = v`, `a[i] += v` etc.

- Semantique:
  - taille > 0,
  - index int,
  - ecriture element int,
  - assignation directe tableau interdite.

- IR:
  - conversion base tableau vers pointeur (`addr`),
  - address arithmetic `index * 4`.

### 12.3 Break/Continue

- Semantique:
  - `break`: boucle ou switch.
  - `continue`: boucle seulement.

- IR:
  - piles `breakTargets` / `continueTargets`.

### 12.4 Switch/case/default

- Semantique:
  - expr switch int,
  - case uniques,
  - max un default.

- IR:
  - chaine de dispatch `cmp_eq`.
  - blocks labels + fallthrough naturel.
  - break cible fin switch.

### 12.5 Pre/Post ++/--

- Semantique:
  - variable int uniquement.

- IR:
  - pre: update puis retourne nouvelle valeur.
  - post: sauvegarde ancienne valeur, update, retourne ancienne valeur.

---

## 13) Limitations connues

1. Type system simplifie
- Principalement `int` et `pointer-like`.
- Pas de type-checking profond multi-niveaux de pointeurs.

2. Return policy stricte
- Le compilateur rejette certains cas que GCC peut accepter (ex: `return;` dans int, `return expr;` dans void).

3. Fonctions
- Limite semantique de 6 arguments.
- Pas de support de declarations sans definition separee.

4. Allocation memoire
- Pas de gestion de destruction des objets dynamiques C++ internes du compilateur.

5. Warnings
- Pas de passe dediee de warning globale (hors warnings ponctuels comme division/modulo par zero constante).

---

## 14) Guide de modification (playbook)

### 14.1 Ajouter un nouvel operateur

1. Modifier `ifcc.g4` (syntaxe et priorite).
2. Regenerer parser (`make` suffit).
3. Ajouter regle semantique dans `SymbolVisitor`.
4. Ajouter lowering dans `IRVisitor`.
5. Ajouter emission asm dans `x86Backend::generate` et `ArmBackend::generate`.
6. Ajouter tests valides/invalides + stress dans `testfiles/`.

### 14.2 Ajouter une nouvelle instruction de controle

1. Grammaire (`stmt` + regle dediee).
2. Validation contexte dans `SymbolVisitor` (profondeurs/piles).
3. Construction CFG dans `IRVisitor` (basic blocks + exits).
4. Tests de flux (fallthrough, imbrications, interactions avec return/break).

### 14.3 Ajouter un nouveau type de donnees

1. Etendre `VariableInfo` / type system semantique.
2. Adapter toutes les regles de compatibilite dans `SymbolVisitor`.
3. Propager metadata type vers temporaires (`createTemp`, `tempIsPointer` equivalent generalise).
4. Adapter backend (taille, move, compare, ABI args/ret).

---

## 15) Checklist de regression minimale

Apres toute modification:

1. `make -C compiler clean && make -C compiler ifcc`
2. `python3 ifcc-test.py testfiles`
3. Verifier categories critiques:
- pointeurs/tableaux
- switch/break/continue
- incdec
- appels fonctions
- invalides semantiques

4. Si un test valide GCC est rejete IFCC:
- inspecter `ifcc-test-output/<job>/ifcc-compile.txt`.

5. Si un test compile mais diverge a l'execution:
- comparer `asm-gcc.s` vs `asm-ifcc.s`.
- verifier CFG genere indirectement via structure des sauts labels.

---

## 16) Aide au debug (symptome -> zone probable)

- Erreur de syntaxe immediate:
  - `ifcc.g4` ou priorites/alternatives ambigues.

- Programme invalide accepte:
  - regle manquante dans `SymbolVisitor`.

- Programme valide rejete:
  - regle trop stricte dans `SymbolVisitor` (ou difference de politique assumee).

- Mauvais resultat runtime avec compilation ok:
  - lowering `IRVisitor` incorrect,
  - ou emission backend (taille registre, branchement, offset).

- Segfault runtime:
  - calcul d'adresse (`addr`, index*4, rmem/wmem),
  - ou erreur de frame offset.

- Sauts/fallthrough incorrects:
  - `visitIf_stmt`, `visitWhile_stmt`, `visitSwitch_stmt`, ou branchement final des BB dans backend.

---

## 17) Resume executif pour nouveau mainteneur

- Le pipeline est proprement separe: grammaire -> semantique -> IR/CFG -> backend.
- Les deux classes les plus sensibles sont:
  - `SymbolVisitor` (coherence langage),
  - `IRVisitor` (coherence comportementale).
- Le backend est relativement mecanique, mais toute erreur de largeur (32/64 bits) casse vite pointeurs/appels.
- Les tests comparent IFCC a GCC, ce qui donne un filet de securite solide.
- Le projet est maintenable si toute nouvelle feature suit strictement le workflow:
  - syntaxe, semantique, IR, backend, tests.

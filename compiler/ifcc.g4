grammar ifcc;

axiom : prog EOF ;

prog : function_decl+;

function_decl : type VAR '(' param_list? ')' block;

type : 'int' | 'void';

param_list : param (',' param)*;

param : 'int' ('*')* VAR;

block : '{'stmt*'}' ;

stmt : declare_stmt
     | return_stmt
     | break_stmt
     | continue_stmt
     | switch_stmt
     | expr ';'
     | block
     | if_stmt
     | while_stmt
     ;

declare_stmt : 'int' declarator (',' declarator)* ';' ;

declarator : ('*')* VAR ('[' INT ']')? ;

return_stmt : 'return' expr? ';' ;

if_stmt : 'if' '(' expr ')' stmt ('else' stmt)? ; 

while_stmt : 'while' '(' expr ')' stmt ;

break_stmt : 'break' ';' ;

continue_stmt : 'continue' ';' ;

switch_stmt : 'switch' '(' expr ')' '{' switch_part* '}' ;

switch_part : case_label | default_label | stmt ;

case_label : 'case' (INT | CHAR) ':' ;

default_label : 'default' ':' ;

expr : '(' expr ')'                                               #ParensExpr
     | '&' VAR                                                    #AddrExpr
     | '*' expr OP=('=' | '+=' | '-=' | '*=' | '/=') expr         #DerefAssignExpr
     | VAR '[' expr ']' OP=('=' | '+=' | '-=' | '*=' | '/=') expr #ArrayAssignExpr
     | OP=('++' | '--') VAR                                       #PreIncDecVarExpr
     | VAR OP=('++' | '--')                                       #PostIncDecVarExpr
     | OP=('!' | '-' | '*')expr                                   #UnitaryExpr
     | VAR '[' expr ']'                                           #ArrayAccessExpr
     | expr OP=('*' | '/' | '%') expr                             #MultDivModExpr
     | expr OP=('+' | '-') expr                                   #AddSubExpr
     | expr OP=('>' | '<' | '>=' | '<=') expr                     #CompareExpr
     | expr OP=('==' | '!=') expr                                 #EqualExpr
     | expr '&' expr                                              #LogicBitANDExpr
     | expr '^' expr                                              #LogicBitXORExpr
     | expr '|' expr                                              #LogicBitORExpr
     | expr '&&' expr                                             #LogicANDExpr
     | expr '||' expr                                             #LogicORExpr
     | VAR OP=('=' | '+=' | '-=' | '*=' | '/=') expr              #AssignExpr
     | (INT | CHAR )                                              #ConstExpr
     | VAR                                                        #VarExpr
     | VAR '(' (expr (',' expr)*)? ')'                            #CallExpr
     ;


VAR : [a-zA-Z_][a-zA-Z0-9_]* ;
INT : [0-9]+ ;
CHAR : '\'' . '\'' ;
COMMENTMULTI : '/*' .*? '*/' -> skip ;
COMMENT : '//' ~[\r\n]* -> skip ;
DIRECTIVE : '#' .*? '\n' -> skip ;
WS    : [ \t\r\n] -> channel(HIDDEN);
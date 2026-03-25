grammar ifcc;

axiom : prog EOF ;

prog : function_decl+;

function_decl : type VAR '(' param_list? ')' block;

type : 'int' | 'void';

param_list : param (',' param)*;

param : 'int' VAR;

block : '{'stmt*'}' ;

stmt : declare_stmt ';' | return_stmt ';' | expr ';' | block | if_stmt | while_stmt ;

declare_stmt : 'int' declare_elmt (',' declare_elmt)* ;
declare_elmt : VAR | assign_stmt ;
assign_stmt : VAR '=' expr ;

return_stmt : 'return' expr? ';' ;

if_stmt : 'if' '(' expr ')' stmt ('else' stmt)? ; 

while_stmt : 'while' '(' expr ')' stmt ;

expr : '(' expr ')'                                    #ParensExpr
     | OP=('!' | '-')expr                              #UnitaryExpr
     | expr OP=('*' | '/' | '%') expr                  #MultDivModExpr
     | expr OP=('+' | '-') expr                        #AddSubExpr
     | expr OP=('>' | '<' | '>=' | '<=') expr          #CompareExpr
     | expr OP=('==' | '!=') expr                      #EqualExpr
     | expr '&' expr                                   #LogicBitANDExpr
     | expr '^' expr                                   #LogicBitXORExpr
     | expr '|' expr                                   #LogicBitORExpr
     | expr '&&' expr                                  #LogicANDExpr
     | expr '||' expr                                  #LogicORExpr
     | VAR OP=('=' | '+=' | '-=' | '*=' | '/=') expr   #AssignExpr
     | (INT | CHAR )                                   #ConstExpr
     | VAR                                             #VarExpr
     | VAR '(' (expr (',' expr)*)? ')'                 #CallExpr
     ;

DOUBLEDASH : '--' ;
VAR : [a-zA-Z_][a-zA-Z0-9_]* ;
INT : [0-9]+ ;
CHAR : '\'' . '\'' ;
COMMENTMULTI : '/*' .*? '*/' -> skip ;
COMMENT : '//' ~[\r\n]* -> skip ;
DIRECTIVE : '#' .*? '\n' -> skip ;
WS    : [ \t\r\n] -> channel(HIDDEN);
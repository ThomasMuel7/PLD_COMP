grammar ifcc;

axiom : prog EOF ;

prog : 'int' 'main' '(' ')' block;

block : '{'stmt*'}' ;

stmt : declare_stmt | return_stmt | expr ';' | block ;

declare_stmt : 'int' VAR (',' VAR)* ';' ;

return_stmt : 'return' expr ';' ;

expr : '(' expr ')'                                    #ParensExpr
     | OP=('!' | '-')expr                              #UnitaryExpr
     | expr OP=('*' | '/' | '%') expr                  #MultDivModExpr
     | expr OP=('+' | '-') expr                        #AddSubExpr
     | expr OP=('>' | '<' | '>=' | '<=') expr          #CompareExpr
     | expr OP=('==' | '!=') expr                      #EqualExpr
     | expr '&' expr                                   #LogicBitANDExpr
     | expr '^' expr                                   #LogicBitXORExpr
     | expr '|' expr                                   #LogicBitORExpr
     | VAR OP=('=' | '+=' | '-=' | '*=' | '/=') expr   #AssignExpr
     | (INT | CHAR )                                   #ConstExpr
     | VAR                                             #VarExpr
     ;

DOUBLEDASH : '--' ;
VAR : [a-zA-Z_][a-zA-Z0-9_]* ;
INT : [0-9]+ ;
CHAR : '\'' . '\'' ;
COMMENTMULTI : '/*' .*? '*/' -> skip ;
COMMENT : '//' ~[\r\n]* -> skip ;
DIRECTIVE : '#' .*? '\n' -> skip ;
WS    : [ \t\r\n] -> channel(HIDDEN);
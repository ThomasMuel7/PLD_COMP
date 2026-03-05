grammar ifcc;

axiom : prog EOF ;

prog : 'int' 'main' '(' ')' '{' stmt* '}' ;

stmt : declare_stmt | assign_stmt | return_stmt ;

declare_stmt : 'int' VAR (',' VAR)* ';' ;

assign_stmt : VAR OP=('=' | '+=' | '-=' | '*=' | '/=') expr ';' ;

return_stmt : 'return' expr ';' ;

expr : '(' expr ')'                                    #ParensExpr
     | OP=('!' | '-') expr                             #UnitaryExpr
     | expr OP=('*' | '/' | '%') expr                  #MultDivModExpr
     | expr OP=('+' | '-') expr                        #AddSubExpr
     | expr OP=('>' | '<' | '>=' | '<=') expr          #CompareExpr
     | expr OP=('==' | '!=') expr                      #EqualExpr
     | expr '&' expr                                   #LogicBitANDExpr
     | expr '^' expr                                   #LogicBitXORExpr
     | expr '|' expr                                   #LogicBitORExpr
     | (INT | CHAR )                                   #ConstExpr
     | VAR                                             #VarExpr
     ;

VAR : [a-zA-Z_][a-zA-Z0-9_]* ;
INT : [0-9]+ ;
CHAR : '\'' . '\'' ;
COMMENTMULTI : '/*' .*? '*/' -> skip ;
COMMENT : '//' .* '\n' -> skip ;
DIRECTIVE : '#' .*? '\n' -> skip ;
WS    : [ \t\r\n] -> channel(HIDDEN);